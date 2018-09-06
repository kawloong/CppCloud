/*-------------------------------------------------------------------------
FileName     : begn_hand.h
Description  : 通用消息处理类 / 初始消息
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-01-31       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _BEGN_HAND_H_
#define _BEGN_HAND_H_
#include "comm/hep_base.h"
#include "rapidjson/json.hpp"



class BegnHand: public HEpBase
{

public:
    HEPCLASS_DECL(BegnHand, BegnHand)
    BegnHand(void);
    virtual ~BegnHand(void);

    // 简单命令用函数处理
    static int ProcessOne( HEpBase* parent, unsigned cmdid, void* param );
    
    static int getFromCache( string& rdsval, const string& rdskey );
    static int setToCache( const string& rdskey, const string& rdsval );

private:
    #define CMD2FUNCCALL_DESC(cmd) static int on_##cmd(HEpBase* parent, const Value* doc, unsigned seqid )
    static int Json2Map( const Value* objnode, HEpBase* dst );
    static int getIntFromJson( const string& key, const Value* doc );

    CMD2FUNCCALL_DESC(CMD_WHOAMI_REQ);
    CMD2FUNCCALL_DESC(CMD_HUNGUP_REQ);
    CMD2FUNCCALL_DESC(CMD_SETARGS_REQ);

    static int on_ExchangeMsg( HEpBase* parent, const Value* doc, unsigned cmdid, unsigned seqid );

protected: // interface IEPollRun
    virtual int run( int flag, long p2 );
	virtual int onEvent( int evtype, va_list ap );


protected:

};

#endif
