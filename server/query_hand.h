/*-------------------------------------------------------------------------
FileName     : query_hand.h
Description  : 查询消息处理类
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-06       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _QUERY_HAND_H_
#define _QUERY_HAND_H_
#include "comm/hep_base.h"
#include "rapidjson/json.hpp"


class IOHand;
class QueryHand//: public HEpBase
{

public:
    //HEPCLASS_DECL(QueryHand, QueryHand)
    QueryHand(void);
    virtual ~QueryHand(void);

    // 简单命令用函数处理
    static int ProcessOne( void* iohand, unsigned cmdid, void* param );
    
private:
    #define CMD2FUNCCALL_DESC(cmd) static int on_##cmd(IOHand* iohand, const Value* doc, unsigned seqid )

    static int getIntFromJson( const string& key, const Value* doc );

    CMD2FUNCCALL_DESC(CMD_GETCLI_REQ);
    CMD2FUNCCALL_DESC(CMD_GETLOGR_REQ);
    CMD2FUNCCALL_DESC(CMD_GETWARN_REQ);
    CMD2FUNCCALL_DESC(CMD_GETCONFIG_REQ);
    CMD2FUNCCALL_DESC(CMD_TESTING_REQ);

    static int on_ExchangeMsg( IOHand* iohand, const Value* doc, unsigned cmdid, unsigned seqid );

protected: // interface IEPollRun
    //virtual int run( int flag, long p2 );
	//virtual int onEvent( int evtype, va_list ap );


protected:

};

#endif
