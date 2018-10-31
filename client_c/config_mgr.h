/*-------------------------------------------------------------------------
FileName     : config_mgr.h
Description  : 分布式配置管理类
remark       : 线程安全
Modification :
--------------------------------------------------------------------------
   1、Date  2018-10-30       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _CONFIG_MGR_H_
#define _CONFIG_MGR_H_
#include "comm/public.h"
#include <string>
#include <map>

class ConfigMgr
{
public:
	static int OnCMD_EVNOTIFY_REQ( void* ptr, unsigned cmdid, void* param );
	int onCMD_EVNOTIFY_REQ( void* ptr, unsigned cmdid, void* param );
	static int OnCMD_GETCONFIG_RSP( void* ptr, unsigned cmdid, void* param );
	int onCMD_GETCONFIG_RSP( void* ptr, unsigned cmdid, void* param );


public:
    int initLoad( const string& confName );
    int setMainName( const string& mainConf ); 
    
private:
    string m_mainConfName; // 主配置文件名
    map<string, ConfJson*> m_jcfgs;
    RWLock m_rwLock0;
    static ConfigMgr* This;
};

#endif