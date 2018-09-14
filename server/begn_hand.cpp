#include "begn_hand.h"
#include "iobuff.h"
#include "comm/strparse.h"
#include "cppcloud_config.h"
#include "redis/redispooladmin.h"
#include "cloud/const.h"
#include "exception.h"
#include "flowctrl.h"
#include "climanage.h"
#include "act_mgr.h"

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

int BegnHand::ProcessOne( void* ptr, unsigned cmdid, void* param )
{
	int ret = 0;
	IOHand* iohand = (IOHand*)ptr;
	IOBuffItem* iBufItem = (IOBuffItem*)param;
	unsigned seqid = iBufItem->head()->seqid;
	char* body = iBufItem->body();

	Document doc;
	if (doc.ParseInsitu(body).HasParseError())
	{
		// 收到的报文json格式有误
		SendMsgEasy(iohand, CMD_WHOAMI_RSP, seqid, 400, "json format invalid");
		return -79;
	}

#define CMDID2FUNCALL(cmd) case cmd: ret = on_##cmd(iohand, &doc, seqid); break
	switch (cmdid)
	{
		CMDID2FUNCALL(CMD_WHOAMI_REQ);
		CMDID2FUNCALL(CMD_HUNGUP_REQ);
		CMDID2FUNCALL(CMD_SETARGS_REQ);
		CMDID2FUNCALL(CMD_KEEPALIVE_REQ);
		CMDID2FUNCALL(CMD_KEEPALIVE_RSP);

		case CMD_EXCHANG_REQ:
		case CMD_EXCHANG_RSP:
			ret = on_ExchangeMsg(iohand, &doc, cmdid, seqid);
		break;

		default:
		break;
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

int BegnHand::on_CMD_WHOAMI_REQ( IOHand* iohand, const Value* doc, unsigned seqid )
{
	int ret;
	int svrid = 0;
	bool first = (0 == iohand->getCliType()); // 首次请求whoami
	string str; 

	ret = Rjson::GetInt(svrid, CONNTERID_KEY, doc);

	NormalExceptionOff_IFTRUE(ret, 400, CMD_WHOAMI_RSP, seqid, "leak of svrid?");

	if (first)
	{
		if (svrid > 0)
		{
			NormalExceptionOff_IFTRUE(CliMgr::Instance()->getChildBySvrid(svrid), // 是否已被使用
				400, CMD_WHOAMI_RSP, seqid, StrParse::Format("svrid=%d existed", svrid));
		}
		else
		{
			svrid = ++ss_svrid_gen + CloudConf::CppCloudServID()*1000;
		}
		str = StrParse::Itoa(svrid);

		ret = CliMgr::Instance()->addAlias2Child(str, iohand);
		if (ret)
		{
			LOGERROR("SETALIAS| msg=notify set asname fail| asname=%s| ret=%d", str.c_str(), ret);
			throw NormalExceptionOff(400, CMD_WHOAMI_RSP, seqid, "addAlias2Child fail");
		}

		// 解析出每一个字符串属性
		ret = iohand->Json2Map(doc);
		ERRLOG_IF1(ret, "SETPROP| msg=json2map set prop fail %d| svrid=%d", ret, svrid);

		// svrid属性新分配要设置
		CliMgr::Instance()->setProperty(iohand, CONNTERID_KEY, str);

		// 通知获取客户信息完成
		ret = Notify(iohand, HEPNTF_INIT_FINISH);
		Rjson::ToString(str, doc);


		Actmgr::Instance()->setCloseLog(svrid, ""); // clean warn message
		Actmgr::Instance()->appendCliOpLog(StrParse::Format("CLIENT_LOGIN| prog=%s| %s",
															iohand->m_idProfile.c_str(), str.c_str()));
	}
	else
	{
	    iohand->Json2Map(doc);
		Actmgr::Instance()->setCloseLog(svrid, ""); // clean warn message
		LOGWARN("CMD_WHOAMI| msg=set whoami twice| mi=%s", iohand->m_idProfile.c_str());
	}


	whoamiFinish(iohand, first);

	ret = SendMsg(iohand, CMD_WHOAMI_RSP, seqid, true, "{ \"code\": 0, \"svrid\": %d }", svrid);
	LOGDEBUG("CMD_WHOAMI_REQ| req=%s| svrid=%d| seqid=%d sndret=%d", str.c_str(), svrid, seqid, ret);

	return ret;
}

// 自报身份完毕.
int BegnHand::whoamiFinish( IOHand* ioh, bool first )
{

}

int BegnHand::on_CMD_HUNGUP_REQ( IOHand* iohand, const Value* doc, unsigned seqid )
{
	string op;
	int svrid = getIntFromJson(CONNTERID_KEY, doc);
	int ret = Rjson::GetStr(op, "op", doc);

	if ("get" == op) // 获取hung机信息
	{
		string json = g_resp_strbeg;
		Actmgr::Instance()->pickupCliCloseLog(json);
		json += "}";
		ret = SendMsg(iohand, CMD_HUNGUP_RSP, seqid, json, true);
	}
	else if ("set" == op) // 清除hung
	{
		Actmgr::Instance()->rmCloseLog(svrid);
		SendMsgEasy(iohand, CMD_HUNGUP_RSP, seqid, 0, "success");
	}
	else
	{
		SendMsgEasy(iohand, CMD_HUNGUP_RSP, seqid, 402, "leak of op param");
		return -42;
	}

	return ret;
}

// param format: { "warn": "ok", "taskkey": "10023" }
int BegnHand::on_CMD_SETARGS_REQ( IOHand* iohand, const Value* doc, unsigned seqid )
{
	int ret;
	string warnstr;

	// 解析出每一个字符串属性
	ret = iohand->Json2Map(doc);
	ERRLOG_IF1(ret, "SETPROP| msg=json2map set prop fail %d| mi=%s", ret, iohand->m_idProfile.c_str());
	if (0 == Rjson::GetStr(warnstr, "warn", doc) && !warnstr.empty())
	{
		string taskkey = iohand->m_idProfile;
		bool paramok = false;

		if ( warnstr == "ok" || warnstr == "ng" )
		{
			Actmgr::Instance()->setWarnMsg(taskkey, iohand);
			paramok = true;
		}
		else if (0 == warnstr.compare("clear"))
		{
			Actmgr::Instance()->clearWarnMsg(taskkey);
			paramok = true;
		}

		if (!paramok)
		{
			SendMsgEasy(iohand, CMD_SETARGS_RSP, seqid, 401, "leak of param(warn or taskkey)");
			return -50;
		}
	}

	
	SendMsgEasy(iohand, CMD_SETARGS_RSP, seqid, 0, "success");
	return ret;
}


int BegnHand::on_CMD_KEEPALIVE_REQ( IOHand* iohand, const Value* doc, unsigned seqid )
{
	return iohand->sendData(CMD_KEEPALIVE_RSP, seqid, "", 0, true);
}

int BegnHand::on_CMD_KEEPALIVE_RSP( IOHand* iohand, const Value* doc, unsigned seqid )
{
	LOGDEBUG("KEEPALIVE_RSP| mi=%s", iohand->m_idProfile.c_str());
	return 0;
}

// @summery: 消息转发/交换
int BegnHand::on_ExchangeMsg( IOHand* iohand, const Value* doc, unsigned cmdid, unsigned seqid )
{
	int ret = 0;
	int fromSvrid = 0;
	int toSvrid = 0;
	IOHand* dst = NULL;

	Rjson::GetInt(fromSvrid, "from", doc);
	ret = Rjson::GetInt(toSvrid, "to", doc);

	do {
		ERRLOG_IF1BRK(ret || 0 == toSvrid, -46, "EXCHMSG| msg=unknow to svrid| ret=%d| mi=%s", ret, iohand->m_idProfile.c_str());
		dst = dynamic_cast<IOHand*>(CliMgr::Instance()->getChildBySvrid(toSvrid));
		WARNLOG_IF1BRK(NULL == dst, -47, "EXCHMSG| msg=maybe tosvrid offline| toSvrid=%d| mi=%s", toSvrid, iohand->m_idProfile.c_str());
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
			ret = SendMsgEasy(iohand, cmdid|CMDID_MID, seqid, 400, "dst svr offline");
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