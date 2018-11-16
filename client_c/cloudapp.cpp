#include "cloudapp.h"
#include "comm/strparse.h"
#include "comm/sock.h"
#include "cloud/switchhand.h"
#include "cloud/msgid.h"
#include "cloud/exception.h"
#include "cloud/const.h"
#include "cloud/homacro.h"
#include "rapidjson/json.hpp"
#include <sys/epoll.h>

CloudApp* CloudApp::This = NULL;
HEPMUTICLASS_IMPL(CloudApp, CloudApp, IOHand)

CloudApp::CloudApp()
{
	m_appid = 0;
	m_epfd = INVALID_FD;
	m_stage = 0;
	m_seqid = 0;
	m_port = 0;
	m_cliType = 100;
	m_inqueue = false;
	m_existLink = false;
	m_inEpRun = false;
	This = this;
}

// 读出配置文件, 构造出连接PeerServ的对象
int CloudApp::init( int epfd, const string& svrhost_port, const string& appname )
{
	const int connect_timeout_sec = 3;
	vector<string> vec;
	int ret = StrParse::SpliteStr(vec, svrhost_port, ':');
	ERRLOG_IF1RET_N(ret || vec.size() < 2, -24, 
		"ClOUDAPP_INIT| msg=invalid host config| config=%s", svrhost_port.c_str());

	m_epfd = epfd;
	m_rhost = vec[0];
	m_port = atoi(vec[1].c_str());
	m_svrname = appname;

	m_idProfile = "connTO" + svrhost_port;
	m_epCtrl.setEPfd(epfd);

	// 启动初始，同步方式进行连接
	reset();
	ret = Sock::connect(m_cliFd, m_rhost.c_str(), m_port, connect_timeout_sec, false, false);
	ERRLOG_IF1(ret, "CLOUDAPPCONNECT| msg=connect to %s fail| ret=%d", m_rhost.c_str(), ret);
	if (0 == ret) // 连接成功
	{
		m_existLink = true;
		string whoamistr = whoamiMsg();
		m_stage = 1; // connecting
		m_cliName = Sock::peer_name(m_cliFd, true);
		m_idProfile = m_cliName;

		string resp;
		ret = begnRequest(resp, CMD_WHOAMI_REQ, whoamistr);
		ERRLOG_IF1RET_N(ret, -5, "CLOUDAPPSEND| msg=send CMD_WHOAMI_REQ fail %d", ret);
		ret = onCMD_WHOAMI_RSP(resp);

		ERRLOG_IF1(ret, "CLOUDAPPSEND| msg=tell whoami to %s fail| ret=%d", m_rhost.c_str(), ret);

		addCmdHandle(CMD_WHOAMI_RSP, OnCMD_WHOAMI_RSP);
		addCmdHandle(CMD_KEEPALIVE_REQ, OnCMD_KEEPALIVE_REQ);
		addCmdHandle(CMD_EVNOTIFY_REQ, OnCMD_EVNOTIFY_REQ);
	}

	return ret;
}

int CloudApp::setToEpollEv( void )
{
	int ret = m_epCtrl.setActFd(m_cliFd);
	ret |= m_epCtrl.setEvt(EPOLLOUT|EPOLLIN, this);

	m_inEpRun = true;
	return ret;
}

// 在同一线程中进行请求和接收一消息，不经过io复用过程
// 调用条件： m_cliFd已连接且不加进epoll
int CloudApp::begnRequest( string& resp, unsigned cmdid, const string& reqmsg, bool noRcv )
{
	IFRETURN_N(0 != m_epCtrl.m_eventFg, -11);
	IOBuffItem obf;
	obf.setData(cmdid, ++m_seqid, reqmsg.c_str(), reqmsg.length());

	int ret;
	do
	{
		// 1. 发送请求
		ret = ::send(m_cliFd, obf.head(), obf.totalLen, 0);
		IFBREAK_N(ret != (int)obf.totalLen, -6);
		IFBREAK_N(noRcv, 0);

		// 2. 接收报文头
		char buff[HEADER_LEN + 10240];
		ret = ::recv(m_cliFd, buff, HEADER_LEN, 0);
		IFBREAK_N(ret != HEADER_LEN, -7);

		IOBuffItem ibf;
		ibf.buff.append(buff, HEADER_LEN);
		ibf.ntoh();
		head_t* hdr = ibf.head();
		ERRLOG_IF1BRK(hdr->head_len != HEADER_LEN || hdr->body_len > g_maxpkg_len, -8, 
			"SYNCREQ| headlen=%u| bodylen=%u", hdr->head_len, hdr->body_len);

		//3. 接收报文体
		IFBREAK_N(hdr->body_len >= sizeof(buff), -8);
		ret = ::recv(m_cliFd, buff, hdr->body_len, 0);
		IFBREAK_N(ret != (int)hdr->body_len, -9);
		buff[hdr->body_len] = '\0';
		resp.append(buff);
		ret = 0;
	}
	while(0);
	ERRLOG_IF1(ret, "SYNCREQ| msg=send or recv fail %d| err=(%d)%s", ret, errno, strerror(errno));

	return ret;
}

