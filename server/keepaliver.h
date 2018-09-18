/*-------------------------------------------------------------------------
FileName     : keepalive.h
Description  : 客户对象保活机制
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-13       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _KEEPALIVE_H_
#define _KEEPALIVE_H_
#include "comm/hep_base.h"

class CliBase;


class KeepAliver: public ITaskRun2
{
    SINGLETON_CLASS2(KeepAliver)
    KeepAliver();

public:
    void init( void );

protected: // interface IEPollRun
	virtual int run(int p1, long p2);
    virtual int qrun( int flag, long p2 );

    int sendReq( CliBase* cli ); // keepalive request
    void closeCli( CliBase* cli );

protected:
    int m_seqid;
};

#endif
