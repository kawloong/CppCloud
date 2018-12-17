#include "provider_manage.h"
#include "clibase.h"
#include "cloud/exception.h"
#include "climanage.h"
#include "cloud/homacro.h"
#include "broadcastcli.h"
#include "rapidjson/json.hpp"
#include "comm/strparse.h"
#include "cloud/const.h"

HEPCLASS_IMPL_FUNCX_BEG(ProviderMgr)
HEPCLASS_IMPL_FUNCX_MORE(ProviderMgr, OnCMD_SVRREGISTER_REQ)
HEPCLASS_IMPL_FUNCX_MORE(ProviderMgr, OnCMD_SVRSEARCH_REQ)
HEPCLASS_IMPL_FUNCX_MORE(ProviderMgr, OnCMD_SVRSHOW_REQ)
HEPCLASS_IMPL_FUNCX_MORE(ProviderMgr, OnCMD_SVRSTAT_REQ)
HEPCLASS_IMPL_FUNCX_END(ProviderMgr)

ProviderMgr* ProviderMgr::This = ProviderMgr::Instance();

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

// 接收所有客户应用断开连接通知
void ProviderMgr::onCliCloseHandle( CliBase* cli )
{
	if (cli->getCliType() > 1)
	{
		map<string, ServiceProvider*>::iterator itr = m_providers.begin();
		for (; itr != m_providers.end(); ++itr)
		{
			ServiceProvider* second = itr->second;
			bool exist = second->removeItme(cli);
			if (exist)
			{
				notify2Invoker(itr->first, cli->getIntProperty(CONNTERID_KEY), 0);
			}
		}
	}
}

void ProviderMgr::notify2Invoker( const string& regname, int svrid, int prvdid )
{
	string preKey(SVRBOOKCH_ALIAS_PREFIX);
	preKey += "_" + regname + "@";
	CliMgr::AliasCursor finder(preKey);
	CliBase *invokerCli = NULL;
	string msg;
	while ((invokerCli = finder.pop()))
	{
		IOHand *invokerIO = dynamic_cast<IOHand *>(invokerCli);
		if (NULL == invokerIO) continue;
		if (msg.empty())
		{
			msg = _F("{\"notify\": \"provider_down\", \"svrid\": %d, \"prvdid\": %d, \"regname\": \"%s\"}",
					 svrid, prvdid, regname.c_str());
		}

		int ret = invokerIO->sendData(CMD_EVNOTIFY_REQ, ++m_seqid, msg.c_str(), msg.length(), true);
		LOGDEBUG("NOTIFYINVOKER| msg=notify provider_down to %d| ret=%d| msg=%s",
				 invokerCli->getIntProperty(CONNTERID_KEY), ret, msg.c_str());
	}
}

/** 服务提供者注册服务
 * request: by CMD_SVRREGISTER_REQ CMD_SVRREGISTER2_REQ
 * format: { "regname": "app1", "svrid": 100, 
 * 		"svrprop": {"prvdid":1, "url": "tcp://x", ..} }
 **/
int ProviderMgr::OnCMD_SVRREGISTER_REQ( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false);
	RJSON_GETSTR_D(regname, &doc);
	NormalExceptionOn_IFTRUE(regname.empty(), 400, CMD_SVRREGISTER_RSP, seqid, "leak of regname");
	
	int ret = 0;
	int svrid = 0;
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
		BroadCastCli::Instance()->toWorld(doc, CMD_SVRREGISTER2_REQ, ++This->m_seqid, false);
	}

	int prvdid = 0;
	Rjson::GetInt(prvdid, SVRREG_PROP_KEY, "prvdid", &doc);

	string regname2 = _F("%s%s%%%d", SVRPROP_PREFIX, regname.c_str(), prvdid);

	int urlChange = CheckValidUrlProtocol(cli, &doc, regname2, seqid);
	int enableBeforValue = cli->getIntProperty(regname2 + ":enable");
	ret = This->setProviderProperty(cli, &doc, regname2);
	int enableAfterValue = cli->getIntProperty(regname2 + ":enable");
	bool enableChange = (1 == enableBeforValue && 0 == enableAfterValue);

	if (0 == ret && (urlChange || enableChange)) // 禁用服务时触发
	{
		// 通知各个订阅者
		This->notify2Invoker(regname, svrid, prvdid);
	}

	This->updateProvider(cli, regname, prvdid);
	string resp = _F("{\"code\": 0, \"desc\": \"reg %s result %d\"}", regname2.c_str(), ret);
	iohand->sendData(CMD_SVRREGISTER_RSP, seqid, resp.c_str(), resp.length(), true);
	return ret;
}

