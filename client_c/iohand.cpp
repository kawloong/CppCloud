#include "iohand.h"
#include "comm/strparse.h"
#include "comm/sock.h"
#include "comm/lock.h"
#include "cloud/const.h"
#include "cloud/exception.h"
//#include "climanage.h"
#include <sys/epoll.h>
#include <cstring>
#include <cerrno>

HEPCLASS_IMPL(IOHand, IOHand)

static map<unsigned, ProcOneFunT> s_cmdid2interceptor; // 消息拦截器
static map<unsigned, ProcOneFunT> s_cmdid2clsname; // 事件处理器，滞后于拦截器
static RWLock gCmdHandleLocker;

datasize_t IOHand::serv_recv_bytes = 0;
datasize_t IOHand::serv_send_bytes = 0;
int IOHand::serv_recvpkg_num = 0;
int IOHand::serv_sendpkg_num = 0;
PEVENT_FUNC IOHand::m_parentEvHandle = NULL;

// static
int IOHand::Init( PEVENT_FUNC phand )
{
	m_parentEvHandle = phand;
	return 0;
}

void IOHand::AddCmdHandle( unsigned cmdid, ProcOneFunT func )
{
	s_cmdid2clsname[cmdid] = func;
}

IOHand::IOHand(void): m_cliFd(INVALID_FD), m_closeFlag(0),
		m_authFlag(0), m_recv_bytes(0), m_send_bytes(0), 
		m_recvpkg_num(0), m_sendpkg_num(0), m_iBufItem(NULL), m_oBufItem(NULL)
{
    
}
IOHand::~IOHand(void)
{
	clearBuf();
}

void IOHand::clearBuf( void )
{
	IFDELETE(m_iBufItem);
	IFDELETE(m_oBufItem);
	IOBuffItem* buf = NULL;
	while (m_oBuffq.pop(buf, 0))
	{
		IFDELETE(buf);
	}
}

int IOHand::onRead( int p1, long p2 )
{
	int ret = 0;
	
	do {
		if (NULL == m_iBufItem)
		{
			m_iBufItem = new IOBuffItem;
		}

		if (m_cliName.empty())
		{
			m_cliName = Sock::peer_name(m_cliFd, true);
			setProperty("_ip", Sock::peer_name(m_cliFd, false));
		}
		
		if (m_iBufItem->len < HEADER_LEN)
		{
			unsigned rcvlen = 0;
			char buff[HEADER_LEN];
			ret = Sock::recv(m_cliFd, buff, rcvlen, HEADER_LEN);
			IFBREAK_N(ERRSOCK_AGAIN == ret, 0);

			if (0 == ret)
			{
				m_closeReason = "normal recv close";
				m_closeFlag = 2;
				break;
			}
			if (ret < 0) // close 清理流程 
			{
				m_closeReason = (strerror(errno));
				throw OffConnException(m_closeReason);
			}

			m_iBufItem->len += rcvlen;
			if (ret != (int)rcvlen || m_iBufItem->len > HEADER_LEN)
			{
				m_closeFlag = 2;
				m_closeReason = "Sock::recv sys error";
				LOGERROR("IOHAND_READ| msg=recv param error| ret=%d| olen=%u| rcvlen=%u| mi=%s",
				 	 ret, rcvlen, m_iBufItem->len,m_cliName.c_str());
				break;
			}
			
			IFBREAK(m_iBufItem->len < HEADER_LEN); // 头部未完整,wait again
			m_recv_bytes += m_iBufItem->len;
			serv_recv_bytes += m_iBufItem->len;
			m_iBufItem->buff.append(buff, m_iBufItem->len); // binary data

			m_iBufItem->ntoh();
			// m_iBufItem->totalLen 合法检查
			head_t* hdr = m_iBufItem->head();
			if (hdr->head_len != HEADER_LEN)
			{
				m_closeFlag = 2;
				m_closeReason = StrParse::Format("headlen invalid(%u)", hdr->head_len);
				break;
			}
			if (hdr->body_len > g_maxpkg_len)
			{
				m_closeFlag = 2;
				m_closeReason = StrParse::Format("bodylen invalid(%u)", hdr->body_len);
				break;
			}
			m_iBufItem->buff.resize(m_iBufItem->totalLen);
		}
		
		// body 接收
		if (m_iBufItem->totalLen > m_iBufItem->len)
		{
			ret = Sock::recv(m_cliFd, (char*)m_iBufItem->head(), m_iBufItem->len, m_iBufItem->totalLen);
			if (ret <= 0)
			{
				m_closeFlag = 2;
				m_closeReason = (0==ret? "recv body closed": strerror(errno));
				break;
			}
			m_recv_bytes += ret;
			serv_recv_bytes += ret;
		}

		if (m_iBufItem->ioFinish()) // 报文接收完毕
		{
			m_recvpkg_num++;
			serv_recvpkg_num++;
			IFBREAK(authCheck(m_iBufItem)); // 权限检查
			ret = interceptorProcess(m_iBufItem);
			IFBREAK_N(1 != ret, 0);

			if (m_child)
			{
				if (1 == Notify(m_child, HEPNTF_NOTIFY_CHILD, (IOBuffItem*)m_iBufItem))
				{
					ret = cmdProcess(m_iBufItem); // parse package [cmdid]
				}
			}
			else
			{
				ret = cmdProcess(m_iBufItem);
			}
			
			NotifyParent(HEPNTF_UPDATE_TIME, this);
		}
	}
	while (0);

	if (m_iBufItem && m_iBufItem->ioFinish())
	{
		IFDELETE(m_iBufItem);
	}

	return ret;
}