void CloudApp::setSvrid( int svrid )
{
	m_appid = svrid;
}

void CloudApp::setNotifyCB( const string& notify, NotifyCBFunc func )
{
	ERRLOG_IF1RET(m_ntfCB.end() != m_ntfCB.find(notify), "SETNTFCB| msg=exist notify %s CB", notify.c_str());
	m_ntfCB[notify] = func;
}

void CloudApp::reset( void )
{
	m_stage = 0;
	m_closeReason.clear();
	m_closeFlag = 0;
	m_existLink = false;
	IFCLOSEFD(m_cliFd);
}

int CloudApp::notifyParent(int evtype, ...)
{
	return 0;
}

int CloudApp::onClose( int p1, long p2 )
{
	int ret = IOHand::onClose(p1, p2);
	LOGINFO("CLOUDAPPCLOSE| msg=disconnect with %s| p1=%d| p2=%ld", m_rhost.c_str(), p1, p2);
	appendTimerq();

	return ret;
}

int CloudApp::appendTimerq( void )
{
	int ret = 0;
	if (!m_inqueue)
	{
		int wait_time_msec = 10*1000;
		ret = SwitchHand::Instance()->appendQTask(this, wait_time_msec );
		m_inqueue = (0 == ret);
		ERRLOG_IF1(ret, "APPENDQTASK| msg=append fail| ret=%d", ret);
	}
	return ret;
}


int CloudApp::qrun( int flag, long p2 )
{
	int  ret = 0;
	m_inqueue = false;
	if (0 == flag)
	{
		try 
		{
			ret = taskRun(flag, p2);
		}
		catch(OffConnException& exp)
		{
			appendTimerq();
			LOGERROR("CONNECTFAIL| msg=exception| reson=%s| mi=%s", exp.reson.c_str(), m_idProfile.c_str());
			reset();
		}

		return ret;
	}
	else if (1 == flag)
	{
		IOHand::onClose(HEFG_PEXIT, 2);
	}

	return ret;
}

int CloudApp::taskRun( int flag, long p2 )
{
	int ret = 0;
	if (0 == m_stage && INVALID_FD != m_cliFd)
	{
		LOGERROR("PEERS_RUN| msg=unexcepct flow| sock=%d| mi=%s", Sock::geterrno(m_cliFd), m_idProfile.c_str());
		throw OffConnException("cliFd not null at stage0");
	}

	reset();
	ret = Sock::connect_noblock(m_cliFd, m_rhost.c_str(), m_port);
	if (ERRSOCK_AGAIN == ret || 0 == ret)
	{
		m_epCtrl.setActFd(m_cliFd);
		m_stage = 1; // connecting
		//setIntProperty("fd", m_cliFd);
		m_idProfile = StrParse::Format("conneting to %s:%d", m_rhost.c_str(), m_port);

		if (0 == ret)
		{
			m_cliName = Sock::peer_name(m_cliFd, true);
			m_idProfile = m_cliName;
		}

		string whoamistr = whoamiMsg();
		m_epCtrl.setEvt(EPOLLOUT | EPOLLIN, this);
		ret = sendData(CMD_WHOAMI_REQ, ++m_seqid, whoamistr.c_str(), whoamistr.length(), true);
	}
	else
	{
		throw OffConnException("connect fail");
	}

	return ret;
}

int CloudApp::OnCMD_WHOAMI_RSP( void* ptr, unsigned cmdid, void* param )
{
	IOBuffItem* iBufItem = (IOBuffItem*)param; 
	//unsigned seqid = iBufItem->head()->seqid; 
	string body = iBufItem->body();
	return This->onCMD_WHOAMI_RSP(body);
}

int CloudApp::OnCMD_KEEPALIVE_REQ( void* ptr, unsigned cmdid, void* param )
{
	IOBuffItem* iBufItem = (IOBuffItem*)param; 
	unsigned seqid = iBufItem->head()->seqid;
	LOGDEBUG("CMD_KEEPALIVE| msg=kv resp");
	return This->sendData(CMD_KEEPALIVE_RSP, seqid, "", 0, true);	
}

int CloudApp::onCMD_WHOAMI_RSP( string& whoamiResp )
{
	Document doc;
	if (doc.ParseInsitu((char*)whoamiResp.data()).HasParseError())
	{
		return -3;
	}
	
	int code = -1;
	RJSON_GETINT(code, &doc);
	if (0 == code) // success
	{
		RJSON_VGETINT_D(svrid, CONNTERID_KEY, &doc);
		if (svrid > 0)
		{
			m_appid = svrid;
			m_stage = 2;
		}
		RJSON_VGETSTR(m_mconf, "mconf", &doc);
	}

	LOGOPT_EI(0 == m_appid, "WHOAMI_RSP| resp=%s", Rjson::ToString(&doc).c_str());
	return m_appid > 0 ? 0 : -4;
}