void ProviderMgr::updateProvider( CliBase* cli,  const string& regname, int prvdid)
{
	ServiceProvider* provider = m_providers[regname];
	if (NULL == provider)
	{
		provider = new ServiceProvider(regname);
		m_providers[regname] = provider;
	}

	provider->setItem(cli, prvdid);
}

// url&protocol合法性检查
int ProviderMgr::CheckValidUrlProtocol( CliBase* cli, const void* doc, const string& regname2, unsigned seqid ) 
{
	string url1;
	string url0 = cli->getProperty(regname2 + ":url");
	int protocol1 = 0;
	int ret = 0;
	
	if (0 != Rjson::GetStr(url1, SVRREG_PROP_KEY, "url", (const Document*)doc))
	{
		url1 = url0;
	}
	else
	{
		if (!url0.empty() && !url1.empty() && url1 != url0)
		{
			ret = 1;
		}
	}
	if (0 != Rjson::GetInt(protocol1, SVRREG_PROP_KEY, "protocol", (const Document*)doc))
	{
		protocol1 = cli->getIntProperty(regname2 + ":protocol");
	}

	if (!url1.empty() && 0 != protocol1)
	{
		static const int protocolLen = 4;
		static const string urlPrefix[protocolLen+1] = {"tcp", "udp", "http", "https", "x"};
		NormalExceptionOn_IFTRUE(protocol1 > protocolLen || protocol1 <= 0, 400, 
				CMD_SVRREGISTER_RSP, seqid, _F("invalid protocol %d", protocol1));
	 	NormalExceptionOn_IFTRUE(0 != url1.find(urlPrefix[protocol1-1]), 400, 
				CMD_SVRREGISTER_RSP, seqid, "url protocol not match");
	}

	return ret;
}

int ProviderMgr::setProviderProperty( CliBase* cli, const void* doc, const string& regname2 )
{
	int ret = -1;
	const Value* svrprop = NULL;
	if (0 == Rjson::GetObject(&svrprop, SVRREG_PROP_KEY, (const Document*)doc) && svrprop)
	{
		Value::ConstMemberIterator itr = svrprop->MemberBegin();
    	for (; itr != svrprop->MemberEnd(); ++itr)
		{
			const char* key = itr->name.GetString();
			if (itr->value.IsString())
			{
				const char* val = itr->value.GetString();
				cli->setProperty(regname2 + ":" + key, val);
			}
			else if (itr->value.IsInt())
			{
				string val = StrParse::Itoa(itr->value.GetInt());
				cli->setProperty(regname2 + ":" + key, val);
			}
			else if (itr->value.IsBool()) // true -> "1", false -> "0"
			{
				cli->setProperty(regname2 + ":" + key, itr->value.IsTrue()? "1": "0");
			}
		}
		ret = 0;
	}

	return ret;
}

// format: {"regname": "app1", version: 1, idc: 1, rack: 2, bookchange: 1 }
int ProviderMgr::OnCMD_SVRSEARCH_REQ( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false);
	RJSON_GETSTR_D(regname, &doc);
	NormalExceptionOn_IFTRUE(regname.empty(), 400, CMD_SVRSEARCH_RSP, seqid, "leak of regname");
	RJSON_GETINT_D(version, &doc);
	RJSON_GETINT_D(idc, &doc);
	RJSON_GETINT_D(rack, &doc);
	RJSON_GETINT_D(bookchange, &doc);
	int limit = 4;
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

	if (bookchange) // 订阅改变事件（cli下线）
	{
		string alias = string(SVRBOOKCH_ALIAS_PREFIX) + "_" + regname + "@" + iohand->getProperty(CONNTERID_KEY);
		CliMgr::Instance()->addAlias2Child(alias, iohand);
	}

	return iohand->sendData(CMD_SVRSEARCH_RSP, seqid, resp.c_str(), resp.length(), true);
}