int IOHand::onWrite( int p1, long p2 )
{
	int ret = 0;

	do {
		m_epCtrl.oneShotUpdate();
		if (NULL == m_oBufItem)
		{ // 同步问题... // 
			bool bpopr = m_oBuffq.pop(m_oBufItem, 0);
			WARNLOG_IF1BRK(!bpopr, 0, "IOHAND_WRITE| msg=obuff pop fail| mi=%s", m_cliName.c_str());
		}
		
		ret = Sock::send(m_cliFd, (char*)m_oBufItem->head(), m_oBufItem->len, m_oBufItem->totalLen);
		if (ret <= 0)
		{
			// maybe closed socket
			m_closeFlag = 2;
			m_closeReason = StrParse::Format("send err(%d) sockerr(%s)", ret, strerror(Sock::geterrno(m_cliFd)));
			break;
		}
		
		m_send_bytes += ret;
		serv_send_bytes += ret;
		if (m_oBufItem->len < m_oBufItem->totalLen)
		{
			LOGDEBUG("IOHAND_WRITE| msg=obuf send wait again| mi=%s", m_cliName.c_str());
			break;
		}

		m_sendpkg_num++;
		serv_sendpkg_num++;
		IFDELETE(m_oBufItem);
		if (m_oBuffq.size() <= 0)
		{
			ret = m_epCtrl.rmEvt(EPOLLOUT);
			ERRLOG_IF1(ret, "IOHAND_WRITE| msg=rmEvt fail %d| mi=%s| eno=%d", ret, m_cliName.c_str(), errno);
			if (m_oBuffq.size() > 0) // 此处因存在多线程竞争而加此处理
			{
				ret = m_epCtrl.addEvt(EPOLLOUT);
			}

			if (1 == m_closeFlag)
			{
				m_closeFlag = 2;
			}
		}
	}
	while (0);
	return ret;
}

int IOHand::run( int p1, long p2 )
{
	int ret = 0;
	
	try 
	{
		if (EPOLLIN & p1) // 有数据可读
		{
			ret = onRead(p1, p2);
		}
	}
	catch( NormalExceptionOff& exp ) // 业务异常,先发送错误响应,后主动关闭connection
	{
		string rsp = StrParse::Format("{ \"code\": %d, \"desc\": \"%s\" }", exp.code, exp.desc.c_str());
		m_closeFlag = 1; // 待发送完后close
		LOGERROR("NormalExceptionOff| reson=%s | mi=%s", exp.desc.c_str(), m_idProfile.c_str());
		m_closeReason = exp.desc;
		sendData(exp.cmdid, exp.seqid, rsp.c_str(), rsp.length(), true);
		IFDELETE(m_iBufItem);
	}
	catch ( NormalExceptionOn& exp )
	{
		string rsp = StrParse::Format("{ \"code\": %d, \"desc\": \"%s\" }", exp.code, exp.desc.c_str());
		LOGERROR("NormalExceptionOn| reson=%s | mi=%s", exp.desc.c_str(), m_idProfile.c_str());
		sendData(exp.cmdid, exp.seqid, rsp.c_str(), rsp.length(), true);
		IFDELETE(m_iBufItem);
	}
	catch( OffConnException& exp )
	{
		LOGERROR("OffConnException| reson=%s | mi=%s", exp.reson.c_str(), m_idProfile.c_str());
		m_closeFlag = 2;
		m_closeReason = exp.reson;
		IFDELETE(m_iBufItem);
	}
	catch( RouteExException& exp )
	{
		string rsp = StrParse::Format(
				"{ \"code\": -1, \"from\": %u, \"to\": %u, \"refer_path\":\"%s\", \"act_path\":\"%u>\", \"error\": \"RouteExException\" }", 
				exp.from, exp.to, exp.rpath.c_str(), exp.from);
		LOGERROR("RouteExException| reson=%s | mi=%s| rsp=%s", 
				exp.reson.c_str(), m_idProfile.c_str(), rsp.c_str());
		if (exp.reqcmd < CMDID_MID)
		{
			sendData(exp.reqcmd|CMDID_MID, exp.seqid, rsp.c_str(), rsp.length(), true);
		}
		IFDELETE(m_iBufItem);
	}

	if (EPOLLOUT & p1) // 可写
	{
		ret = onWrite(p1, p2);
	}


	if ((EPOLLERR|EPOLLHUP) & p1)
	{
		int fderrno = Sock::geterrno(m_cliFd);
		LOGERROR("IOHAND_OTHRE| msg=sock err| mi=%s| err=%d(%s)", m_cliName.c_str(), fderrno, strerror(fderrno));
		m_closeFlag = 2;
	}

	if (HEFG_PEXIT == p1 && 2 == p2) /// #PROG_EXITFLOW(6)
	{
		m_closeFlag = 2; // program exit told
	}

	if (m_closeFlag >= 2)
	{
		ret = onClose(p1, p2);
	}

	return ret;
}

