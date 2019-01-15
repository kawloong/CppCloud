#include "asyncprvdmsg.h"
#include "climanage.h"
#include "iohand.h"
#include "cloud/switchhand.h"
#include "comm/public.h"



int ASyncPrvdMsg::pushMessage( const msg_prop_t* mp, const string& msg )
{
    AMsgItem msgitm;
    msgitm.msgprop = *mp;
    msgitm.msg = msg;

    {
        LockGuard lk(m_lock);
        m_msgQueue.push_back(msgitm);
    }

    int ret = SwitchHand::Instance()->appendQTask(this, 0);
    return ret;
}



int ASyncPrvdMsg::run(int p1, long p2)
{
    LOGERROR("DUMMYFLOW");
    return -1;
}

int ASyncPrvdMsg::qrun( int flag, long p2 )
{
    int ret = 0;
    m_lock.Lock();
    if (!m_msgQueue.empty())
    {
        AMsgItem msgitm = m_msgQueue.front();
        m_msgQueue.pop_front();
        m_lock.Unlock();

        IOHand* iohand = (IOHand*)msgitm.msgprop.iohand;
        if (CliMgr::Instance()->getCliInfo(iohand))
        {
            ret = iohand->sendData(msgitm.msgprop.cmdid | CMDID_MID, 
                msgitm.msgprop.seqid, msgitm.msg.c_str(), msgitm.msg.length(), true);
        }
        else
        {
            LOGERROR("ASYNCMSG| msg=iohand maybe offline| iohand=%p| cmdid=0x%x| seqid=%u", 
                iohand, msgitm.msgprop.cmdid, msgitm.msgprop.seqid);
            ret = -2;
        }
    }
    else
    {
        m_lock.Unlock();
    }

    return ret;
}
