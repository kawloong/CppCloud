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
#include <vector>
#include <string>

using std::map;
using std::vector;
using std::string;

typedef void (*CliPreCloseNotifyFunc)( IOHand* );
typedef MapRanger<string, IOHand*> CliMapRange;

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
        IOHand* pop(bool forceFindEach=false);
        bool empty(void);
    };

public:
    // 别名引用相关的操作
    int newChild( int clifd, int epfd );
    int addChild( IOHand* child, bool inCtrl = true );
    int addAlias2Child( const string& asname, IOHand* ptr );

    void removeAliasChild( const string& asname );
    void removeAliasChild( IOHand* ptr, bool rmAll );

    IOHand* getChildByName( const string& asname );
    IOHand* getChildBySvrid( int svrid );
    CliInfo* getCliInfo( IOHand* child );
    map<IOHand*, CliInfo>* getAllChild() { return &m_children; }
    int getLocalAllCliJson( string& jstr );

    void updateCliTime( IOHand* child );


    // 自定义属性的操作
    void setProperty( IOHand* dst, const string& key, const string& val );
    string getProperty( IOHand* dst, const string& key );

    // 退出处理
    int progExitHanele( int flg );
    int onChildEvent( int evtype, va_list ap );
    static int OnChildEvent( int evtype, va_list ap );
    // 获取当前对象状态
    string selfStat( bool incAliasDetail );


private:
    CliMgr(void);
    ~CliMgr(void);

protected:
    map<IOHand*, CliInfo> m_children;
    map<string, IOHand*> m_aliName2Child;
    vector<CliPreCloseNotifyFunc> m_cliCloseConsumer; // 客户关闭事件消费者

    IOHand* m_waitRmPtr;
};

#endif
