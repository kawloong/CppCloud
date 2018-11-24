#include "service_provider.h"
#include "clibase.h"
#include "rapidjson/json.hpp"
#include "cloud/const.h"
#include "comm/strparse.h"

ServiceItem::ServiceItem( void ): ivk_ok(0), ivk_ng(0)
{

}

int ServiceItem::parse0( const string& regname, CliBase* cli, int prvdid )
{
	svrid = cli->getIntProperty("svrid");
	ERRLOG_IF1RET_N(regname.empty() || svrid <= 0, -70, 
		"SERVITEMPARSE| msg=parse fail| cli=%s", cli->m_idProfile.c_str());

	this->regname = regname;
	this->regname2 = _F("%s%s%%%d", SVRPROP_PREFIX, regname.c_str(), prvdid);
	this->prvdid = prvdid;
	idc = cli->getIntProperty("idc");
	rack = cli->getIntProperty("rack");
	islocal = cli->isLocal();
	return 0;
}

int ServiceItem::parse( CliBase* cli )
{
	// 固定部份-不会随运行过程改变的
	if (0 == version || 0 == protocol || url.empty())
	{
		url = cli->getProperty(regname2 + ":url");
		protocol = cli->getIntProperty(regname2 + ":protocol");
		version = cli->getIntProperty(regname2 + ":version");
	}

	// 变化部分
	desc = cli->getProperty(regname2 + ":desc");
	//okcount = cli->getIntProperty(regname2 + ":okcount");
	//ngcount = cli->getIntProperty(regname2 + ":ngcount");
	weight = cli->getIntProperty(regname2 + ":weight");
	enable = cli->getIntProperty(regname2 + ":enable");

	if (prvdid > 0)
	{
		int ntmp = 0;
		if ( (ntmp = cli->getIntProperty(regname2 + "idc")) )
		{
			idc = ntmp;
		}
		if ( (ntmp = cli->getIntProperty(regname2 + "rack")) )
		{
			rack = ntmp;
		}
	}

	return 0;
}

void ServiceItem::getJsonStr( string& strjson, int oweight ) const
{
	strjson.append("{");
	StrParse::PutOneJson(strjson, "regname", regname, true);
	StrParse::PutOneJson(strjson, "url", url, true);
	StrParse::PutOneJson(strjson, "desc", desc, true);
	StrParse::PutOneJson(strjson, "svrid", svrid, true);
	StrParse::PutOneJson(strjson, "prvdid", prvdid, true);
	StrParse::PutOneJson(strjson, "okcount", okcount, true);
	StrParse::PutOneJson(strjson, "ngcount", ngcount, true);
	StrParse::PutOneJson(strjson, "protocol", protocol, true);
	StrParse::PutOneJson(strjson, "version", version, true);
	// 权重，当服务消费应用调用时，weight=匹配分数值
	StrParse::PutOneJson(strjson, "weight", oweight>0? oweight: weight, true);
	StrParse::PutOneJson(strjson, "idc", idc, true);
	StrParse::PutOneJson(strjson, "rack", rack, true);
	StrParse::PutOneJson(strjson, "enable", enable, false);
	strjson.append("}");
}

void ServiceItem::getCalcJson( string& strjson, int oweight ) const
{
	getJsonStr(strjson, oweight);
}

int ServiceItem::score( short oidc, short orack ) const
{
	static const int match_mult1 = 4;
	static const int match_mult2 = 8;
	int score = 0;
	int calc_weight = this->weight>0? this->weight : 1;

	if (oidc > 0 && oidc == this->idc) // 同一机房
	{
		score += (orack > 0 && orack == this->rack)? calc_weight*match_mult2 : calc_weight*match_mult1;
	}

	if (this->islocal) // 优先同一注册中心的服务
	{
		score += calc_weight;
	}

	score += calc_weight;
	score += (rand()&0x7);
	return score;
}

ServiceProvider::ServiceProvider( const string& svrName ): m_regName(svrName)
{

}

