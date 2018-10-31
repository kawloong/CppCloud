#include "synchand.h"
#include "comm/public.h"

SyncHand::SyncHand( void )
{
    pthread_mutex_init(&m_mutex, NULL);
}

SyncHand::~SyncHand( void )
{
    pthread_mutex_destroy(&m_mutex);
}

SyncHand::MsgItem::MsgItem( void ): expire_time(0), timeout_sec(3), step(0)
{
    pthread_cond_init(&m_cond, NULL);
}

SyncHand::MsgItem::~MsgItem( void )
{
    pthread_cond_destroy(&m_cond);
}

int SyncHand::putRequest( unsigned rspid, unsigned seqid, int timeout_sec )
{
    unsigned key = rspid;
    key |= (seqid << 16);

    pthread_mutex_lock(&m_mutex);
    MsgItem& msgitm = m_msgItems[key];
    if (msgitm.expire_time > 0) // 已存在
    {
        pthread_mutex_unlock(&m_mutex);
        LOGWARN("SYNCREQ| msg=put req more| key=0x%x| expire_t=%d", key, msgitm.expire_time);
        return -70;
    }

    time_t now = time(NULL);
    msgitm.expire_time = now + timeout_sec;
    pthread_mutex_unlock(&m_mutex);
    return 0;
}

int SyncHand::waitResponse( string& resp, unsigned rspid, unsigned seqid )
{
    unsigned key = rspid;
    key |= (seqid << 16);

    time_t now = time(NULL);

    pthread_mutex_lock(&m_mutex);
    map<unsigned, MsgItem>::iterator it = m_msgItems.find(key);
    if (m_msgItems.end() != it)
    {
        if (2 == it->second.step)
        {
            resp = it->second.resp;
            m_msgItems.erase(it);
            LOGDEBUG("SYNCREQ| msg=resp atonce| key=0x%x| rsplen=%zu| use_dt=%ds", 
                key, resp.size(), int(time(NULL)-now));
                    
            pthread_mutex_unlock(&m_mutex);
            return 0;
        }
    }

    MsgItem& msgitm = it->second;
    struct timespec abstime;
    abstime.tv_sec = msgitm.expire_time;
    abstime.tv_nsec = 0;
    int ret = pthread_cond_timedwait(&msgitm.m_cond, &m_mutex, &abstime);
    it = m_msgItems.find(key);
    if (m_msgItems.end() != it)
    {
        resp = it->second.resp;
        m_msgItems.erase(it);
        ret = (ETIMEDOUT == ret)? -71 : ret;
        LOGDEBUG("SYNCREQ| msg=resp 0x%x(%u)| key=0x%x| rsplen=%zu| use_dt=%ds| ret=%d", 
            rspid, seqid, key, resp.size(), int(time(NULL)-now), ret);
    }
    else
    {
        LOGWARN("SYNCREQ| msg=reqmsg be del | key=0x%x| use_dt=%ds| timwait_ret=%d", 
            key, int(time(NULL)-now), ret);
        ret = -72;
    }

    pthread_mutex_unlock(&m_mutex);
    return ret;    
}

void SyncHand::delRequest( unsigned rspid, unsigned seqid )
{
    unsigned key = rspid;
    key |= (seqid << 16);
    pthread_mutex_lock(&m_mutex);
    m_msgItems.erase(key);
    pthread_mutex_unlock(&m_mutex);
}

int SyncHand::notify( unsigned rspid, unsigned seqid, const string& msg )
{
    int ret = 1;
    unsigned key = rspid;
    key |= (seqid << 16);

    pthread_mutex_lock(&m_mutex);
    map<unsigned, MsgItem>::iterator it = m_msgItems.find(key);
    if (m_msgItems.end() != it)
    {
        it->second.resp = msg;
        int signalret = pthread_cond_signal(&it->second.m_cond);
        WARNLOG_IF1(signalret, "SYNCNOTIFY| msg=pthread_cond_signal fail %d| key=0x%x", signalret, key);
        ret = 0;
    }
    pthread_mutex_unlock(&m_mutex);

    return ret;
}
