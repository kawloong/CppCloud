/*-------------------------------------------------------------------------
FileName     : route_exchange.h
Description  : 路由交换拦截器
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-19       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _ROUTE_EXCHANGE_H_
#define _ROUTE_EXCHANGE_H_
#include "comm/hep_base.h"
#include "rapidjson/json.hpp"

class IOHand;

class RouteExchage: public HEpBase
{
public:
    HEPCLASS_DECL(RouteExchage, RouteExchage);

    RouteExchage(void);
    static void Init( int my_svrid );

    static int TransMsg( void* ptr, unsigned cmdid, void* param );

    static int PostToCli( const string& jobj, unsigned cmdid, unsigned seqid,
        int toSvr = 0, int fromSvr = 0, int bto = 0 );

protected: // interface IEPollRun
	//virtual int onEvent( int evtype, va_list ap );
    //virtual int qrun( int flag, long p2 );
    static int TransMsg( Document& doc, unsigned cmdid, unsigned seqid, int fromSvr, int toSvr, int bto, IOHand* iohand );
    static int AdjustRoutMsg( Document& doc, int fromSvr, int* toSvr, int* bto );

protected:
    static int s_my_svrid;
};

#endif