ServiceProvider::~ServiceProvider( void )
{
	map<CliBase*, SVRITEM_MAP>::iterator itr = m_svrItems.begin();
	for (; itr != m_svrItems.end(); ++itr)
	{
		//CliBase* first = itr->first;
		for (auto iit : itr->second)
		{
			IFDELETE(iit.second);
		}
	}
}

int ServiceProvider::setItem( CliBase* cli, int prvdid )
{
	ServiceItem*& pitem = m_svrItems[cli][prvdid];

	if (NULL == pitem)
	{
		string regname2 = _F("%s%s%%%d", SVRPROP_PREFIX, m_regName.c_str(), prvdid);

		pitem = new ServiceItem;
		int ret = pitem->parse0(m_regName, cli, prvdid);

		if (ret)
		{
			IFDELETE(pitem);
			m_svrItems[cli].erase(prvdid);
			return ret;
		}

		// 设置属性标记，以便在广播给其他serv时能够还原提供的服务
		string provval = cli->getProperty(SVRPROVIDER_CLI_KEY);
		cli->setProperty( SVRPROVIDER_CLI_KEY, 
			provval.empty()? regname2: (regname2 + "+" + provval) );
	}
	
	return  pitem->parse(cli);
}

void ServiceProvider::setStat( CliBase* cli, int prvdid, int pvd_ok, int pvd_ng, int ivk_dok, int ivk_dng )
{
	auto itr = m_svrItems.find(cli);
	if (itr != m_svrItems.end())
	{
		auto itr2 = itr->second.find(prvdid);
		if ( itr2 != itr->second.end() )
		{
			ServiceItem* itm = itr2->second;
			if (pvd_ok > 0) itm->okcount = pvd_ok;
			if (pvd_ng > 0) itm->ngcount = pvd_ng;
			if (ivk_dok > 0) itm->ivk_ok += ivk_dok;
			if (ivk_dng > 0) itm->ivk_ng += ivk_dng;
		}
	}
}

bool ServiceProvider::removeItme( CliBase* cli )
{
	bool ret = false;
	auto itr = m_svrItems.find(cli);
	if (itr != m_svrItems.end())
	{
		for (auto itMap : itr->second)
		{
			IFDELETE(itMap.second);
		}
		
		m_svrItems.erase(itr);
		ret = true;
	}

	return ret;
}

// 返回json-array[]形式
int ServiceProvider::getAllJson( string& strjson ) const
{
	strjson.append("[");
	int i = 0;

	for (auto itr : m_svrItems)
	{
		for (auto itMap : itr.second)
		{
			if (i > 0) strjson.append(",");
			itMap.second->getJsonStr(strjson);
		}
	}

	strjson.append("]");
	return i;
}

/**
 * @summery: 消息者查询可用服务列表
 * @remark: 调用时采用客户端负载均衡方式
 * @return: 返回可用服务个数
 * @param: strjson [out] 返回json字符串[array格式]
 **/
int ServiceProvider::query( string& strjson, short idc, short rack, short version, short limit ) const
{
	// sort all item by score
	map<int, ServiceItem*> sortItemMap;
	map<CliBase*, SVRITEM_MAP>::const_iterator itr = m_svrItems.begin();
	for (; itr != m_svrItems.end(); ++itr)
	{
		for (auto itMap : itr->second)
		{
			ServiceItem* ptr = itMap.second;
			if (!ptr->valid()) continue;
			if (version > 0 && version != ptr->version) continue;

			int score = ptr->score(idc, rack);
			ptr->tmpnum = score;
			while (NULL != sortItemMap[score])
			{
				score++;
			}
			sortItemMap[score] = ptr;			
		}

	}
	
	short count = 0;
	strjson.append("[");
	map<int, ServiceItem*>::reverse_iterator ritr = sortItemMap.rbegin();
	for (; count < limit && sortItemMap.rend() != ritr; ++ritr, ++count)
	{
		ServiceItem* ptr = ritr->second;
		if (count > 0) strjson.append(",");
		ptr->getCalcJson(strjson, ptr->tmpnum);
	}
	strjson.append("]");
	
	return count;
}