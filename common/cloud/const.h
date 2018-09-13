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
const char REMOTESERV_ALIAS_PREFIX[] = "serv_";

const char CLI_PREFIX_KEY_TIMEOUT[] = "atime_";
const char CLI_PREFIX_KEY_SERV[] = "dserv";

const int SERV_CLITYPE_ID = 1;
const int CLI_KEEPALIVE_TIME_SEC = 60*2;
const int CLI_DEAD_TIME_SEC = 60*8;

#endif