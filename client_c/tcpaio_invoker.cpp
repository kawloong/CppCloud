#include "tcpaio_invoker.h"
#include "msgprop.h"
#include "comm/hep_base.h"
#include "comm/strparse.h"
#include "comm/sock.h"
#include "comm/lock.h"
#include "cloud/const.h"
#include "cloud/switchhand.h"
#include <sys/epoll.h>
#include <cstring>
#include <cerrno>


datasize_t TcpAioInvoker::serv_recv_bytes = 0;
datasize_t TcpAioInvoker::serv_send_bytes = 0;
int TcpAioInvoker::serv_recvpkg_num = 0;
int TcpAioInvoker::serv_sendpkg_num = 0;



TcpAioInvoker::TcpAioInvoker( const string& dsthostp ): 
		m_dstPort(0), m_stage(0), 
		m_cliFd(INVALID_FD), m_seqid(0), m_timeout_interval_sec(3), m_atime(0),
		m_epThreadID(0), m_closeFlag(0),
		m_recv_bytes(0), m_send_bytes(0), 
		m_recvpkg_num(0), m_sendpkg_num(0), m_iBufItem(NULL), m_oBufItem(NULL),
		m_inTimerq(false)
{
	size_t pos = dsthostp.find(":");
    if (pos > 0)
    {
        m_dstHost = dsthostp.substr(0, pos);
        m_dstPort = atoi(dsthostp.c_str() + pos + 1);
    }
}

int TcpAioInvoker::init( int epfd, int timeout_sec )
{
	m_epCtrl.setEPfd(epfd);
	m_timeout_interval_sec = timeout_sec;
	
	ERRLOG_IF1RET_N(m_dstPort<=0 || m_dstHost.empty(), -133, 
			"INVOKERAIO_INIT| msg=invalid hostp %s:%d",
			m_dstHost.c_str(), m_dstPort);
	
	return _connect();
}

TcpAioInvoker::~TcpAioInvoker(void)
{
	clearBuf();
}


int TcpAioInvoker::_connect( void )
{
	IFCLOSEFD(m_cliFd);
	m_closeFlag = 0;
	m_closeReason.clear();
	
	int ret = Sock::connect_noblock(m_cliFd, m_dstHost.c_str(), m_dstPort);
	LOGDEBUG("INVOKERAIO_INIT| msg=connecting %s:%d", m_dstHost.c_str(), m_dstPort);
	m_atime = time(NULL);

	if (ERRSOCK_AGAIN == ret)
	{
		m_stage = 1;
		m_epCtrl.setActFd(m_cliFd);
		ret = m_epCtrl.setEvt(EPOLLOUT|EPOLLIN, this);
		ERRLOG_IF1RET_N(ret, -131, "INVOKERAIO_INIT| msg=connect to %s:%d , setev fail| ret=%d",
				m_dstHost.c_str(), m_dstPort, ret);
	}
	else if (0 == ret)
	{
		m_epCtrl.setActFd(m_cliFd);
		ret = m_epCtrl.setEvt(EPOLLOUT|EPOLLIN, this);
		m_stage = 2;
	}
	else
	{
		ERRLOG_IF1RET_N(ret, -130, "INVOKERAIO_INIT| msg=connect to %s:%d fail| ret=%d",
				m_dstHost.c_str(), m_dstPort, ret);
	}
	
	return 0;
}

void TcpAioInvoker::clearBuf( void )
{
	IFDELETE(m_iBufItem);
	IFDELETE(m_oBufItem);
	clearReqQueue();

	IOBuffItem* buf = NULL;
	while (m_oBuffq.pop(buf, 0))
	{
		IFDELETE(buf);
	}
}

void TcpAioInvoker::clearReqQueue( void )
{
	if (!m_reqQueue.empty())
	{
		RWLOCK_WRITE(m_qLock);
		for (auto it = m_reqQueue.begin(); m_reqQueue.end() != it; ++it)
		{
			it->second->append(""); // to unblock wait
		}
		m_reqQueue.clear();
	}
}

