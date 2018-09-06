#include "begn_hand.h"
#include "iobuff.h"
#include "climanage.h"
#include "comm/strparse.h"
#include "cppcloud_config.h"
#include "redis/redispooladmin.h"
#include "flowctrl.h"

HEPCLASS_IMPL_EX(BegnHand, _, MoniFunc)
static const char g_resp_strbeg[] = "{ \"code\": 0, \"desc\": \"success\", \"data\": ";

static int ss_svrid_gen = 1000;
const char s_app_cache_id[] = "3";
const char s_scommid_key[] = "scomm_svrid_max";

BegnHand::BegnHand(void)
{
	
}
BegnHand::~BegnHand(void)
{
	
}

int BegnHand::run( int flag, long p2 )
{
	//setToCache(s_scommid_key, StrParse::Itoa(ss_svrid_gen));
	return 0;
}

// 通告事件处理
int BegnHand::onEvent( int evtype, va_list ap )
{
	int ret = 0;
	if (HEPNTF_INIT_PARAM == evtype)
	{

	}
	
	return ret;
}

int BegnHand::ProcessOne( HEpBase* parent, unsigned cmdid, void* param )
{
	int ret = 0;
	IOBuffItem* iBufItem = (IOBuffItem*)param;
	unsigned seqid = iBufItem->head()->seqid;
	char* body = iBufItem->body();

	Document doc;
	if (doc.ParseInsitu(body).HasParseError())
	{
		// 收到的报文json格式有误
		SendMsgEasy(parent, CMD_WHOAMI_RSP, seqid, 400, "json format invalid");
		return -79;
	}

#define CMDID2FUNCALL(cmd) case cmd: ret = on_##cmd(parent, &doc, seqid); break
	switch (cmdid)
	{
		CMDID2FUNCALL(CMD_WHOAMI_REQ);
		CMDID2FUNCALL(CMD_HUNGUP_REQ);
		CMDID2FUNCALL(CMD_SETARGS_REQ);

		case CMD_EXCHANG_REQ:
		case CMD_EXCHANG_RSP:
			ret = on_ExchangeMsg(parent, &doc, cmdid, seqid);
		break;

		default:
		break;
	}

	return ret;
}

int BegnHand::Json2Map( const Value* objnode, HEpBase* dst )
{
	IFRETURN_N(!objnode->IsObject(), -1);
	int ret = 0;
    Value::ConstMemberIterator itr = objnode->MemberBegin();
    for (; itr != objnode->MemberEnd(); ++itr)
    {
        const char* key = itr->name.GetString();
        if (itr->value.IsString())
        {
        	const char* val = itr->value.GetString();
			ret = Notify(dst, HEPNTF_SET_PROPT, key, val);
        }
		else if (itr->value.IsInt())
		{
			string val = StrParse::Itoa(itr->value.GetInt());
			ret = Notify(dst, HEPNTF_SET_PROPT, key, val.c_str());
		}
    }

    return ret;
}

// 适配整型和字符型
int BegnHand::getIntFromJson( const string& key, const Value* doc )
{
	int ret = 0;
	string strid;
	if (Rjson::GetStr(strid, key.c_str(), doc))
	{
		Rjson::GetInt(ret, key.c_str(), doc);
	}
	else
	{
		ret = atoi(strid.c_str());
	}

	return ret;
}

int BegnHand::on_CMD_WHOAMI_REQ( HEpBase* parent, const Value* doc, unsigned seqid )
{
	int ret;
	int svrid = 0;
	bool first = false;
	string str; 

	ret = Rjson::GetInt(svrid, "svrid", doc);
	if (ret)
	{
		SendMsgEasy(parent, CMD_WHOAMI_RSP, seqid, 401, "leak of svrid?");
		return -40;
	}

	if (svrid > 0)
	{
		if (CliMgr::Instance()->getChildBySvrid(svrid))
		{
			svrid = 0; // 有冲突的话就要重新获取
		}
	}

	if (0 == svrid)
	{
		svrid = ++ss_svrid_gen + CloudConf::CppCloudSvrid()*1000;
		//HEPCLS_STATIC_TIMER_FUN(BegnHand, 0, 200); // 异步事件调用
		first = true;
	}

	str = StrParse::Itoa(svrid);
	ret = HEpBase::Notify(parent, HEPNTF_SET_ALIAS, parent, str.c_str());
	WARNLOG_IF1(ret, "SETALIAS| msg=notify set asname fail| asname=%s| ret=%d", str.c_str(), ret);

	// 解析出每一个字符串属性
	ret = Json2Map(doc, parent);
	ERRLOG_IF1(ret, "SETPROP| msg=json2map set prop fail %d| svrid=%d", ret, svrid);

	// svrid属性新分配要设置
	ret = Notify(parent, HEPNTF_SET_PROPT, "svrid", str.c_str());
	WARNLOG_IF1(ret, "SETALIAS| msg=notify set svrid prop fail| svrid=%d | ret=%d", svrid, ret);

	// 通知获取客户信息完成
	Notify(parent, HEPNTF_INIT_FINISH);
	Rjson::ToString(str, doc);

	if (first)
	{
		CliMgr::Instance()->appendCliOpLog( StrParse::Format("CLIENT_LOGIN| prog=%s| %s",
			 parent->m_idProfile.c_str(), str.c_str()) );
	}
	else
	{
		CliMgr::Instance()->setCloseLog(svrid, ""); // clean warn message
	}

	ret = SendMsg(parent, CMD_WHOAMI_RSP, seqid, true, "{ \"code\": 0, \"svrid\": %d }", svrid);
	LOGDEBUG("CMD_WHOAMI_REQ| req=%s| svrid=%d| seqid=%d sndret=%d", str.c_str(), svrid, seqid, ret);

	return ret;
}

