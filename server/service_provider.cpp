#include "service_provider.h"
#include "clibase.h"
#include "rapidjson/json.hpp"

ServiceItem::ServiceItem( void ): svrid(0), okcount(0), ngcount(0),
	 protocol(0),version(0), weight(0), idc(0), rack(0), enable(false)
{

}

bool ServiceItem::valid( void )
{
	return enable && protocol > 0 && svrid > 0 && weight > 0 && !url.empty();
}

int ServiceItem::parse0( const string& svrname, CliBase* cli )
{
	svrid = cli->getIntProperty("svrid");
	ERRLOG_IF1RET_N(svrname.empty() || svrid <= 0, -70, "SERVITEMPARSE| msg=parse fail| cli=%s", cli->m_idProfile.c_str());

	this->svrname = svrname;
	idc = cli->getIntProperty("idc");
	rack = cli->getIntProperty("rack");
	return 0;
}

int ServiceItem::parse( CliBase* cli )
{
	// 固定部份-不会随运行过程改变的
	if (0 == version || 0 == protocol || url.empty())
	{
		url = cli->getProperty(svrname + ":url");
		protocol = cli->getIntProperty(svrname + ":protocol");
		version = cli->getIntProperty(svrname + ":version");		
	}

	// 变化部分
	desc = cli->getProperty(svrname + ":desc");
	okcount = cli->getIntProperty(svrname + ":okcount");
	ngcount = cli->getIntProperty(svrname + ":ngcount");
	weight = cli->getIntProperty(svrname + ":weight");
	enable = cli->getIntProperty(svrname + ":enable");
	return 0;
}

ServiceProvider::ServiceProvider( const string& svrName ): m_svrName(svrName)
{

}

ServiceProvider::~ServiceProvider( void )
{
	map<CliBase*, ServiceItem*>::iterator itr = m_svrItems.begin();
	for (; itr != m_svrItems.end(); ++itr)
	{
		//CliBase* first = itr->first;
		ServiceItem* second = itr->second;
		IFDELETE(second);
	}
}

int ServiceProvider::setItem( CliBase* cli )
{
	ServiceItem* pitem = NULL;
	map<CliBase*, ServiceItem*>::iterator itr = m_svrItems.find(cli);
	if (m_svrItems.end() == itr)
	{
		pitem = new ServiceItem;
		int ret = pitem->parse0(m_svrName, cli);
		if (ret)
		{
			IFDELETE(pitem);
			return ret;
		}
		m_svrItems[cli] = pitem;
	}
	else
	{
		pitem = itr->second;
	}
	
	return  pitem->parse(cli);
}

void ServiceProvider::removeItme( CliBase* cli )
{
	map<CliBase*, ServiceItem*>::iterator itr = m_svrItems.find(cli);
	if (itr != m_svrItems.end())
	{
		IFDELETE(itr->second);
		m_svrItems.erase(itr);
	}
}