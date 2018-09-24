#include <sys/epoll.h>
#include "remote_serv.h"
#include "comm/strparse.h"
#include "comm/sock.h"
#include "cloud/const.h"
#include "exception.h"
#include "climanage.h"
#include "switchhand.h"


HEPMUTICLASS_IMPL(RemoteServ, RemoteServ, IOHand)

int RemoteServ::s_my_svrid = 0;

RemoteServ::RemoteServ(void): m_stage(0), m_seqid(0), m_svrid(0), m_epfd(INVALID_FD), m_port(0)
{
 	m_cliType = SERV_CLITYPE_ID;
	m_isLocal = true;
	//m_outObj = true;
	m_inqueue = false;
}

RemoteServ::~RemoteServ(void)
{
	
}

void RemoteServ::Init( int mysvrid )
{
	s_my_svrid = mysvrid;
}

int RemoteServ::init( const string& rhost, int port, int epfd )
{
	m_rhost = rhost;
	m_epfd = epfd;
	m_port = port;

	m_idProfile = "connTO" + rhost;
	m_epCtrl.setEPfd(epfd);
	return 0;
}

void RemoteServ::setSvrid( int svrid )
{
	m_svrid = svrid;
}

int RemoteServ::onEvent( int evtype, va_list ap )
{
	return 0;
}


int RemoteServ::qrun( int flag, long p2 )
{
	int  ret = 0;
	m_inqueue = false;
	if (0 == flag)
	{
		try {
			ret = taskRun(flag, p2);
		}
		catch(OffConnException& exp)
		{
			LOGERROR("REMOTES_TASKRUN| msg=exception| reson=%s| mi=%s", exp.reson.c_str(), m_idProfile.c_str());
			IOHand::onClose(flag, p2);
			m_stage = 0;
		}

		return appendTimerq();
	}
	else if (1 == flag)
	{
		onClose(HEFG_PEXIT, 2);
	}

	return ret;
}

// 连接不上或断开连接时到达
int RemoteServ::onClose( int p1, long p2 )
{
	return IOHand::onClose(p1, p2);
}

void RemoteServ::reset( void )
{
	m_stage = 0;
	m_closeReason.clear();
	m_closeFlag = 0;
	IFCLOSEFD(m_cliFd);
}

int RemoteServ::appendTimerq( void )
{
	int ret = 0;
	if (!m_inqueue)
	{
		ret = SwitchHand::Instance()->appendQTask(this, REMOTESERV_EXIST_CHKTIME + s_my_svrid*1000);
		m_inqueue = (0 == ret);
		ERRLOG_IF1(ret, "APPENDQTASK| msg=append fail| ret=%d", ret);
	}
	return ret;
}

int RemoteServ::taskRun( int flag, long p2 )
{
	// 向远端serv发起连接
	// 先检查是否已有同一ID的远端serv,有则无需发起
	string rsvrid = StrParse::Itoa(m_svrid);
	string alias_beg = string(REMOTESERV_ALIAS_PREFIX) + rsvrid + "A"; // "A"是排除serv11进入serv1的范围
	string alias_end = string(REMOTESERV_ALIAS_PREFIX) + rsvrid + "z"; // serv1C serv1S
	int ret = 0;

	CliMgr::AliasCursor finder(alias_beg, alias_end);
	CliBase* serv = NULL;

	LOGDEBUG("REMOTESRUN| %s", CliMgr::Instance()->selfStat(true).c_str());
	if ( (serv = finder.pop()) )
	{
		if (s_my_svrid < m_svrid)
		{
			CliBase* more_serv = finder.pop();
			IOHand* more_ioh = dynamic_cast<IOHand*>(more_serv);
			if (more_ioh)
			{
				more_ioh->driveClose("close more tcplink"); // 关闭多余的一个连接
				LOGINFO("REMOTSERVRUN| msg=remove more tcplink| svrid=%d", m_svrid);
			}
		}
		//LOGDEBUG("REMOTES_RUN| msg=remote serv aliving| svr=%s", serv->m_idProfile.c_str());
	}
	else
	{
		if (0 == m_stage && INVALID_FD != m_cliFd)
		{
			LOGERROR("REMOTES_RUN| msg=unexcepct flow| sock=%d| mi=%s", Sock::geterrno(m_cliFd), m_idProfile.c_str());
			throw OffConnException("cliFd not null at stage0");
		}

		reset();
		ret = Sock::connect(m_cliFd, m_rhost.c_str(), m_port, false);
		if (ERRSOCK_AGAIN == ret || 0 == ret)
		{
			m_epCtrl.setActFd(m_cliFd);
			m_stage = 1; // connecting
			setIntProperty("fd", m_cliFd);
			m_idProfile = StrParse::Format("conneting to %s:%d", m_rhost.c_str(), m_port);

			if (0 == ret)
			{
				m_cliName = Sock::peer_name(m_cliFd, true);
				m_idProfile = m_cliName;
			}

			prepareWhoIam();
			m_epCtrl.setEvt(EPOLLOUT|EPOLLIN, this);
		}
		else
		{
			throw OffConnException("connect fail");
		}
	}

	return ret;
}

// 准备好发送的包,等连接OK后发出
int RemoteServ::prepareWhoIam( void )
{
	string whoIamJson;

	whoIamJson += "{";
	StrParse::PutOneJson(whoIamJson, CONNTERID_KEY, s_my_svrid, true);
	StrParse::PutOneJson(whoIamJson, "svrname", MYSERVNAME, true);
	StrParse::PutOneJson(whoIamJson, CLISOCKET_KEY, Sock::sock_name(m_cliFd, true, false), true);
	StrParse::PutOneJson(whoIamJson, "begin_time", (int)time(NULL), true);
	
	StrParse::PutOneJson(whoIamJson, "pid", getpid(), true);
	StrParse::PutOneJson(whoIamJson, CLIENT_TYPE_KEY, m_cliType, false);

	whoIamJson += "}";

	IOBuffItem* obf = new IOBuffItem;
	obf->setData(CMD_IAMSERV_REQ, ++m_seqid, whoIamJson.c_str(), whoIamJson.length());
	if (!m_oBuffq.append(obf))
	{
		LOGERROR("REMOTES_WAI| msg=append to oBuffq fail| len=%d| mi=%s", m_oBuffq.size(), m_cliName.c_str());
		delete obf;
		throw OffConnException("oBuffq.append fail");
	}
	LOGDEBUG("REMOTES_WAI| msg=send iamserv req| mi=%s", m_cliName.c_str());

	return 0;
}