int IOHand::onEvent( int evtype, va_list ap )
{
	int ret = 0;
	if (HEPNTF_INIT_PARAM == evtype)
	{
		int clifd = va_arg(ap, int);
		int epfd = va_arg(ap, int);
		
		m_epCtrl.setEPfd(epfd);
		m_epCtrl.setActFd(clifd);
		m_cliFd = clifd;
		m_cliName = Sock::peer_name(m_cliFd, true);
		m_idProfile = m_cliName;
		setIntProperty("fd", clifd);
		setProperty("_ip", Sock::peer_name(m_cliFd, false));
		ret = m_epCtrl.setEvt(EPOLLIN, this);

		LOGINFO("IOHAND_INIT| msg=a client accept| fd=%d| mi=%s", m_cliFd, m_cliName.c_str());
	}
	else if (HEPNTF_SEND_MSG == evtype)
	{
		unsigned int cmdid = va_arg(ap, unsigned int);
		unsigned int seqid = va_arg(ap, unsigned int);
		const char* body = va_arg(ap, const char*);
		unsigned int bodylen = va_arg(ap, unsigned int);

		ret = sendData(cmdid, seqid, body, bodylen, false);
	}
	else if (HEPNTF_SET_EPOUT == evtype)
	{
		ret = m_epCtrl.addEvt(EPOLLOUT);
		ERRLOG_IF1(ret, "IOHAND_SET_EPOU| msg=set out flag fail %d| mi=%s", ret, m_cliName.c_str());
	}
	else
	{
		ret = -99;
	}
	
	return ret;
}

void IOHand::setAuthFlag( int auth )
{
	m_authFlag = auth;
}

int IOHand::sendData( unsigned int cmdid, unsigned int seqid, const char* body, unsigned int bodylen, bool setOutAtonce )
{
	IOBuffItem* obf = new IOBuffItem;
	obf->setData(cmdid, seqid, body, bodylen);
	if (!m_oBuffq.append(obf))
	{
		LOGERROR("IOHANDSNDMSG| msg=append to oBuffq fail| len=%d| mi=%s", m_oBuffq.size(), m_cliName.c_str());
		delete obf;
		return -77;
	}

	if (setOutAtonce)
	{
		int ret = m_epCtrl.addEvt(EPOLLOUT);
		ERRLOG_IF1(ret, "IOHAND_SET_EPOU| msg=set out flag fail %d| mi=%s| fd=%d", ret, m_cliName.c_str(), m_cliFd);
	}

	return 0;
}


void IOHand::setProperty( const string& key, const string& val )
{
	m_cliProp[key] = val;
}

string IOHand::getProperty( const string& key ) const
{
	map<string, string>::const_iterator itr = m_cliProp.find(key);
	if (itr != m_cliProp.end())
	{
		return itr->second;
	}

	return "";
}

void IOHand::setIntProperty( const string& key, int val )
{
	m_cliProp[key] = StrParse::Itoa(val);
}

int IOHand::getIntProperty( const string& key ) const
{
	return atoi(getProperty(key).c_str());
}

int IOHand::driveClose( const string& reason )
{
	m_closeReason = reason;
	return onClose(0, 0);
}

