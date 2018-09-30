#include "query_hand.h"
#include "iobuff.h"
#include "comm/strparse.h"
#include "cloud/const.h"
#include "redis/redispooladmin.h"
#include "flowctrl.h"
#include "act_mgr.h"
#include "iohand.h"
#include "homacro.h"
#include "exception.h"
#include "broadcastcli.h"
#include "route_exchange.h"

HEPCLASS_IMPL_FUNC(QueryHand, ProcessOne)
static const char g_resp_strbeg[] = "{ \"code\": 0, \"desc\": \"success\", \"data\": ";

//const char s_app_cache_id[] = "3";
//const char s_scommid_key[] = "scomm_svrid_max";

QueryHand::QueryHand(void)
{
	
}
QueryHand::~QueryHand(void)
{
	
}

int QueryHand::run( int flag, long p2 )
{
	return 0;
}

// 通告事件处理
int QueryHand::onEvent( int evtype, va_list ap )
{
	int ret = 0;
	if (HEPNTF_INIT_PARAM == evtype)
	{

	}
	
	return ret;
}

int QueryHand::ProcessOne( void* ptr, unsigned cmdid, void* param )
{
	CMDID2FUNCALL_BEGIN
	CMDID2FUNCALL_CALL(CMD_GETCLI_REQ);
	CMDID2FUNCALL_CALL(CMD_GETLOGR_REQ);
	CMDID2FUNCALL_CALL(CMD_GETWARN_REQ);
	CMDID2FUNCALL_CALL(CMD_TESTING_REQ);
	
	return 0;
}


// 适配整型和字符型
int QueryHand::getIntFromJson( const string& key, const Value* doc )
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


int QueryHand::on_CMD_GETCLI_REQ( IOHand* parent, const Value* doc, unsigned seqid )
{
	int ret = 0;
	int svrid = 0;
	string dstSvrId;
	string dstKey;
	string outjson;

	svrid = getIntFromJson(CONNTERID_KEY, doc);

	Rjson::GetStr(dstKey, "key", doc);

	outjson = g_resp_strbeg;
	ret = Actmgr::Instance()->pickupCliProfile(outjson, svrid, dstKey);
	outjson += "}";
	ERRLOG_IF1(ret, "CMD_GETCLI_REQ| msg=pickupCliProfile fail %d| get_svrid=%d| key=%s", ret, svrid, dstKey.c_str());

	ret = SendMsg(parent, CMD_GETCLI_RSP, seqid, outjson, true);
	return ret;
}


int QueryHand::on_CMD_GETLOGR_REQ( IOHand* parent, const Value* doc, unsigned seqid )
{
	int nsize = 10;
	Rjson::GetInt(nsize, "size", doc);
	string outstr = g_resp_strbeg;
	Actmgr::Instance()->pickupCliOpLog(outstr, nsize);
	outstr += "}";
	
	int ret = SendMsg(parent, CMD_GETLOGR_RSP, seqid, outstr, true);
	return ret;
}

// request format: { "filter_key": "warn", "filter_val":"ng" }
int QueryHand::on_CMD_GETWARN_REQ( IOHand* parent, const Value* doc, unsigned seqid )
{
	string filter_key;
	string filter_val;
	string outstr = g_resp_strbeg;

	Rjson::GetStr(filter_key, "filter_key", doc);
	Rjson::GetStr(filter_val, "filter_val", doc);
	Actmgr::Instance()->pickupWarnCliProfile(outstr, filter_key, filter_val);
	outstr += "}";
	
	int ret = parent->sendData(CMD_GETWARN_RSP, seqid, outstr.c_str(), outstr.length(), true);
	return ret;
}

/**
 * 调试接口
*/
int QueryHand::on_CMD_TESTING_REQ( IOHand* iohand, const Value* doc, unsigned seqid )
{
	string cmd;
	Rjson::GetStr(cmd, "cmd", doc);
	if (cmd == "broadshow")
	{
		string rsp = BroadCastCli::GetDebugTrace();
		iohand->sendData(CMD_TESTING_RSP, seqid, rsp.c_str(), rsp.length(), true);
	}
	else if (cmd == "routeto")
	{
		RJSON_GETINT_D(cmdid, doc);
		RJSON_GETINT_D(toSvr, doc);
		RJSON_GETINT_D(fromSvr, doc);
		RJSON_GETINT_D(bto, doc);
		fromSvr = iohand->getIntProperty(CONNTERID_KEY);
		
		int ret = RouteExchage::PostToCli("{\"cmd\": \"dbg\" }", CMD_TESTING_REQ, seqid, toSvr, fromSvr, bto);
		LOGDEBUG("TESTING_REQ| msg=test PostToCli %d->%d| ret=%d", fromSvr, toSvr, ret);
	}
	else if (cmd == "dbg")
	{
		RJSON_GETINT_D(from, doc);
		
		LOGDEBUG("TESTING_REQ| msg=dbg show| %s", Rjson::ToString(doc).c_str());
		int ret = RouteExchage::PostToCli("{\"cmd\": \"dbg-resp\" }", CMD_TESTING_RSP, seqid, from, 0, 0);
		LOGDEBUG("TESTING_REQ| msg=test PostToCli ?->%d| ret=%d", from, ret);
	}

	return 0;
}