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
#include <vector>

using namespace std;
typedef void (*CONF_CHANGE_CB)(const string& confname);
class ConfJson;

class ConfigMgr
{
    SINGLETON_CLASS2(ConfigMgr)
public:
	static int OnCMD_EVNOTIFY_REQ( void* ptr );
	int onCMD_EVNOTIFY_REQ( void* ptr );
	static int OnCMD_GETCONFIG_RSP( void* ptr, unsigned cmdid, void* param );
	int onCMD_GETCONFIG_RSP( void* ptr, unsigned cmdid, void* param );

    ConfigMgr( void );
    ~ConfigMgr( void );

public:
    int initLoad( const string& confName );
    //void setMainName( const string& mainConf );
    void uninit( void );
    void setChangeCallBack( CONF_CHANGE_CB cb );

    /**
     * @summery: 通过传入查询键名qkey，返回对应值
     * @param: ValT must be [string, int, map<string,string>, map<string,int>, vector<string>, vector<int>]
     * @remart: thread-safe method
     * @return: if success return 0; 
    ***/
    int query( int& oval, const string& fullqkey );
    int query( string& oval, const string& fullqkey );
    int query( map<string, string>& oval, const string& fullqkey );
    int query( map<string, int>& oval, const string& fullqkey );
    int query( vector<string>& oval, const string& fullqkey );
    int query( vector<int>& oval, const string& fullqkey );
    

private:
    void _clearCache( void );

    // ValT must be [string, int, map<string,string>, map<string,int>, vector<string>, vector<int>]
    template<class ValT>
    int _query( ValT& oval, const string& fullqkey, map<string, ValT >& cacheMap, bool wideStr ) const;
    template<class ValT>
    int _tryGetFromCache( ValT& oval, const string& fullqkey, const map<string, ValT >& cacheMap ) const;

    
private:
    string m_mainConfName; // 主配置文件名
    map<string, ConfJson*> m_jcfgs;
    CONF_CHANGE_CB m_changeCB;
    static ConfigMgr* This;

    // 缓存
    map<string, string> m_cacheStr;
    map<string, int> m_cacheInt;
    map<string, map<string, string> > m_cacheMapStr;
    map<string, map<string, int> > m_cacheMapInt;
    map<string, vector<string> > m_cacheVecStr;
    map<string, vector<int> > m_cacheVecInt;
};

#endif