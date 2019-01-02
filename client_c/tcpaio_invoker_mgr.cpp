#include "tcpaio_invoker_mgr.h"
#include "tcpaio_invoker.h"
#include "svrconsumer.h"
#include "comm/strparse.h"
#include "comm/lock.h"
#include "cloud/msgid.h"
#include "comm/hepoll.h"

static RWLock gLocker;
//static HEpoll gHepo;

TcpAioInvokerMgr::TcpAioInvokerMgr( void )
{
    //gHepo.init();
    m_epfd = -1;
}

TcpAioInvokerMgr::~TcpAioInvokerMgr( void )
{
    //IOVOKER_POOLT::iterator it = m_pool.find(hostport);
    for (auto it : m_pool)
    {
        IFDELETE(it.second);
    }

    m_pool.clear();
}

/*
void* TcpAioInvokerMgr::run(void)
{
    gHepo.run(m_exit);
    return NULL;
}*/

void TcpAioInvokerMgr::init( int epfd )
{
    m_epfd = epfd;
}

TcpAioInvoker* TcpAioInvokerMgr::getInvoker( const string& hostport, int timeout_sec )
{
    TcpAioInvoker* ivk = NULL;
    {
        RWLOCK_READ(gLocker);
        auto it = m_pool.find(hostport);
        if (it != m_pool.end())
        {
            ivk = it->second;
        }
    }

    if (NULL == ivk)
    {
        ivk = new TcpAioInvoker(hostport);
        int ret = ivk->init(m_epfd, timeout_sec);
        if (0 == ret)
        {
            RWLOCK_WRITE(gLocker);
            auto it = m_pool.find(hostport);
            if (it == m_pool.end())
            {
                m_pool[hostport] = ivk;
                LOGDEBUG("NEWINVOKER| msg=new TcpAioInvoker(%s)", hostport.c_str());
            }
            else
            {
                IFDELETE(ivk);
                ivk = it->second;
            }
        }
        else
        {
            IFDELETE(ivk);
        }
    }

    return ivk;
}


int TcpAioInvokerMgr::requestByHost( string& resp, const string& reqmsg, const string& hostp, int timeout_sec )
{
    static const int check_more_dtsec = 30*60; // 超过此时间的连接可能会失败，增加一次重试
    static const int max_trycount = 2;
    int ret = -1; 
    time_t atime;
    time_t now = time(NULL);

    TcpAioInvoker* ivker = getInvoker(hostp, timeout_sec);
    IFRETURN_N(NULL==ivker, -95);
    atime = ivker->getAtime();
    int trycnt = atime > now - check_more_dtsec ? 1 : max_trycount; // 缰久的连接可重次一次

    while (trycnt-- > 0)
    {

        ret = ivker->request(resp, CMD_TCP_SVR_REQ, reqmsg);
        IFBREAK(0 == ret);
        
    }

    return ret;
}

int TcpAioInvokerMgr::request( string& resp, const string& reqmsg, const string& svrname )
{
    int ret;
    svr_item_t pvd;

    ret = SvrConsumer::Instance()->getSvrPrvd(pvd, svrname);
    ERRLOG_IF1RET_N(ret, ret, "GETPROVIDER| msg=getSvrPrvd fail %d| svrname=%s", ret, svrname.c_str());

    int tosec = SvrConsumer::Instance()->getInvokeTimeoutSec(svrname);
    string hostp = _F("%s:%d", pvd.host.c_str(), pvd.port);
    
    ret = requestByHost(resp, reqmsg, hostp, tosec);
    SvrConsumer::Instance()->addStat(pvd, 0 == ret);
    
    return ret;
}