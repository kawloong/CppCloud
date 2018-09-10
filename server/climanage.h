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
#include "iohand.h"
#include <cstdarg>
#include <map>
#include <list>
#include <string>

using std::map;
using std::list;
using std::string;


struct CliInfo
{
    time_t t0, t1, t2; // 连接时间，活动时间，关闭时间
    map<string, bool> aliasName; // 客户端的别名，其他人可以通过该别人找到此对象
    map<string, string>* cliProp; // 客户属性

    CliInfo(void): t0(0),t1(0),t2(0),cliProp(NULL){}
};


class CliMgr
{
    SINGLETON_CLASS2(CliMgr)

    enum { CLIOPLOG_SIZE = 200 };

public:
    // 别名引用相关的操作
    int addChild( HEpBase* chd );
    int addChild( IOHand* child );
    int addAlias2Child( const string& asname, IOHand* ptr );
    void removeAliasChild( const string& asname );
    void removeAliasChild( IOHand* ptr, bool rmAll );
    IOHand* getChildBySvrid( int svrid );
    map<IOHand*, CliInfo>* getAllChild() { return &m_children; }

    // 自定义属性的操作
    void setProperty( IOHand* dst, const string& key, const string& val );
    string getProperty( IOHand* dst, const string& key );

    // 退出处理
    int progExitHanele( int flg );

    int onChildEvent( int evtype, va_list ap );

private:
    CliMgr(void);
    ~CliMgr(void);

protected:
    map<IOHand*, CliInfo> m_children;
    map<string, IOHand*> m_aliName2Child;

    IOHand* m_waitRmPtr;
};

#endif
