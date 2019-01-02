/*-------------------------------------------------------------------------
FileName     : tcpaio_invoker_mgr.h
Description  : invoker_aio.h对象管理者
remark       : 较tcp_invoker_mgr使用io复用
Modification :
--------------------------------------------------------------------------
   1、Date  2019-01-01       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _TCPAIO_INVOKER_MGR_H_
#define _TCPAIO_INVOKER_MGR_H_
#include "comm/public.h"
#include "comm/thread.h"
#include <string>
#include <map>
#include <list>


using namespace std;
class TcpAioInvoker;


class TcpAioInvokerMgr //: public Thread
{
	SINGLETON_CLASS2(TcpAioInvokerMgr)
	TcpAioInvokerMgr( void );
	~TcpAioInvokerMgr( void );

public:
	void init( int epfd );

	// 向服务提供者发出请求，并等待响应回复 （同步）
	int request( string& resp, const string& reqmsg, const string& svrname );
	int requestByHost( string& resp, const string& reqmsg, const string& hostp, int timeout_sec );

private:
	TcpAioInvoker* getInvoker( const string& hostport, int timeout_sec );

	//virtual void* run(void);

private:
	int m_invokerTimOut_sec;
	int m_epfd;
	map< string, TcpAioInvoker* > m_pool;
};

#endif
