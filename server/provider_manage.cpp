#include "provider_manage.h"
#include "clibase.h"
#include "climanage.h"


ProviderMgr::ProviderMgr( void )
{
	CliMgr::Instance()->addCliCloseConsumerFunc(OnCliCloseHandle);
}

ProviderMgr::~ProviderMgr( void )
{
	map<string, ServiceProvider*>::iterator itr =  m_providers.begin();
	for (; itr != m_providers.end(); ++itr)
	{
		ServiceProvider* second = itr->second;
		IFDELETE(second);
	}
	m_providers.clear();
}

void ProviderMgr::OnCliCloseHandle( CliBase* cli )
{
	ProviderMgr::Instance()->onCliCloseHandle(cli);
}

void ProviderMgr::onCliCloseHandle( CliBase* cli )
{
	if (cli->getCliType() > 1)
	{
		map<string, ServiceProvider*>::iterator itr = m_providers.begin();
		for (; itr != m_providers.end(); ++itr)
		{
			ServiceProvider* second = itr->second;
			second->removeItme(cli);
		}
	}
}