int TcpAioInvoker::onRead( int p1, long p2 )
{
	int ret = 0;
	
	do {
		if (NULL == m_iBufItem)
		{
			m_iBufItem = new IOBuffItem;
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
				throw InvokerException(0, m_closeReason);
			}
			if (ret < 0) // close 清理流程 
			{
				m_closeReason = string("recv lt 0 ") + (strerror(errno));
				throw InvokerException(-10, m_closeReason);
			}

			m_iBufItem->len += rcvlen;
			if (ret != (int)rcvlen || m_iBufItem->len > HEADER_LEN)
			{
				m_closeReason = "Sock::recv sys error";
				LOGERROR("IOHAND_READ| msg=recv param error| ret=%d| olen=%u| rcvlen=%u| mi=%s",
				 	 ret, rcvlen, m_iBufItem->len,m_cliName.c_str());
				throw InvokerException(-11, m_closeReason);
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
				m_closeReason = StrParse::Format("headlen invalid(%u)", hdr->head_len);
				throw InvokerException(-12, m_closeReason);
			}
			if (hdr->body_len > g_maxpkg_len)
			{
				m_closeReason = StrParse::Format("bodylen invalid(%u)", hdr->body_len);
				throw InvokerException(-13, m_closeReason);
			}
			m_iBufItem->buff.resize(m_iBufItem->totalLen);
		}
		
		// body 接收
		if (m_iBufItem->totalLen > m_iBufItem->len)
		{
			ret = Sock::recv(m_cliFd, (char*)m_iBufItem->head(), m_iBufItem->len, m_iBufItem->totalLen);
			if (ret <= 0)
			{
				m_closeReason = (0==ret? "recv body closed": strerror(errno));
				throw InvokerException(-14, m_closeReason);
			}
			m_recv_bytes += ret;
			serv_recv_bytes += ret;
		}

		if (m_iBufItem->ioFinish()) // 报文接收完毕
		{
			m_recvpkg_num++;
			serv_recvpkg_num++;
			
			{
				ret = cmdProcess(m_iBufItem);
			}
			
		}
	}
	while (0);

	m_atime = time(NULL);
	if (m_iBufItem && m_iBufItem->ioFinish())
	{
		IFDELETE(m_iBufItem);
	}

	return ret;
}

