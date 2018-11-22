/*-------------------------------------------------------------------------
FileName     : provider_manage.h
Description  : 全局服务提供者管理类
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-10-16       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _PROVIDER_MANAGE_H_
#define _PROVIDER_MANAGE_H_
#include <string>
#include <map>
#include "comm/public.h"
#include "service_provider.h"

using namespace std;
class CliBase;

class ProviderMgr
{
	SINGLETON_CLASS2(ProviderMgr)

	ProviderMgr( void );
	~ProviderMgr( void );

public:
	static int OnCMD_SVRREGISTER_REQ( void* ptr, unsigned cmdid, void* param ); // 服务注册/更新
	static int OnCMD_SVRSEARCH_REQ( void* ptr, unsigned cmdid, void* param ); // 服务发现
	static int OnCMD_SVRSHOW_REQ( void* ptr, unsigned cmdid, void* param ); // 服务展示
	static int OnCMD_SVRSTAT_REQ( void* ptr, unsigned cmdid, void* param ); // 服务统计上报

	static void OnCliCloseHandle( CliBase* cli );
	void onCliCloseHandle( CliBase* cli );
	void updateProvider( CliBase* cli,  const string& regname, int prvdid );

private:
	ServiceProvider* getProviderPtr( const string& regname ) const;
	int getAllJson( string& strjson ) const;
	int getOneProviderJson( string& strjson, const string& regname ) const;
	int getOneProviderJson( string& strjson, const string& regname, short idc, short rack, short version, short limit ) const;

	// 服务提供者退出或禁用，通知各订阅过服务的消费者
	void notify2Invoker( const string& regname, int svrid, int prvdid );
	// 注册或设备服务提供者的属性
	int setProviderProperty( CliBase* cli, const void* doc, const string& regname );

private:
	map<string, ServiceProvider*> m_providers;
	int m_seqid;
	static ProviderMgr* This;
};

#endif