int CloudApp::OnSyncMsg( void* ptr, unsigned cmdid, void* param )
{
	return This->onSyncMsg(ptr, cmdid, param);
}
int CloudApp::onSyncMsg( void* ptr, unsigned cmdid, void* param )
{
	IOBuffItem* iBufItem = (IOBuffItem*)param;
	unsigned seqid = iBufItem->head()->seqid;

	int ret = m_syncHand.notify(cmdid, seqid, iBufItem->body());
	ERRLOG_IF1(ret, "SYNCMSG| msg=maybe msg resp delay| cmdid=0x%x| seqid=%u", cmdid, seqid);
	return 0;
}

int CloudApp::OnCMD_EVNOTIFY_REQ( void* ptr, unsigned cmdid, void* param )
{
    return This->onCMD_EVNOTIFY_REQ(ptr, cmdid, param);
}
int CloudApp::onCMD_EVNOTIFY_REQ( void* ptr, unsigned cmdid, void* param )
{
	MSGHANDLE_PARSEHEAD(false);
	RJSON_GETSTR_D(notify, &doc);

	map<string, NotifyCBFunc>::iterator itr = m_ntfCB.find(notify);
	if (m_ntfCB.end() != itr)
	{
		(itr->second)(&doc);
	}
	else
	{
		LOGWARN("EVNOTIFY| msg=no callback| notify=%s", notify.c_str());
	}

	return 0;
}

bool CloudApp::isInitOk( void ) const
{
	return m_existLink && m_appid > 0 && m_stage > 1;
}

bool CloudApp::isInEpRun( void ) const
{
	return m_inEpRun;
}

void CloudApp::uninit( void )
{
	m_epCtrl.setEvt(0, NULL);
	reset();
}

string CloudApp::whoamiMsg( void ) const
{
	string whoIamJson;

	whoIamJson += "{";
	StrParse::PutOneJson(whoIamJson, CONNTERID_KEY, m_appid, true);
	StrParse::PutOneJson(whoIamJson, SVRNAME_KEY, m_svrname.c_str(), true);
	StrParse::PutOneJson(whoIamJson, CLISOCKET_KEY, Sock::sock_name(m_cliFd, true, false), true);
	StrParse::PutOneJson(whoIamJson, "begin_time", (int)time(NULL), true);
	
	StrParse::PutOneJson(whoIamJson, "pid", getpid(), true);
	StrParse::PutOneJson(whoIamJson, CLIENT_TYPE_KEY, m_cliType, false);

	whoIamJson += "}";
	return whoIamJson;
}

string CloudApp::getMConf( void ) const
{
	return m_mconf;
}

string CloudApp::getLocalIP( void ) const
{
	return Sock::sock_name(m_cliFd);
}

// 同步等待请求+响应全过程完成
int CloudApp::syncRequest( string& resp, unsigned cmdid, const string& reqmsg, int tosec ) 
{
	ERRLOG_IF1RET_N(!m_inEpRun, -79,
			"SYNCREQ| msg=maybe should call begnRequest() instand| cmdid=0x%x", cmdid);

	int ret;
	unsigned seqid = ++m_seqid;
	unsigned rspid = (cmdid | CMDID_MID);

	bool badd = addCmdHandle(rspid, OnSyncMsg, seqid);
	ERRLOG_IF1RET_N(!badd, -80, "SYNCREQ| msg=addCmdHandle fail| cmdid=0x%x| seqid=%u", cmdid, seqid);
	
	do
	{
		ret = m_syncHand.putRequest(rspid, seqid, tosec);
		if (ret)
		{
			delCmdHandle(rspid, seqid);
			return -81;
		}

		ret = sendData(cmdid, seqid, reqmsg.c_str(), reqmsg.size(), true);
		ERRLOG_IF1BRK(ret, -82, "SYNCREQ| msg=sendData fail %d| cmdid=0x%x| seqid=%u", ret, cmdid, seqid)

		ret = m_syncHand.waitResponse(resp, rspid, seqid);
	}
	while (0);

	// 结束清理中间过程数据
	delCmdHandle(rspid, seqid);
	m_syncHand.delRequest(rspid, seqid);

	return ret;
}

int CloudApp::postRequest( unsigned cmdid, const string& reqmsg )
{
	return postRequest(cmdid, reqmsg, !m_inEpRun);
}

// 发送消息后不等待响应就返回
// param: noEpFlag 当启动阶段未进入io-epoll复用前传true； 有io-epoll复用的业务里传false
int CloudApp::postRequest( unsigned cmdid, const string& reqmsg, bool noEpFlag )
{
	if (noEpFlag)
	{
		string tmp;
		return begnRequest(tmp, cmdid, reqmsg, true);
	}

	return sendData(cmdid, ++m_seqid, reqmsg.c_str(), reqmsg.size(), true);
}