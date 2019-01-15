/*-------------------------------------------------------------------------
FileName     : asyncprvdmsg.h
Description  : 服务提代者异步回送消息的线程安全
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-09       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _ASYNC_PRVD_MSG_H_
#define _ASYNC_PRVD_MSG_H_
#include <list>
#include <string>
#include "msgprop.h"
#include "comm/lock.h"
#include "comm/i_taskrun.h"

using std::string;

class ASyncPrvdMsg: public ITaskRun2
{
    struct AMsgItem
    {
        msg_prop_t msgprop;
        string msg;
    };

public:
    int pushMessage( const msg_prop_t* mp, const string& msg );

public:
    virtual int run(int p1, long p2);
    virtual int qrun( int flag, long p2 );

private:
    std::list<AMsgItem> m_msgQueue;
    ThreadLock m_lock;
};

#endif