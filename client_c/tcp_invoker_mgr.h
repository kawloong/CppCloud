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

private:
	TcpInvoker* getInvoker( void );

private:
	int m_eachLimitCount;
	int m_invokerTimOut_sec;
	IOVOKER_POOLT m_pool;
};

#endif
