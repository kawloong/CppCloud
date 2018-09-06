#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <cerrno>
#include <cstring>
#include "listen.h"
#include "comm/sock.h"


HEPCLASS_IMPL(Listen, Lisn)

Listen::Listen(void)
{
    m_listenFd = INVALID_FD;
	m_waitRmPtr = NULL;
	m_workClassName = "IOHand";
}
Listen::~Listen(void)
{
	IFDELETE(m_waitRmPtr);
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
			HEpBase* worker = HEpBase::New(m_workClassName.c_str());
			IFBREAK_N(NULL == worker, -40);
			
			HEpBase::BindSon(this, worker);
			// ... //
			Notify(worker, HEPNTF_INIT_PARAM, ret, m_evCtrl.m_epfd);
			m_child = NULL; // Listen类的子对象存于map中，而非m_child
			CliInfo& cliinfo = m_children[worker];
			cliinfo.t0 = time(NULL);
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
		
		ret = progExitHanele(0);
	}
	
	return ret;
}

// 通告事件处理
int Listen::onEvent( int evtype, va_list ap )
{
	int ret = 0;
	if (HEPNTF_INIT_PARAM == evtype)
	{
		int port = va_arg(ap, int);
		const char* host = va_arg(ap, const char*);
		int lqueue = va_arg(ap, int);
		int epfd = va_arg(ap, int);
		
		m_evCtrl.setEPfd(epfd);
		ret = creatSock(port, host, lqueue);
		IFRETURN_N(ret, ret);
		ret = m_evCtrl.setEvt(EPOLLIN, this);
	}
	else if (HEPNTF_SOCK_CLOSE == evtype)
	{
		HEpBase* son = va_arg(ap, HEpBase*);

		removeAliasChild(son, true);
	}
	else if (HEFG_PROGEXIT == evtype) /// #PROG_EXITFLOW(3)
	{
		IFCLOSEFD(m_listenFd);
	}
	
	return ret;
}

int Listen::addAlias2Child( const string& asname, HEpBase* ptr )
{
	int ret = 0;

	map<HEpBase*, CliInfo>::iterator it = m_children.find(ptr);
	if (m_children.end() == it) // 非直接子对象，暂时不处理
	{
		LOGWARN("ADDALIASCHILD| msg=ptr isnot listen's child| name=%s", asname.c_str());
		return -1;
	}

	it->second.aliasName[asname] = true;

	HEpBase*& secondVal = m_aliName2Child[asname];
	if (NULL != secondVal && ptr != secondVal) // 已存在旧引用的情况下，要先清理掉旧的设置
	{
		LOGWARN("ADDALIASCHILD| msg=alias child name has exist| name=%s", asname.c_str());
		map<HEpBase*, CliInfo>::iterator itr = m_children.find(secondVal);
		if (m_children.end() != itr)
		{
			itr->second.aliasName.erase(asname);
		}
		
		ret = 1;
	}
	
	secondVal = ptr;

	return ret;
}
void Listen::removeAliasChild( const string& asname )
{
	map<string, HEpBase*>::iterator it = m_aliName2Child.find(asname);
	if (m_aliName2Child.end() != it)
	{
		map<HEpBase*, CliInfo>::iterator itr = m_children.find(it->second);
		if (m_children.end() != itr)
		{
			itr->second.aliasName.erase(asname);
		}

		m_aliName2Child.erase(it);
	}
}
void Listen::removeAliasChild( HEpBase* ptr, bool rmAll )
{
	map<HEpBase*, CliInfo>::iterator it = m_children.find(ptr);
	if (it != m_children.end()) // 移除所有别名引用
	{
		string asnamestr;
		CliInfo& cliinfo = it->second;
		for (map<string, bool>::iterator itr = cliinfo.aliasName.begin(); itr != cliinfo.aliasName.end(); ++itr)
		{
			asnamestr.append( asnamestr.empty()?"":"&" ).append(itr->first);
			m_aliName2Child.erase(itr->first);
		}

		cliinfo.aliasName.clear();

		if (rmAll) // 移除m_children指针
		{
			m_children.erase(it);
			if (ptr != m_waitRmPtr)
			{
				IFDELETE(m_waitRmPtr); // 清理前一待删对象(为避免同步递归调用)
			}

			LOGINFO("LISTEN_CHILDRM| msg=a iohand close| dt=%ds| asname=%s", int(time(NULL)-cliinfo.t0), asnamestr.c_str());
			m_waitRmPtr = ptr;
		}
	}
}

// ep线程调用此方法通知各子对象退出
int Listen::progExitHanele( int flg )
{
	map<HEpBase*, CliInfo>::iterator it = m_children.begin();
	for (; it != m_children.end(); ++it)
	{
		it->first->run(HEFG_PEXIT, 2); /// #PROG_EXITFLOW(5)
	}

	return 0;
}