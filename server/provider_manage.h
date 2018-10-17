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

	static void OnCliCloseHandle( CliBase* cli );
	void onCliCloseHandle( CliBase* cli );

private:
	int getAllJson( string& strjson ) const;
	int getOneProviderJson( string& strjson, const string& svrname ) const;
	int getOneProviderJson( string& strjson, const string& svrname, short idc, short rack, short version, short limit ) const;

private:
	map<string, ServiceProvider*> m_providers;
	int m_seqid;
	static ProviderMgr* This;
};

#endif
