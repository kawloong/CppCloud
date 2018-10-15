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
#include "hocfg_mgr.h"

HEPCLASS_IMPL_FUNCX_BEG(QueryHand)
HEPCLASS_IMPL_FUNCX_MORE(QueryHand, ProcessOne)
HEPCLASS_IMPL_FUNCX_END(QueryHand)

static const char g_resp_strbeg[] = "{ \"code\": 0, \"desc\": \"success\", \"data\": ";

//const char s_app_cache_id[] = "3";
//const char s_scommid_key[] = "scomm_svrid_max";

QueryHand::QueryHand(void)
{
	
}
QueryHand::~QueryHand(void)
{
	
}

/**
 *  interface for HEpBase
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
*/

int QueryHand::ProcessOne( void* ptr, unsigned cmdid, void* param )
{
	CMDID2FUNCALL_BEGIN
	CMDID2FUNCALL_CALL(CMD_GETCLI_REQ);
	CMDID2FUNCALL_CALL(CMD_GETLOGR_REQ);
	CMDID2FUNCALL_CALL(CMD_GETWARN_REQ);
	CMDID2FUNCALL_CALL(CMD_GETCONFIG_REQ);
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


int QueryHand::on_CMD_GETCLI_REQ( IOHand* iohand, const Value* doc, unsigned seqid )
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

	//ret = SendMsg(iohand, CMD_GETCLI_RSP, seqid, outjson, true);
	iohand->sendData(CMD_GETCLI_RSP, seqid, outjson.c_str(), outjson.length(), true);
	return ret;
}


int QueryHand::on_CMD_GETLOGR_REQ( IOHand* iohand, const Value* doc, unsigned seqid )
{
	int nsize = 10;
	Rjson::GetInt(nsize, "size", doc);
	string outstr = g_resp_strbeg;
	Actmgr::Instance()->pickupCliOpLog(outstr, nsize);
	outstr += "}";
	
	return iohand->sendData(CMD_GETLOGR_RSP, seqid, outstr.c_str(), outstr.length(), true);
}

// request format: { "filter_key": "warn", "filter_val":"ng"}
int QueryHand::on_CMD_GETWARN_REQ( IOHand* iohand, const Value* doc, unsigned seqid )
{
	string filter_key;
	string filter_val;
	string outstr = g_resp_strbeg;

	Rjson::GetStr(filter_key, "filter_key", doc);
	Rjson::GetStr(filter_val, "filter_val", doc);
	Actmgr::Instance()->pickupWarnCliProfile(outstr, filter_key, filter_val);
	outstr += "}";
	
	int ret = iohand->sendData(CMD_GETWARN_RSP, seqid, outstr.c_str(), outstr.length(), true);
	return ret;
}

/** 
 * @format: { "file_pattern": "app1-dev", "key_pattern":"/", 
 * 		     "incbase": 1, "gt_mtime": 154000999  }
 * @file_pattern: 文件名
 * @key_pattern: 查询键 （可选）
 * @incbase: 包含继承文件 （可选）
 * @gt_mtime:  （可选）当大于此时间的情况下才响应配置信息，否则返回"ok"
 **/ 
int QueryHand::on_CMD_GETCONFIG_REQ( IOHand* iohand, const Value* doc, unsigned seqid )
{
	RJSON_VGETINT_D(incbase, HOCFG_INCLUDEBASE_KEY, doc);
	RJSON_VGETINT_D(gt_mtime, HOCFG_GT_MTIME_KEY, doc);
	RJSON_VGETSTR_D(file_pattern, HOCFG_FILENAME_KEY, doc);
	RJSON_VGETSTR_D(key_pattern, HOCFG_KEYPATTEN_KEY, doc);

	string result;
	int ret;
	//bool rspIncMtime = doc.HasMember("gt_mtime");
	int curMtime = HocfgMgr::Instance()->getCfgMtime(file_pattern, incbase);
	if (0 == gt_mtime || curMtime > gt_mtime)
	{
		string strtmp;
		result = "{";
		StrParse::PutOneJson(result, "code", 0, true);
		StrParse::PutOneJson(result, "mtime", curMtime, true);
		ret = HocfgMgr::Instance()->query(strtmp, file_pattern, key_pattern, incbase);
		result += _F("\"contents\": %s }", strtmp.c_str());
	}
	else
	{
		result = (0 == curMtime)? "null" : "ok";
	}

	ret = iohand->sendData(CMD_GETCONFIG_RSP, seqid, result.c_str(), result.length(), true);
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