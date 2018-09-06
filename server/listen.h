/*-------------------------------------------------------------------------
FileName     : listen.h
Description  : scomm服务端监听器
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-01-23       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _LISTEN_H_
#define _LISTEN_H_
#include "comm/hep_base.h"
#include <map>
#include <list>
#include <string>

using std::map;
using std::list;
using std::string;

class Listen: public HEpBase
{
    struct CliInfo
    {
        time_t t0, t1, t2; // 连接时间，活动时间，关闭时间
        map<string, bool> aliasName; // 客户端的别名，其他人可以通过该别人找到此对象
        CliInfo(void): t0(0),t1(0),t2(0){}
    };

public:
    HEPCLASS_DECL(Listen, Lisn)
    Listen(void);
    virtual ~Listen(void);


public: // interface IEPollRun
    virtual int run( int flag, long p2 );
	virtual int onEvent( int evtype, va_list ap );

protected:
    int creatSock( int port, const char* svrhost = NULL, int lqueue=100 );

protected:

	HEpEvFlag m_evCtrl;
    int m_listenFd;
	string m_workClassName;

};

#endif