int BegnHand::on_CMD_HUNGUP_REQ( HEpBase* parent, const Value* doc, unsigned seqid )
{
	string op;
	int svrid = getIntFromJson("svrid", doc);
	int ret = Rjson::GetStr(op, "op", doc);

	if ("get" == op) // 获取hung机信息
	{
		string json = g_resp_strbeg;
		CliMgr::Instance()->pickupCliCloseLog(json);
		json += "}";
		ret = SendMsg(parent, CMD_HUNGUP_RSP, seqid, json, true);
	}
	else if ("set" == op) // 清除hung
	{
		CliMgr::Instance()->rmCloseLog(svrid);
		SendMsgEasy(parent, CMD_HUNGUP_RSP, seqid, 0, "success");
	}
	else
	{
		SendMsgEasy(parent, CMD_HUNGUP_RSP, seqid, 402, "leak of op param");
		return -42;
	}

	return ret;
}

// param format: { "warn": "ok", "taskkey": "10023" }
int BegnHand::on_CMD_SETARGS_REQ( HEpBase* parent, const Value* doc, unsigned seqid )
{
	int ret;
	string warnstr;

	// 解析出每一个字符串属性
	ret = Json2Map(doc, parent);
	ERRLOG_IF1(ret, "SETPROP| msg=json2map set prop fail %d| mi=%d", ret, parent->m_idProfile.c_str());
	if (0 == Rjson::GetStr(warnstr, "warn", doc) && !warnstr.empty())
	{
		string taskkey = parent->m_idProfile;
		bool paramok = false;

		if ( warnstr == "ok" || warnstr == "ng" )
		{
			CliMgr::Instance()->setWarnMsg(taskkey, parent);
			paramok = true;
		}
		else if (0 == warnstr.compare("clear"))
		{
			CliMgr::Instance()->clearWarnMsg(taskkey);
			paramok = true;
		}

		if (!paramok)
		{
			SendMsgEasy(parent, CMD_SETARGS_RSP, seqid, 401, "leak of param(warn or taskkey)");
			return -50;
		}
	}

	
	SendMsgEasy(parent, CMD_SETARGS_RSP, seqid, 0, "success");
	return ret;
}


// @summery: 消息转发/交换
int BegnHand::on_ExchangeMsg( HEpBase* parent, const Value* doc, unsigned cmdid, unsigned seqid )
{
	int ret = 0;
	int fromSvrid = 0;
	int toSvrid = 0;
	HEpBase* dst = NULL;

	Rjson::GetInt(fromSvrid, "from", doc);
	ret = Rjson::GetInt(toSvrid, "to", doc);

	do {
		ERRLOG_IF1BRK(ret || 0 == toSvrid, -46, "EXCHMSG| msg=unknow to svrid| ret=%d| mi=%s", ret, parent->m_idProfile.c_str());
		dst = CliMgr::Instance()->getChildBySvrid(toSvrid);
		WARNLOG_IF1BRK(NULL == dst, -47, "EXCHMSG| msg=maybe tosvrid offline| toSvrid=%d| mi=%s", toSvrid, parent->m_idProfile.c_str());
	}
	while (0);

	if (dst)
	{
		ret = SendMsg(dst, cmdid, seqid, Rjson::ToString(doc));
	}
	else
	{
		if (cmdid < CMDID_MID)
		{
			ret = SendMsgEasy(parent, cmdid|CMDID_MID, seqid, 400, "dst svr offline");
		}
	}

	return ret;
}

int BegnHand::getFromCache( string& rdsval, const string& rdskey )
{
	Redis* rds = NULL;
	int ret;
	
	ret = RedisConnPoolAdmin::Instance()->GetConnect(rds, s_app_cache_id);

	ERRLOG_IF1RET_N(0 != ret, -43, "GETREDISDB| msg=get redis conn fail %d| rid=%s", ret, s_app_cache_id);
	ret = rds->get(rdsval, rdskey.c_str());
	ERRLOG_IF1(ret, "GETREDISDB| msg=redis.get fail %d| key=%s", ret, rdskey.c_str());

	RedisConnPoolAdmin::Instance()->ReleaseConnect(rds);
	return ret;
}

int BegnHand::setToCache( const string& rdskey, const string& rdsval )
{
	Redis* rds = NULL;
	int ret;
	
	ret = RedisConnPoolAdmin::Instance()->GetConnect(rds, s_app_cache_id);

	ERRLOG_IF1RET_N(0 != ret, -43, "DETREDISDB| msg=get redis conn fail %d| rid=%s", ret, s_app_cache_id);
	ret = rds->set(rdskey.c_str(), rdsval.c_str());
	ERRLOG_IF1(ret, "SETREDISDB| msg=redis.set fail %d| key=%s", ret, rdskey.c_str());

	RedisConnPoolAdmin::Instance()->ReleaseConnect(rds);
	return ret;
}