#include "provider_manage.h"
#include "clibase.h"
#include "exception.h"
#include "climanage.h"
#include "homacro.h"
#include "broadcastcli.h"
#include "rapidjson/json.hpp"
#include "comm/strparse.h"
#include "cloud/const.h"

HEPCLASS_IMPL_FUNCX_BEG(ProviderMgr)
HEPCLASS_IMPL_FUNCX_MORE(ProviderMgr, OnCMD_SVRREGISTER_REQ)
HEPCLASS_IMPL_FUNCX_MORE(ProviderMgr, OnCMD_SVRSEARCH_REQ)
HEPCLASS_IMPL_FUNCX_END(ProviderMgr)

ProviderMgr* ProviderMgr::This = NULL;

ProviderMgr::ProviderMgr( void )
{
	CliMgr::Instance()->addCliCloseConsumerFunc(OnCliCloseHandle);
	This = this;
	m_seqid = 0;
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

/** 服务提供者注册服务
 * request: by CMD_SVRREGISTER_REQ
 * format: { "regname": "app1", "svrid": 100, "svrprop": {"okcount": 123, ..} }
 **/
int ProviderMgr::OnCMD_SVRREGISTER_REQ( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false);
	RJSON_GETSTR_D(regname, &doc);
	NormalExceptionOn_IFTRUE(regname.empty(), 400, CMD_SVRREGISTER_RSP, seqid, "leak of regname");
	
	int ret = 0;
	int svrid = 0;
	const Value* svrprop = NULL;
	CliBase* cli = NULL;

	if (0 == Rjson::GetInt(svrid, CONNTERID_KEY, &doc)) // 来自其他Peer-Serv的广播
	{
		cli = CliMgr::Instance()->getChildBySvrid(svrid);
		NormalExceptionOn_IFTRUE(NULL==cli, 400, CMD_SVRREGISTER_RSP, seqid, _F("invalid svrid %d", svrid));
	}
	else // 来自客户应用的上报
	{
		cli = iohand;
		svrid = iohand->getIntProperty(CONNTERID_KEY);
		Rjson::SetObjMember(CONNTERID_KEY, svrid, &doc);
		BroadCastCli::Instance()->toWorld(doc, CMD_SVRREGISTER_REQ, ++This->m_seqid, false);
	}

	if (0 == Rjson::GetObject(&svrprop, "svrprop", &doc) && svrprop)
	{
		Value::ConstMemberIterator itr = svrprop->MemberBegin();
    	for (; itr != svrprop->MemberEnd(); ++itr)
		{
			const char* key = itr->name.GetString();
			const char* val = itr->value.GetString();
			cli->setProperty(regname + ":" + key, val);
		}
	}

	ServiceProvider* provider = This->m_providers[regname];
	if (NULL == provider)
	{
		provider = new ServiceProvider(regname);
		This->m_providers[regname] = provider;
	}

	ret = provider->setItem(cli);
	return ret;
}

// format: {"regname": "app1", version: 1, idc: 1, rack: 2 }
int ProviderMgr::OnCMD_SVRSEARCH_REQ( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false);
	RJSON_GETSTR_D(regname, &doc);
	NormalExceptionOn_IFTRUE(regname.empty(), 400, CMD_SVRSEARCH_RSP, seqid, "leak of regname");
	RJSON_GETINT_D(version, &doc);
	RJSON_GETINT_D(idc, &doc);
	RJSON_GETINT_D(rack, &doc);
	int limit = 10;
	RJSON_GETINT(limit, &doc);

	if (0 == idc)
	{
		idc = iohand->getIntProperty("idc");
		rack = iohand->getIntProperty("rack");
	}

	string resp("{\"data\": ");
	int ret = This->getOneProviderJson(resp, regname, idc, rack, version, limit);
	resp.append(",");
	StrParse::PutOneJson(resp, "count", ret, true);
	StrParse::PutOneJson(resp, "desc", _F("total resp %d providers", ret), true);
	StrParse::PutOneJson(resp, "code", 0, false);
	resp.append("}");

	return iohand->sendData(CMD_SVRSEARCH_RSP, seqid, resp.c_str(), resp.length(), true);
}

// return provider个数
int ProviderMgr::getAllJson( string& strjson ) const
{
	int count = 0;
	strjson.append("{");
	map<string, ServiceProvider*>::const_iterator itr = m_providers.begin();
	for (; itr != m_providers.end(); ++itr)
	{
		strjson += _F("\"%s\":", itr->first.c_str());
		itr->second->getAllJson(strjson);
		++count;
	}
	strjson.append("}");
	return count;
}

// return item个数
int ProviderMgr::getOneProviderJson( string& strjson, const string& svrname ) const
{
	int count = 0;
	strjson.append("{");
	map<string, ServiceProvider*>::const_iterator itr = m_providers.find(svrname);
	if (itr != m_providers.end())
	{
		strjson += "\"svrname\":";
		count = itr->second->getAllJson(strjson);
	}
	strjson.append("}");
	return count;
}
// return item个数
int ProviderMgr::getOneProviderJson( string& strjson, const string& svrname, short idc, short rack, short version, short limit ) const
{
	int count = 0;
	map<string, ServiceProvider*>::const_iterator itr = m_providers.find(svrname);
	if (itr != m_providers.end() && limit > 0)
	{
		count = itr->second->query(strjson, idc, rack, version, limit);
	}
	else
	{
		strjson.append("[]");
	}

	return count;
}