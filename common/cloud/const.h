/*
 * file : cloud/const.h
 * 项目中通用常量定义
*/

#ifndef __CONST__H__
#define __CONST__H__

const char MYSERVNAME[] = "serv";
const char CONNTERID_KEY[] = "svrid";
const char CLISOCKET_KEY[] = "localsock";
const char CLIENT_TYPE_KEY[] = "clitype";
const char OBJECT_TYPE_KEY[] = "objtype";
const char SERV_ALIAS_PREFIX[] = "serv_";
const char SERV_IN_ALIAS_PREFIX[] = "serv_I";
const char SERV_OUT_ALIAS_PREFIX[] = "serv_o";
const char INNERCLI_ALIAS_PREFIX[] = "c_I";
const char OUTERCLI_ALIAS_PREFIX[] = "c_o"; // c_o_[servid]_[appid]
const char BOOK_HOCFG_ALIAS_PREFIX[] = "cfg";
const char LAST_REQ_SERVMTIME[] = "lastreqall_mtime";

const char CLI_PREFIX_KEY_TIMEOUT[] = "atime_";
const char CLI_PREFIX_KEY_SERV[] = "dserv";
const char EXCLUDE_SVRID_LIST[] = "haspass";
const char ROUTE_PATH[] = "act_path";
const char CLIS_ERASTRING_KEY[] = "ERA";
const char HOCFG_ERASTRING_KEY[] = "CFGERA";
//const char EXROUTE_MSG_KEY[] = ".r"; // 路由信息key

// 广播消息json控制Key
const char BROARDCAST_KEY_FROM[] = "from";
const char BROARDCAST_KEY_PASS[] = "pass";
const char BROARDCAST_KEY_TRAIL[] = "path";
const char BROARDCAST_KEY_JUMP[] = "jump";
const char BROARDCAST_KEY_CLIS[] = "_toapp_";

// 单播消息json控制Key
const char ROUTE_MSG_KEY_FROM[] = "from";
const char ROUTE_MSG_KEY_TO[] = "to";
const char ROUTE_MSG_KEY_BEGORETO[] = "bto";
const char ROUTE_MSG_KEY_REFPATH[] = "refer_path";
const char ROUTE_MSG_KEY_TRAIL[] = "path";
const char ROUTE_MSG_KEY_JUMP[] = "jump";

const char UPDATE_CLIPROP_UPKEY[] = "up";
const char UPDATE_CLIPROP_DOWNKEY[] = "down";

// 分布式配置
const char HOCFG_METAFILE[] = "_meta.json";
const char HOCFG_FILENAME_KEY[] = "file_pattern";
const char HOCFG_KEYPATTEN_KEY[] = "key_pattern";
const char HOCFG_GT_MTIME_KEY[] = "gt_mtime";
const char HOCFG_INCLUDEBASE_KEY[] = "incbase";

const int SERV_CLITYPE_ID = 1;
const int CLI_KEEPALIVE_TIME_SEC = 60*10;
const int CLI_DEAD_TIME_SEC = 60*20;
const int BROADCASTCLI_INTERVAL_SEC = 60*2;
const int PEERSERV_EXIST_CHKTIME = 500*1000; // ms unit
const int PEERSERV_NOEXIST_CHKTIME = 20*1000; // ms unit


#endif