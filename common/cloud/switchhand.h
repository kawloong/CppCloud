/*-------------------------------------------------------------------------
FileName     : switchhand.h
Description  : 线程切换过渡操作
remark       : 主要实现定时等待的任务
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-10       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _SWITCHHAND_H_
#define _SWITCHHAND_H_
#include <thread>
#include "comm/hep_base.h"
#include "comm/queue.h"
#include "comm/public.h"

class SwitchHand: public ITaskRun2
{
    SINGLETON_CLASS2(SwitchHand);

    SwitchHand(void);
    virtual ~SwitchHand(void);

public:
    void init( int epFd );
    int setActive( char fg );

    void notifyExit( void );
    void join( void );

    // 添加到某一任务io-epoll线程
    int appendQTask( ITaskRun2* tsk, int delay_ms );
    int remove( ITaskRun2* tsk );

private:
    static void TimeWaitThreadFunc( SwitchHand* This );

public: // interface ITaskRun2
    virtual int run( int flag, long p2 );
    virtual int qrun( int flag, long p2 );
	virtual int onEvent( int evtype, va_list ap );

private:
    HEpEvFlag m_epCtrl;
    int m_pipe[2]; // 通过pipe的方式和epoll线程通信
    
    static std::thread* s_thread;
    Queue<ITaskRun2*, false> tskwaitq; // 未到触发点的等待任务
    Queue<ITaskRun2*, false> tskioq; // 队列中的对象将会在io-epoll线程执行.qrun()方法
    bool bexit;
};

#endif