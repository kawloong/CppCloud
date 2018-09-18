#include "remote_mgr.h"

#include "comm/strparse.h"
#include "switchhand.h"
#include "cppcloud_config.h"
#include "remote_serv.h"
#include "remote_cli.h"
#include "climanage.h"

RemoteMgr::RemoteMgr()
{
	m_mysvrid = 0;
	m_epfd = 0;
}

// 读出配置文件, 构造出连接PeerServ的对象
int RemoteMgr::init( int epfd )
{
    string peernodes = CloudConf::CppCloudPeerNode();
    vector<string> vhost;
    StrParse::SpliteStr(vhost, peernodes, '|');

	m_epfd = epfd;
    m_mysvrid = CloudConf::CppCloudServID();
    RemoteServ::Init(m_mysvrid);
	RemoteCli::Init(m_mysvrid);

	int ret = 0;
    vector<string>::const_iterator itr = vhost.begin();
    for (; itr != vhost.end(); ++itr)
    {
		vector<string> vec;
		int ret = StrParse::SpliteStr(vec, *itr, ':');
		ERRLOG_IF1RET_N(ret || vec.size() < 2, -4, 
			"REMOTEMGR_INIT| msg=invalid host config| config=%s", peernodes.c_str());
		
		RemoteServ* ptr = new RemoteServ;
		ptr->init(vec[0], atoi(vec[1].c_str()), m_epfd);
		BindSon(this, ptr); // 使得onEvent()可以接收关闭通知
		m_rSvrs[ptr] = 0;

		ret = SwitchHand::Instance()->appendQTask(ptr, m_mysvrid*1000); // 在$[m_mysvrid]秒后触发主动连接
		IFBREAK(ret);
    }

	return ret;
}

int RemoteMgr::onEvent( int evtype, va_list ap )
{
	int ret = 0;
	if (HEPNTF_SOCK_CLOSE == evtype)
	{
		IOHand* son = va_arg(ap, IOHand*);
		int clitype = va_arg(ap, int);
		int isExit = va_arg(ap, int);

		RemoteServ* ptr = dynamic_cast<RemoteServ*>(son);
		ERRLOG_IF1RET_N(NULL==ptr, -45, "REMOTE_SOCKCLOSE| msg=not remoteserv pointer found| "
		 	"clitype=%d, son=%p", clitype, son);

		CliMgr::Instance()->removeAliasChild(son, true);

		if (isExit)
		{
			delete ptr; // m_outObj = true
			m_rSvrs.erase(ptr);
		}
		else
		{
			ptr->appendTimerq();
		}
	}
	
	return ret;
}

void RemoteMgr::uninit( void )
{
	map<RemoteServ*, int>::iterator itr_;
	map<RemoteServ*, int>::iterator itr = m_rSvrs.begin();
	for (; itr != m_rSvrs.end(); )
	{
		itr_ = itr++;
		itr_->first->driveClose();
		// delete itr->first; // 上一句会调用到this->ononEvent() HEPNTF_SOCK_CLOSE
	}

	m_rSvrs.clear();
}