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


class KeepAliver: public ITaskRun2
{
    SINGLETON_CLASS2(KeepAliver)

    static void init( void );

protected: // interface IEPollRun
	virtual int onEvent( int evtype, va_list ap );
    virtual int qrun( int flag, long p2 );

protected:

};

#endif
