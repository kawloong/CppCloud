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
#include "comm/public.h"
#include "comm/maprange.hpp"
#include "iohand.h"
#include <cstdarg>
#include <map>
#include <list>
#include <string>

using std::map;
using std::list;
using std::string;

typedef MapRanger<string, CliBase*> CliMapRange;

struct CliInfo
{
    time_t t0, t1, t2; // 连接时间，活动时间，关闭时间
    bool inControl; // 如果为true,当调用removeAliasChild删除时会delete对象
    map<string, bool> aliasName; // 客户端的别名，其他人可以通过该别人找到此对象
    map<string, string>* cliProp; // 客户属性

    CliInfo(void): t0(0),t1(0),t2(0),inControl(true),cliProp(NULL){}
};


class CliMgr
{
    SINGLETON_CLASS2(CliMgr)

public:
    enum { CLIOPLOG_SIZE = 200 };
    struct AliasCursor
    {
        CliMapRange iter_range;
        AliasCursor(const string& key_beg);
        AliasCursor(const string& key_beg, const string& key_end);
        CliBase* pop(void);
    };

public:
    // 别名引用相关的操作
    int addChild( HEpBase* chd );
    int addChild( CliBase* child, bool inCtrl = true );
    int addAlias2Child( const string& asname, CliBase* ptr );

    void removeAliasChild( const string& asname );
    void removeAliasChild( CliBase* ptr, bool rmAll );

    CliBase* getChildByName( const string& asname );
    CliBase* getChildBySvrid( int svrid );
    CliInfo* getCliInfo( CliBase* child );
    map<CliBase*, CliInfo>* getAllChild() { return &m_children; }
    int getLocalClisEra( void ) { return m_localEra; }


    void updateCliTime( CliBase* child );

    // 获取一个范围的CliBase*
    // 使用AliasCursor struct

    // 自定义属性的操作
    void setProperty( CliBase* dst, const string& key, const string& val );
    string getProperty( CliBase* dst, const string& key );

    // 退出处理
    int progExitHanele( int flg );
    // 获取当前对象状态
    string selfStat( bool incAliasDetail );

    int onChildEvent( int evtype, va_list ap );

private:
    CliMgr(void);
    ~CliMgr(void);

protected:
    map<CliBase*, CliInfo> m_children;
    map<string, CliBase*> m_aliName2Child;

    IOHand* m_waitRmPtr;
    int m_localEra;
};

#endif
