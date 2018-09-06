/*-------------------------------------------------------------------------
FileName     : climanage.h
Description  : 客户端连接管理类
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-01-23       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _CLIMANAGE_H_
#define _CLIMANAGE_H_
#include "comm/hep_base.h"
#include "comm/public.h"
#include <cstdarg>
#include <map>
#include <list>
#include <string>

using std::map;
using std::list;
using std::string;

class CliMgr
{
    SINGLETON_CLASS2(CliMgr)

    struct CliInfo
    {
        time_t t0, t1, t2; // 连接时间，活动时间，关闭时间
        map<string, bool> aliasName; // 客户端的别名，其他人可以通过该别人找到此对象

        CliInfo(void): t0(0),t1(0),t2(0){}
    };

    enum { CLIOPLOG_SIZE = 200 };

public:
    // 别名引用相关的操作
    int addChild( HEpBase* child );
    int addAlias2Child( const string& asname, HEpBase* ptr );
    void removeAliasChild( const string& asname );
    void removeAliasChild( HEpBase* ptr, bool rmAll );
    HEpBase* getChildBySvrid( int svrid );

    // 自定义属性的操作
    void setProperty( HEpBase* dst, const string& key, const string& val );
    string getProperty( HEpBase* dst, const string& key );

    // 获取连接中的客户端属性信息
    int pickupCliProfile( string& json, int svrid, const string& key );
    // 获取已掉线的客户信息
    int pickupCliCloseLog( string& json );
    // 获取客户行为日志信息
    int pickupCliOpLog( string& json, int nSize );
    // 获取所有告警状态的客户机信息
    int pickupWarnCliProfile( string& json, const string& filter_key, const string& filter_val );

    // 网监业务相关方法
    int onChildEvent( int evtype, va_list ap );
    void setCloseLog( int svrid, const string& cloLog );
    void rmCloseLog( int svrid );
    void appendCliOpLog( const string& logstr );
    /////////////
    void setWarnMsg( const string& taskkey, HEpBase* ptr );
    void clearWarnMsg( const string& taskkey );

    // 退出处理
    int progExitHanele( int flg );

private:
    CliMgr(void);
    ~CliMgr(void);

protected:
    map<HEpBase*, CliInfo> m_children;
    map<string, HEpBase*> m_aliName2Child;
    map<string, HEpBase*> m_warnLog; // 客户机告警的任务
    map<int, string> m_closeLog; // 记录掉线了的客户机信息
    list<string> m_cliOpLog; // 客户机的操作行为记录
    int m_opLogSize;
    HEpBase* m_waitRmPtr;
};

#endif
