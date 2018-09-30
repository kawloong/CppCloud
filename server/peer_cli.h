/*-------------------------------------------------------------------------
FileName     : peer_cli.h
Description  : 远中心端的客户数据管理
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-11       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _PEER_CLI_H_
#define _PEER_CLI_H_
#include "comm/hep_base.h"
#include "rapidjson/json.hpp"

class IOHand;

class PeerCli: public HEpBase
{
public:
    HEPCLASS_DECL(PeerCli, PeerCli)
    PeerCli(void);
    virtual ~PeerCli(void);

    static void Init( int mysid );

protected: // interface IEPollRun
	virtual int onEvent( int evtype, va_list ap );

    int cmdHandle( unsigned cmdid, unsigned seqid, char* body );
    int on_CMD_IAMSERV_REQ( const Value* doc, unsigned seqid );
    int on_CMD_IAMSERV_RSP( const Value* doc, unsigned seqid );

protected:
    IOHand* m_iohand;
    static int s_my_svrid;
};

#endif
