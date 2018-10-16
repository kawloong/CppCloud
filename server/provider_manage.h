/*-------------------------------------------------------------------------
FileName     : provider_manage.h
Description  : 服务提供者
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

	static void OnCliCloseHandle( CliBase* cli );
	void onCliCloseHandle( CliBase* cli );

private:
	map<string, ServiceProvider*> m_providers;
};

#endif
