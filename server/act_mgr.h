/*-------------------------------------------------------------------------
FileName     : act_mgr.h
Description  : 客户端行为信息管理类(告警，上下机等)
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-01-23       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _ACT_MGR_H_
#define _ACT_MGR_H_
#include "comm/public.h"
#include <cstdarg>
#include <map>
#include <list>
#include <string>

using std::map;
using std::list;
using std::string;

class CliBase;
struct CliInfo;

class Actmgr
{
    SINGLETON_CLASS2(Actmgr)
    enum { CLIOPLOG_SIZE = 200 };

public:
    static int NotifyCatch( void* ptr, unsigned cmdid, void* param );

public:
    // 获取连接中的客户端属性信息
    int pickupCliProfile( string& json, int svrid, const string& key );
    // 获取已掉线的客户信息
    int pickupCliCloseLog( string& json );
    // 获取客户行为日志信息
    int pickupCliOpLog( string& json, int nSize );
    // 获取所有告警状态的客户机信息
    int pickupWarnCliProfile( string& json, const string& filter_key, const string& filter_val );


    int appCloseFound( CliBase* son, int clitype, const CliInfo& cliinfo );
    void setCloseLog( int svrid, const string& cloLog );
    void rmCloseLog( int svrid );
    void appendCliOpLog( const string& logstr );
    /////////////
    void setWarnMsg( const string& taskkey, CliBase* ptr );
    void clearWarnMsg( const string& taskkey );

private:
    void getJsonProp( CliBase* cli, string& outj, const string& key );

private:
    Actmgr(void);
    ~Actmgr(void);

protected:
    map<CliBase*, CliInfo>* m_pchildren; // 对CliMgr.m_children的引用
    map<string, CliBase*> m_warnLog; // 客户机告警的任务
    map<int, string> m_closeLog; // 记录掉线了的客户机信息
    list<string> m_cliOpLog; // 客户机的操作行为记录
    int m_opLogSize;

    static Actmgr* This;
};

#endif
