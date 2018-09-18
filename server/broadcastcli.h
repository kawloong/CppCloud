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

class CliBase;


class BroadCastCli: public HEpBase
{
public:
    HEPCLASS_DECL(BroadCastCli, BroadCastCli);
    SINGLETON_CLASS2(BroadCastCli)
    BroadCastCli();

    static int OnBroadCMD( void* ptr, unsigned cmdid, void* param );

    void init( void );

protected: // interface IEPollRun
	virtual int run(int p1, long p2);
    virtual int qrun( int flag, long p2 );



protected:
    static int s_my_svrid;
    int m_seqid;
};

#endif
