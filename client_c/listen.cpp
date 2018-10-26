#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <cerrno>
#include <cstring>
#include "listen.h"
#include "comm/sock.h"
#include "climanage.h"


HEPCLASS_IMPL(Listen, Lisn)

Listen::Listen(void)
{
    m_listenFd = INVALID_FD;
	m_workClassName = "IOHand";
}
Listen::~Listen(void)
{
}

int Listen::init( const string& host, int port, int lqueue, int epfd )
{
	int ret;
	m_evCtrl.setEPfd(epfd);
	ret = creatSock(port, host.c_str(), lqueue);
	IFRETURN_N(ret, ret);
	ret = m_evCtrl.setEvt(EPOLLIN, this);
	
	LOGINFO("LISTEN| msg=listen at %s:%d| listenfd=%d| epfd=%d| evflag=%d", 
		host.c_str(), port, m_listenFd, m_evCtrl.m_epfd, m_evCtrl.m_eventFg);
	return ret;
}

void Listen::uninit( void )
{
	m_evCtrl.setEvt(0, 0);
	m_evCtrl.setActFd(INVALID_FD);
	LOGINFO("LISTENRUN| msg=listenfd close| listenfd=%d", m_listenFd);
	IFCLOSEFD(m_listenFd);
}

int Listen::creatSock( int port, const char* svrhost/*= NULL*/, int lqueue/*=100*/ )
{
	int ret = 0;
	do 
	{
		IFBREAK_N(INVALID_FD != m_listenFd, -20);
		m_listenFd = Sock::create_fd(svrhost, port);
		
		m_evCtrl.setActFd(m_listenFd);
		IFBREAK_N(ret, -21);
		printf("Serv Listen on %s:%d\n", svrhost, port);
	}
	while (0);
	
	return 0;
}

// ep线程可读写通知
int Listen::run( int p1, long p2 )
{
	int ret = 0;

	if (EPOLLIN & p1)
	{
		struct sockaddr_in client;
		socklen_t len = sizeof(client);
		
		while ( (ret = accept(m_listenFd, (struct sockaddr*)&client, &len)) >= 0)
		{
			int clifd = ret;
			CliMgr::Instance()->newChild(clifd, m_evCtrl.m_epfd);
		}
		
		if ( -1 == ret )
		{
			if (EAGAIN == errno || EWOULDBLOCK == errno) {}
			else
			{
				LOGERROR("LISTENRUN| msg=accept fail (%d)| ret=%d| errmsg=%s", errno, ret, strerror(errno));
			}
		}
	}
	else
	{
		// maybe program exit /// #PROG_EXITFLOW(4)
		m_evCtrl.setEvt(0, 0);
		m_evCtrl.setActFd(INVALID_FD);
		IFCLOSEFD(m_listenFd);
		LOGINFO("LISTENRUN| msg=listenfd close| p1=%d", p1);
		
		ret = CliMgr::Instance()->progExitHanele(0);
	}
	
	return ret;
}


// 通告事件处理
int Listen::onEvent( int evtype, va_list ap )
{
	//if (HEPNTF_INIT_PARAM == evtype) // change to init()

	//else if (HEFG_PROGEXIT == evtype) /// #PROG_EXITFLOW(3) // change to uninit()

	return 0;
}
