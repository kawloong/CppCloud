/*-------------------------------------------------------------------------
FileName     : switchhand.h
Description  : 线程切换过渡操作
remark       : 主要实现cli的keepalive动作
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-10       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _SWITCHHAND_H_
#define _SWITCHHAND_H_
#include "comm/i_taskrun.h"
#include "comm/public.h"

class SwitchHand: public ITaskRun2
{
    SINGLETON_CLASS2(SwitchHand);
    SwitchHand(void);
    ~SwitchHand(void);

    void init( int epFd );
    int setActive( char fg );

public: // interface ITaskRun2
    virtual int run( int flag, long p2 );
	virtual int onEvent( int evtype, va_list ap );

private:
    HEpEvFlag m_epCtrl;
    int m_pipe[2];
};

#endif