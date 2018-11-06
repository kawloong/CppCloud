/*-------------------------------------------------------------------------
FileName     : shttp_invoker_mgr.h
Description  : 简单http协议调用
remark       : 线程安全
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-06       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _SHTTP_INVOKER_MGR_H_
#define _SHTTP_INVOKER_MGR_H_
#include "comm/public.h"
#include <string>


using namespace std;

class SHttpInvokerMgr
{
	SINGLETON_CLASS2(SHttpInvokerMgr)
	SHttpInvokerMgr( void );
	~SHttpInvokerMgr( void );

public:
	void setLimitCount( int n );

	// 向服务提供者发出请求，并等待响应回复 （同步）
	int get( string& resp, const string& reqmsg, const string& svrname );
	int post( string& resp, const string& reqmsg, const string& svrname );



private:
	int m_eachLimitCount;
	int m_invokerTimOut_sec;
};

#endif
