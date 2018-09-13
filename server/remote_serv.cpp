#include <sys/epoll.h>
#include "remote_serv.h"
#include "comm/strparse.h"
#include "comm/sock.h"
#include "cloud/const.h"
#include "exception.h"
#include "climanage.h"
#include "flowctrl.h"


HEPMUTICLASS_IMPL(RemoteServ, RemoteServ, IOHand)

int RemoteServ::s_my_svrid = 0;

RemoteServ::RemoteServ(void): m_stage(0), m_seqid(0), m_svrid(0), m_epfd(INVALID_FD), m_port(0)
{
 	m_cliType = SERV_CLITYPE_ID;
	m_isLocal = true;
	m_outObj = true;
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

	m_idProfile = "conn2" + rhost;
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
	if (0 == flag)
	{
		try {
			return taskRun(flag, p2);
		}
		catch(OffConnException& exp)
		{
			LOGERROR("REMOTES_TASKRUN| msg=exception| reson=%s", exp.reson.c_str(), m_idProfile.c_str());
			IOHand::onClose(flag, p2);
			m_stage = 0;
		}
		return -12;
	}
	else if (1 == flag)
	{
		onClose(HEFG_PEXIT, 2);
	}

	return 0;
}

// 连接不上或断开连接时到达
int RemoteServ::onClose( int p1, long p2 )
{
	return IOHand::onClose(p1, p2);
}

int RemoteServ::taskRun( int flag, long p2 )
{
	// 向远端serv发起连接
	// 先检查是否已有同一ID的远端serv,有则无需发起
	string rsvrid = getProperty(CONNTERID_KEY);
	string alias = string(REMOTESERV_ALIAS_PREFIX) + rsvrid;
	int ret = 0;

	if (CliMgr::Instance()->getChildByName(alias))
	{
		LOGDEBUG("REMOTES_RUN| msg=remote serv exist| svr=%s", alias.c_str());
		FlowCtrl::Instance()->appendTask(this, 0, REMOTESERV_EXIST_CHKTIME);
	}
	else
	{
		if (0 == m_stage && INVALID_FD != m_cliFd)
		{
			LOGERROR("REMOTES_RUN| msg=unexcepct flow| sock=%d| mi=%s", Sock::geterrno(m_cliFd), m_idProfile.c_str());
			throw OffConnException("cliFd not null at stage0");
		}

		ret = Sock::connect(m_cliFd, m_rhost.c_str(), m_port, false);
		if (ERRSOCK_AGAIN == ret || 0 == ret)
		{
			m_epCtrl.setActFd(m_cliFd);
			m_stage = 1; // connecting
			m_cliName = Sock::peer_name(m_cliFd, true);
			m_idProfile = m_cliName;
			prepareWhoIam();

			m_epCtrl.setEvt(EPOLLOUT, this);
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

	return 0;
}