int TcpAioInvoker::onWrite( int p1, long p2 )
{
	int ret = 0;

	do 
	{
		m_epCtrl.oneShotUpdate();
		if (NULL == m_oBufItem)
		{ // 注意同步问题... // 
			bool bpopr = m_oBuffq.pop(m_oBufItem, 0);
			if (!bpopr)
			{
				if (m_oBuffq.size() <= 0)
				{
					ret = m_epCtrl.rmEvt(EPOLLOUT);
				}
				LOGWARN("IOHAND_WRITE| msg=obuff pop fail| mi=%s", m_cliName.c_str());
				break;
			}
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

int TcpAioInvoker::run( int p1, long p2 )
{
	int ret = 0;
	
	try 
	{
		if (EPOLLIN & p1) // 有数据可读
		{
			ret = onRead(p1, p2);
		}
	}
	catch( InvokerException& exp )
	{
		LOGOPT_EI(exp.code, "OffConnException| code=%d| reson=%s | mi=%s", 
				exp.code, exp.reson.c_str(), m_cliName.c_str());
		m_closeFlag = 2;

		if (m_closeReason.empty()) m_closeReason = exp.reson;
		IFDELETE(m_iBufItem);
	}

	if (EPOLLOUT & p1) // 可写
	{
		if (1 == m_stage && !((EPOLLERR|EPOLLHUP) & p1))
		{
			m_cliName = Sock::peer_name(m_cliFd, true);
			m_stage = 2;
		}
		ret = onWrite(p1, p2);
	}


	if ((EPOLLERR|EPOLLHUP) & p1)
	{
		int fderrno = Sock::geterrno(m_cliFd);
		LOGERROR("IOHAND_OTHRE| msg=%s| dst=%s:%d| err=%d(%s)", 
				0==m_stage? "connect fail": "sock err",
				m_dstHost.c_str(), m_dstPort, fderrno, strerror(fderrno));
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

int TcpAioInvoker::qrun( int flag, long p2 )
{
	// callback方式调用超时处理
	time_t now = time(NULL);
	int dtsec = 0;
	m_inTimerq = false;
	for (const auto & waitItem : m_reqCBQueue)
	{
		time_t tend = std::get<0>(waitItem.second);
		if (tend > now)
		{
			dtsec = tend - now;
			break;
		}

		std::get<1>(waitItem.second)(-15, waitItem.first, "timeout");
		m_reqCBQueue.erase(waitItem.first);
	}

	if (dtsec > 0 && 0 == flag)
	{
		appendTimerQWait(dtsec);
	}

	return 0;
}

void TcpAioInvoker::appendTimerQWait( int dtsec )
{
	if (!m_inTimerq)
	{
		m_inTimerq = (0 == SwitchHand::Instance()->appendQTask(this, dtsec * 1000));
	}
}

int TcpAioInvoker::sendData( unsigned int cmdid, unsigned int seqid, const char* body, unsigned int bodylen, bool setOutAtonce )
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


time_t TcpAioInvoker::getAtime( void )
{
	return m_atime;
}

// @param: [out] format {all:[进程收到的字节数, 进程发出的字节数, 进程收到的报文数, 进程发出的报文数], 
//		one:[当前连接收到的字节数, 当前连接发出的字节数, 当前连接收到的报文数, 当前连接发出的报文数]}
int TcpAioInvoker::getIOStatJson( string& rspjson ) const
{
	StrParse::AppendFormat(rspjson, "\"all\":[%lld, %lld, %d, %d]",
		serv_recv_bytes, serv_send_bytes, serv_recvpkg_num, serv_sendpkg_num);
	
	StrParse::AppendFormat(rspjson, ",\"one\":[%lld, %lld, %d, %d]",
		m_recv_bytes, m_send_bytes, m_recvpkg_num, m_sendpkg_num);
	
	return 0;
}

int TcpAioInvoker::driveClose( const string& reason )
{
	m_closeReason = reason;
	return onClose(0, 0);
}

int TcpAioInvoker::onClose( int p1, long p2 )
{
	int ret = 0;

	// 清理完监听的evflag
	ret = m_epCtrl.setEvt(0, NULL);
	ERRLOG_IF1(ret, "INVOKERAIO_CLOSE| msg=rm EVflag fail %d| mi=%s| err=%s", 
			ret, m_cliName.c_str(), strerror(errno));

	IFCLOSEFD(m_cliFd);

	clearBuf();
	m_closeFlag = 3;
	m_stage = 0;

	return ret;
}


void TcpAioInvoker::setEpThreadID( void )
{
	m_epThreadID = pthread_self();
}

void TcpAioInvoker::setEpThreadID( pthread_t thid )
{
	m_epThreadID = thid;
}


// 通用（类）消息处理
int TcpAioInvoker::cmdProcess( IOBuffItem*& iBufItem )
{
	int ret = 0;
	do 
	{
		IFBREAK_N(NULL==iBufItem, -71);
		head_t* hdr = iBufItem->head();
		int seqid = hdr->seqid;
		int cmdid = hdr->cmdid;
		ERRLOG_IF1BRK(cmdid <= CMDID_MID, -72, 
			"INVOKERPROCESS| msg=recv req cmdid| cmdid=0x%x| mi=%s", cmdid, m_cliName.c_str());
		{
			RWLOCK_READ(m_qLock);
			auto itr = m_reqQueue.find(seqid);
			if (m_reqQueue.end() == itr)
			{
				auto itrCB = m_reqCBQueue.find(seqid);
				if (m_reqCBQueue.end() != itrCB)
				{
					std::get<1>(itrCB->second)(0, seqid, iBufItem->body());
					ret = 0;
					m_reqCBQueue.erase(itrCB);
					break;
				}

				LOGERROR("INVOKERPROCESS| msg=no reqQueue wait, maybe response late| "
					"cmdid=0x%x| seqid=%d| mi=%s", cmdid, seqid, m_cliName.c_str());
				break;
			}

			itr->second->append(iBufItem->body(), 0);
		}

	}
	while (0);
	IFDELETE(iBufItem);

	return ret;
}

int TcpAioInvoker::stageCheck( void )
{
	IFRETURN_N(2 == m_stage || 1 == m_stage, 0);
	int ret = _connect();
	return ret;
}

// thread-safe
int TcpAioInvoker::request( string& resp, int cmdid, const string& reqmsg )
{
	static const int sec2us = 1000000;
	int seqid = ++m_seqid;
	shared_ptr< Queue<string, false> > shareQueue = make_shared< Queue<string, false> >();
	{
		RWLOCK_WRITE(m_qLock);
		m_reqQueue[seqid] = shareQueue;
	}

	int ret;
	do
	{
		ret = stageCheck();
		IFBREAK_N(ret, -13);
		ret = sendData(cmdid, seqid, reqmsg.c_str(), reqmsg.length(), true);
		IFBREAK_N(ret, -14);

		bool bpop = shareQueue->pop(resp, m_timeout_interval_sec * sec2us); // block until response
		ERRLOG_IF1BRK(!bpop, -15, "INVOKETIMEOUT| msg=req 0x%x at seq%d fail", cmdid, seqid);
	}
	while(0);
	
	{
		RWLOCK_WRITE(m_qLock);
		m_reqQueue.erase(seqid);
	}

	return ret;
}

int TcpAioInvoker::request( InvkCBFunc cb_func, int cmdid, const string& reqmsg )
{
	int seqid = ++m_seqid;
	{
		RWLOCK_WRITE(m_qLock);
		m_reqCBQueue[seqid] = std::make_tuple(time(NULL)+m_timeout_interval_sec, cb_func);
	}

	int ret;
	do
	{
		ret = stageCheck();
		IFBREAK_N(ret, -17);
		ret = sendData(cmdid, seqid, reqmsg.c_str(), reqmsg.length(), true);
		IFBREAK_N(ret, -18);
	}
	while(0);
	
	if (ret < 0)
	{
		RWLOCK_WRITE(m_qLock);
		m_reqCBQueue.erase(seqid);
		return ret;
	}

	appendTimerQWait(m_timeout_interval_sec);
	return seqid;
}