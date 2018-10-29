#include "tcp_invoker_mgr.h"
#include "comm/lock.h"

RWLock gLocker;

TcpInvokerMgr::TcpInvokerMgr( void )
{
    m_eachLimitCount = 5;
}

void TcpInvokerMgr::setLimitCount( int n )
{
     m_eachLimitCount = n;
}

TcpInvoker* TcpInvokerMgr::getInvoker( const string& hostport )
{
    TcpInvoker* ivk = NULL;
    gLocker.WLock();
    IOVOKER_POOLT::iterator it m_pool.find(hostport);
    if (it != m_pool.end())
    {
        if (!it->second.empty())
        {
            ivk = it->second.pop();
        }
    }

    if (NULL == ivk)
    {
        ivk = new TcpInvoker(hostport);
        if (0 != ivk->init(m_invokerTimOut_sec))
        {
            IFDELETE(ivk);
        }
    }

    return ivk;
}