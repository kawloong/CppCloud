/*-------------------------------------------------------------------------
FileName     : tcp_invoker_mgr.h
Description  : tcp_invoker.h对象管理者
remark       : 线程安全
Modification :
--------------------------------------------------------------------------
   1、Date  2018-10-29       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _TCP_INVOKER_MGR_H_
#define _TCP_INVOKER_MGR_H_
#include "comm/public.h"
#include <string>
#include <map>
#include <list>


using namespace std;
class TcpInvoker;
typedef map< string, list<TcpInvoker*> > IOVOKER_POOLT;


class TcpInvokerMgr
{
	SINGLETON_CLASS2(TcpInvokerMgr)
	TcpInvokerMgr( void );
	~TcpInvokerMgr( void );

public:
	void setLimitCount( int n );

	// 向服务提供者发出请求，并等待响应回复 （同步）
	int request( string& resp, const string& reqmsg, const string& svrname );
	int requestByHost( string& resp, const string& reqmsg, const string& hostp, int timeout_sec );

private:
	TcpInvoker* getInvoker( const string& hostport, int timeout_sec );
	void relInvoker( TcpInvoker* ivk );

private:
	int m_eachLimitCount;
	IOVOKER_POOLT m_pool;
};

#endif
