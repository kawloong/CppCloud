/*-------------------------------------------------------------------------
FileName     : remote_cli.h
Description  : 远中心端的客户数据管理
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-11       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _REMOTE_CLI_H_
#define _REMOTE_CLI_H_
#include "comm/hep_base.h"
#include "rapidjson/json.hpp"

class IOHand;

class RemoteCli: public HEpBase
{
public:
    HEPCLASS_DECL(RemoteCli, RemoteCli)
    RemoteCli(void);
    virtual ~RemoteCli(void);

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