int IOHand::onClose( int p1, long p2 )
{
	int ret = 0;
	int fderrno = Sock::geterrno(m_cliFd);
	bool isExit = (HEFG_PEXIT == p1 && 2 == p2);

	LOGOPT_EI(ret, "IOHAND_CLOSE| msg=iohand need quit %d| mi=%s| reason=%s| sock=%d%s",
		ret, m_cliName.c_str(), m_closeReason.c_str(), fderrno, fderrno?strerror(fderrno):"");

	IFDELETE(m_child); // 先清理子级对象

	// 清理完监听的evflag
	ret = m_epCtrl.setEvt(0, NULL);
	ERRLOG_IF1(ret, "IOHAND_CLOSE| msg=rm EVflag fail %d| mi=%s| err=%s", ret, m_cliName.c_str(), strerror(errno));

	IFCLOSEFD(m_cliFd);
	ret = NotifyParent(HEPNTF_SOCK_CLOSE, this, 10, (int)isExit);
	ERRLOG_IF1(ret, "IOHAND_CLOSE| msg=Notify ret %d| mi=%s| reason=%s", ret, m_cliName.c_str(), m_closeReason.c_str());

	clearBuf();
	m_closeFlag = 3;
	return ret;
}

int IOHand::NotifyParent(int evtype, ...)
{
	int ret = 0;
    va_list ap;
    va_start(ap, evtype);
    ret = m_parentEvHandle(evtype, ap);
    va_end(ap);

    return ret;
}

// 权限拦截器
int IOHand::authCheck( IOBuffItem*& iBufItem )
{
	return 0;
}

int IOHand::interceptorProcess( IOBuffItem*& iBufItem )
{
	int ret = 1; // 1 is continue
	head_t* hdr = iBufItem->head();
	map<unsigned, ProcOneFunT>::iterator it;

	it = s_cmdid2interceptor.find(hdr->cmdid);
	if (it != s_cmdid2interceptor.end())
	{
		ProcOneFunT func = it->second;
		ret = func(this, hdr->cmdid, (void*)iBufItem);
	}

	return ret;
}

bool IOHand::addCmdHandle( unsigned cmdid, ProcOneFunT func, unsigned seqid/*=0*/ )
{
	if (seqid > 0)
	{
		cmdid |= (seqid << 16);
	}

	RWLOCK_WRITE(gCmdHandleLocker);
	cmdhandle_t& val = m_cmdidHandle[cmdid];
	IFRETURN_N(val.handfunc, false);

	val.handfunc = func;
	val.key = cmdid;
	return true;
}

void IOHand::delCmdHandle( unsigned cmdid, unsigned seqid/*=0*/ )
{
	if (seqid > 0)
	{
		cmdid |= (seqid << 16);
	}

	RWLOCK_WRITE(gCmdHandleLocker);
	m_cmdidHandle.erase(cmdid);
}

// 通用（类）消息处理
int IOHand::cmdProcess( IOBuffItem*& iBufItem )
{
	int ret = 0;
	do 
	{
		IFBREAK_N(NULL==iBufItem, -71);
		IFBREAK(1 != selfCmdHandle(iBufItem));

		head_t* hdr = iBufItem->head();
		ProcOneFunT handle_func;
		map<unsigned, ProcOneFunT>::iterator it;
		it = s_cmdid2clsname.find(hdr->cmdid);
		WARNLOG_IF1BRK(s_cmdid2clsname.end() == it, -72, "CMDPROCESS| msg=an undefine cmdid recv| "
			"cmdid=0x%X| mi=%s", hdr->cmdid, m_cliName.c_str());
		handle_func = it->second;
		m_cliProp["clisock"] = m_cliName; // 设备ip:port作为其中属性

		ret = handle_func(this, hdr->cmdid, (void*)iBufItem);
	}
	while (0);
	IFDELETE(iBufItem);


	return ret;
}

// 实例的消息处理
// return: 1 will continue
int IOHand::selfCmdHandle( IOBuffItem*& iBufItem )
{
	int ret = 1;
	head_t* hdr = iBufItem->head();

	cmdhandle_t* cmdhand = NULL;
	gCmdHandleLocker.RLock();
	map<unsigned, cmdhandle_t>::iterator itr = m_cmdidHandle.find(hdr->cmdid | (hdr->seqid << 16));

	if (m_cmdidHandle.end() == itr)
	{
		itr = m_cmdidHandle.find(hdr->cmdid);
	}

	if (m_cmdidHandle.end() != itr)
	{
		cmdhand = &itr->second;
	}
	gCmdHandleLocker.UnLock();

	if (cmdhand)
	{
		cmdhand->handfunc(this, hdr->cmdid, (void*)iBufItem);
		if (1 == cmdhand->expire_time || cmdhand->expire_time < time(NULL))
		{
			RWLOCK_WRITE(gCmdHandleLocker);
			m_cmdidHandle.erase(cmdhand->key);
		}
		ret = 0;
	}	

	
	return ret;
}
