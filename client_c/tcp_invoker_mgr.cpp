#include "tcp_invoker_mgr.h"
#include "tcp_invoker.h"
#include "svrconsumer.h"
#include "comm/strparse.h"
#include "comm/lock.h"
#include "cloud/msgid.h"

RWLock gLocker;

TcpInvokerMgr::TcpInvokerMgr( void )
{
    m_eachLimitCount = 5;
}

TcpInvokerMgr::~TcpInvokerMgr( void )
{
    //IOVOKER_POOLT::iterator it = m_pool.find(hostport);
    for (auto it : m_pool)
    {
        for (TcpInvoker* itIn : it.second)
        {
            delete itIn;
        }
    }

    m_pool.clear();
}

void TcpInvokerMgr::setLimitCount( int n )
{
     m_eachLimitCount = n;
}

TcpInvoker* TcpInvokerMgr::getInvoker( const string& hostport )
{
    TcpInvoker* ivk = NULL;
    {
        gLocker.WLock();
        auto it = m_pool.find(hostport);
        if (it != m_pool.end())
        {
            if (!it->second.empty())
            {
                ivk = it->second.front();
                it->second.pop_front();
            }
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

void TcpInvokerMgr::relInvoker( TcpInvoker* ivk )
{
    if (ivk && ivk->check(0))
    {
        string key = ivk->getKey();
        gLocker.WLock();
        if (m_pool[key].size() < (unsigned)m_eachLimitCount)
        {
            m_pool[key].push_back(ivk);
            ivk = NULL;
        }
    }
    
    IFDELETE(ivk);
}

int TcpInvokerMgr::requestByHost( string& resp, const string& reqmsg, const string& hostp )
{
    static const int check_more_dtsec = 30*60; // 超过此时间的连接可能会失败，增加一次重试
    static const int max_trycount = 2;
    int ret = -1; 
    time_t atime;
    time_t now = time(NULL);

    TcpInvoker* ivker = getInvoker(hostp);
    IFRETURN_N(NULL==ivker, -95)
    atime = ivker->getAtime();
    int trycnt = atime > now - check_more_dtsec ? 1 : max_trycount; // 缰久的连接可重次一次

    while (trycnt-- > 0)
    {
        if (!ivker->check(0))
        {
            ret = ivker->connect(true);
            ERRLOG_IF1RET_N(ret, -96, "INVOKERREQ| msg=connect to %s fail", hostp.c_str());
        }

        ret = ivker->send(CMD_TCP_SVR_REQ, reqmsg);
        if (ret)
        {
            ivker->release();
            continue;
        }

        unsigned int rspid = 0;
        ret = ivker->recv(rspid, resp);
        IFBREAK(0 == ret);
        ivker->release();
    }

    relInvoker(ivker);
    return ret;
}

int TcpInvokerMgr::request( string& resp, const string& reqmsg, const string& svrname )
{
    int ret;
    svr_item_t pvd;

    ret = SvrConsumer::Instance()->getSvrPrvd(pvd, svrname);
    ERRLOG_IF1RET_N(ret, ret, "GETPROVIDER| msg=getSvrPrvd fail %d| svrname=%s", ret, svrname.c_str());

    string hostp = _F("%s:%p", pvd.host.c_str(), pvd.port);
    ret = requestByHost(resp, reqmsg, hostp);

    return ret;
}