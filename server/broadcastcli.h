/*-------------------------------------------------------------------------
FileName     : broadcastcli.h
Description  : 广播本Serv下的cli
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-13       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _BROADCAST_CLI_H_
#define _BROADCAST_CLI_H_
#include "comm/hep_base.h"
#include "rapidjson/json.hpp"

class CliBase;
class IOHand;

class BroadCastCli: public HEpBase
{
public:
    HEPCLASS_DECL(BroadCastCli, BroadCastCli);
    SINGLETON_CLASS2(BroadCastCli)
    BroadCastCli();

public:
    static int OnBroadCMD( void* ptr, unsigned cmdid, void* param );
    static int on_CMD_BROADCAST_REQ( IOHand* iohand, const Value* doc, unsigned seqid );

    void init( int my_svrid );

protected: // interface IEPollRun
	virtual int run(int p1, long p2);
    virtual int qrun( int flag, long p2 );

    int toWorld( int svrid, int ttl, const string& era, const string &excludeSvrid, const string &route );

protected:
    static int s_my_svrid;
    int m_seqid;
};

#endif
