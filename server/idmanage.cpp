#include <errno.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "idmanage.h"


IdMgr::IdMgr( void ): m_shmid(-1), m_ptr(NULL)
{

}

IdMgr::~IdMgr( void )
{
    uninit();
}

// 初始化objectid的共享内存, 用于产生唯一ID
int IdMgr::init( int key )
{
    int ret;
    m_key = key;

    do 
    {
        void* tmpptr = NULL;
        bool first = true;

        ret = m_lock.Init(key);
        ERRLOG_IF1BRK(ret, -1, "IDMGRINIT| msg=sem init fail| key=%d", key);

        LockGuard lk(m_lock);
        m_shmid = shmget(key, sizeof(ShmData), IPC_CREAT|IPC_EXCL|0660);
        if (m_shmid < 0)
        {
            if (EEXIST == errno)
            {
                first = false;
                m_shmid = shmget(key, sizeof(ShmData), 0660);
                ERRLOG_IF1BRK(m_shmid<0, -2, "IDMGRINIT| msg=shmget fail(%s)", strerror(errno));
            }
        }

        tmpptr = shmat(m_shmid, 0, 0);
        ERRLOG_IF1BRK((void*)-1==tmpptr || 0==tmpptr, -3, "IDMGRINIT| msg=shmat fail(%s)| shmid=0x%x",
            strerror(errno), m_shmid);
        m_ptr = (ShmData*)tmpptr;

        if (first)
        {
            m_ptr->ver = SHMDATA_VERSION;
            m_ptr->hostid = 0;
            m_ptr->num = 0;
            LOGINFO("IDMGRINIT| msg=create shm sucess| shmid=0x%x| ver=%d", m_shmid, (int)m_ptr->ver);

            // to do: 当共享内存被删时恢复
        }
        else
        {
            // 检查版本
            m_ptr->hostid = 0;
        }

        ret = 0;
    }
    while (0);
    return ret;
}

void IdMgr::uninit( void )
{
    if (m_ptr)
    {
        shmdt(m_ptr);
        m_ptr = NULL;
    }
}

int IdMgr::set( long long val )
{
    IFRETURN_N(NULL == m_ptr, -1);

    LockGuard lk(m_lock);
    m_ptr->num = val;

    return 0;
}

//int IdMgr::getObjectId(string& objectid)
long long IdMgr::getObjectId( void )
{
    char buff[32];
    long long nn;

    IFRETURN_N(NULL == m_ptr, -1);
    {
        LockGuard lk(m_lock);
        nn = ++m_ptr->num;
    }

    return nn;
}
