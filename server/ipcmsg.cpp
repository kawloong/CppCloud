#include "ipcmsg.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <errno.h>


IpcMsg::IpcMsg( void ): m_key(0), m_msgid(-1)
{
    
}

int IpcMsg::init( int key, bool creatIfno )
{
    m_key = key;

    if (creatIfno)
    {
        m_msgid = msgget(m_key, IPC_CREAT|IPC_EXCL|0660);
    }

    if (-1 == m_msgid)
    {
        m_msgid = msgget(m_key, 0660);
    }

    return (m_msgid > -1 ? 0 : -1);
}

int IpcMsg::qsize( void )
{
    int ret;
    struct msqid_ds dbuff;

    ret = msgctl(m_msgid, IPC_STAT, &dbuff);
    if (0 == ret)
    {
        ret = dbuff.msg_qnum;
    }

    return ret;
}

int IpcMsg::del( void )
{
    int ret;
    struct msqid_ds dbuff;

    ret = msgctl(m_msgid, IPC_RMID, &dbuff);
    return ret;
}

int IpcMsg::send( const void* dataptr, unsigned int size )
{
    return msgsnd(m_msgid, dataptr, size, 0);
}

int IpcMsg::recv( void* dataptr, unsigned int size, bool noerr, bool noblock )
{
    int msgflag = noblock? IPC_NOWAIT: 0;
    if (noerr)
    {
        msgflag |= MSG_NOERROR;
    }
    return msgrcv(m_msgid, dataptr, size, 0, msgflag);
}

int IpcMsg::clear( int num )
{
    int ret = 0;
    char buff[4];

    if (num > 0)
    {
        while (num--)
        {
            ret = msgrcv(m_msgid, buff, sizeof(buff), 0, MSG_NOERROR);
            if (ret < 0)
            {
                break;
            }
        }
    }
    else
    {
        do
        {
            ret = msgrcv(m_msgid, buff, sizeof(buff), 0, MSG_NOERROR|IPC_NOWAIT);
        }
        while(0 == ret);

        ret = (-1 == ret && ENOMSG == errno) ? 0 : -2;
    }
    
    return ret;
}