// format: {"regname": "all"}
int ProviderMgr::OnCMD_SVRSHOW_REQ( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false);
	RJSON_GETSTR_D(regname, &doc);
	//RJSON_GETINT_D(onlyname, &doc);

	bool ball = (regname.empty() || "all" == regname);
	int count = 0;
	string resp("{");

	resp.append("\"data\":");
	count = ball? This->getAllJson(resp) : This->getOneProviderJson(resp, regname);
	resp.append(",");
	StrParse::PutOneJson(resp, "len", count, false);
	resp.append("}");

	return iohand->sendData(CMD_SVRSHOW_RSP, seqid, resp.c_str(), resp.length(), true);
}

// format: [{"prvdid": 1, "pvd_ok": 10}]
int ProviderMgr::OnCMD_SVRSTAT_REQ( void* ptr, unsigned cmdid, void* param )
{
	MSGHANDLE_PARSEHEAD(false);
	int i = 0;
	const Value* svrItm = NULL;

	for (; 0 == Rjson::GetObject(&svrItm, i, &doc); ++i )
	{
		RJSON_GETSTR_D(regname, svrItm);
		RJSON_VGETINT_D(svrid, CONNTERID_KEY, svrItm); 
		RJSON_GETINT_D(prvdid, svrItm);
		RJSON_GETINT_D(pvd_ok, svrItm);
		RJSON_GETINT_D(pvd_ng, svrItm);
		RJSON_GETINT_D(ivk_ok, svrItm);
		RJSON_GETINT_D(ivk_ng, svrItm);
		RJSON_GETINT_D(ivk_dok, svrItm);
		RJSON_GETINT_D(ivk_dng, svrItm);

		if (regname.empty()) continue;
		if ((ivk_dok > 0 || ivk_dng) && 0 == svrid)
		{
			throw NormalExceptionOn(400, CMD_SVRSTAT_RSP, seqid, 
				"leak of svrid on ivk_ok or ivk_ng");
		}

		if (pvd_ok || pvd_ng || ivk_dok || ivk_dng)
		{
			CliBase* provider = iohand;
			if (svrid > 0)
			{
				provider = CliMgr::Instance()->getChildBySvrid(svrid);
				if (NULL == provider) continue;
			}

			ServiceProvider* svrPrvder = This->getProviderPtr(regname);
			if (svrPrvder)
			{
				svrPrvder->setStat(provider, prvdid, pvd_ok, pvd_ng, ivk_dok, ivk_dng);
			}			
		}

		if (ivk_ok || ivk_ng)
		{
			string regname2 = _F("%s%s%%%d", SVRPROP_PREFIX, regname.c_str(), prvdid);
			if (ivk_ok) iohand->setProperty(regname2 + ":ivk_ok", ivk_ok);
			if (ivk_ng) iohand->setProperty(regname2 + ":ivk_ng", ivk_ng);
		}
	}

	return 0;
}

ServiceProvider* ProviderMgr::getProviderPtr( const string& regname ) const
{
	ServiceProvider* ret = NULL;
	map<string, ServiceProvider*>::const_iterator itr = m_providers.find(regname);
	if (itr != m_providers.end())
	{
		ret = itr->second;
	}

	return ret;
}

// return provider个数
int ProviderMgr::getAllJson( string& strjson ) const
{
	int count = 0;
	strjson.append("{");
	map<string, ServiceProvider*>::const_iterator itr = m_providers.begin();
	for (; itr != m_providers.end(); ++itr)
	{
		if (count > 0) strjson.append(",");
		strjson.append(_F("\"%s\":", itr->first.c_str()));
		itr->second->getAllJson(strjson);
		++count;
	}
	strjson.append("}");
	return count;
}

// return item个数
int ProviderMgr::getOneProviderJson( string& strjson, const string& regname ) const
{
	int count = 0;
	strjson.append("{");
	map<string, ServiceProvider*>::const_iterator itr = m_providers.find(regname);
	if (itr != m_providers.end())
	{
		strjson.append(_F("\"%s\":", itr->first.c_str()));
		itr->second->getAllJson(strjson);
		count = 1;
	}
	strjson.append("}");
	return count;
}
// return item个数
int ProviderMgr::getOneProviderJson( string& strjson, const string& regname, short idc, short rack, short version, short limit ) const
{
	int count = 0;
	map<string, ServiceProvider*>::const_iterator itr = m_providers.find(regname);
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