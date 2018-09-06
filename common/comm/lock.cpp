#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "lock.h"

/********************** ThreadLock **********************/
ThreadLock::ThreadLock()
{
    pthread_mutex_init( &m_mutex, NULL );
}

ThreadLock::~ThreadLock()
{
    pthread_mutex_destroy( &m_mutex );
}

int ThreadLock::Lock()
{
    if (pthread_mutex_lock( &m_mutex ) != 0)
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

int ThreadLock::Unlock()
{
    if (pthread_mutex_unlock( &m_mutex ) != 0)
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

int ThreadLock::TryLock( char rORw )
{
    if (pthread_mutex_trylock( &m_mutex ) != 0)
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

/********************** RecursiveLock **********************/
RecursiveLock::RecursiveLock()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

RecursiveLock::~RecursiveLock()
{
    pthread_mutex_destroy( &m_mutex );
}

int RecursiveLock::Lock()
{
    if (pthread_mutex_lock( &m_mutex ) != 0)
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

int RecursiveLock::Unlock()
{
    if (pthread_mutex_unlock( &m_mutex ) != 0)
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

int RecursiveLock::TryLock( char rORw )
{
    if (pthread_mutex_trylock( &m_mutex ) != 0)
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

/********************** ConditionMutex **********************/
ConditionMutex::ConditionMutex()
{
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_cond, NULL);
}

ConditionMutex::~ConditionMutex()
{
    pthread_cond_destroy(&m_cond);
    pthread_mutex_destroy(&m_mutex);
}

int ConditionMutex::Lock()
{
    if (pthread_mutex_lock(&m_mutex) != 0)
    {
        return errno == 0 ? -1 : errno;
    }
    return 0;
}

int ConditionMutex::Unlock()
{
    if (pthread_mutex_unlock(&m_mutex) != 0)
    {
        return errno == 0 ? -1 : errno;
    }
    return 0;
}

int ConditionMutex::Wait()
{
    if (pthread_cond_wait(&m_cond, &m_mutex) != 0)
    {
        return errno == 0 ? -1 : errno;
    }
    return 0;
}

int ConditionMutex::Wait(int nWaitTime)
{
    struct timespec abstime;
    abstime.tv_sec = time(NULL) + nWaitTime;
    abstime.tv_nsec = 0;
    
    if (pthread_cond_timedwait(&m_cond, &m_mutex, &abstime) != 0)
    {
        return errno == 0 ? -1 : errno;
    }
    return 0;
}

int ConditionMutex::Signal()
{
    if (pthread_cond_signal(&m_cond) != 0)
    {
        return errno == 0 ? -1 : errno;
    }
    return 0;
}

int ConditionMutex::TryLock( char rORw )
{
    if (pthread_mutex_trylock( &m_mutex ) != 0)
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

/********************** RWLock **********************/
RWLock::RWLock(): m_mutithread(true)
{
	pthread_rwlock_init( &m_rwlock, NULL );
}

RWLock::RWLock(bool muti_thread): m_mutithread(muti_thread)
{
    if (!m_mutithread) return;
    pthread_rwlock_init( &m_rwlock, NULL );
}

RWLock::~RWLock()
{
    if (!m_mutithread) return;
	pthread_rwlock_destroy( &m_rwlock );
}

int RWLock::RLock()
{
    if (!m_mutithread) return 0;
	if (pthread_rwlock_rdlock( &m_rwlock ) != 0)
	{
		return errno == 0 ? -1 : errno;
	}

	return 0;
}

int RWLock::WLock()
{
    if (!m_mutithread) return 0;
	if (pthread_rwlock_wrlock( &m_rwlock ) != 0)
	{
		return errno == 0 ? -1 : errno;
	}

	return 0;
}

int RWLock::UnLock()
{
    if (!m_mutithread) return 0;
	if (pthread_rwlock_unlock( &m_rwlock ) != 0)
	{
		return errno == 0 ? -1 : errno;
	}

	return 0;
}

int RWLock::TryLock( char rORw )
{
    if ('r' == rORw)
    {
        if (pthread_rwlock_tryrdlock( &m_rwlock ) != 0)
        {
            return errno == 0 ? -1 : errno;
        }
    }
    else // 'w'
    {
        if (pthread_rwlock_trywrlock( &m_rwlock ) != 0)
        {
            return errno == 0 ? -1 : errno;
        }
    }

    return 0;
}


/********************** SemLock **********************/
SemLock::SemLock()
{
    m_iSemid = -1;
}

SemLock::~SemLock()
{
}

int SemLock::Lock()
{
    struct sembuf cSembuf;

    cSembuf.sem_num = 0;
    cSembuf.sem_op = -1;
    cSembuf.sem_flg = SEM_UNDO;

    if (semop( m_iSemid, &cSembuf, 1 ) != 0)
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

int SemLock::Unlock()
{
    struct sembuf cSembuf;

    cSembuf.sem_num = 0;
    cSembuf.sem_op = 1;
    cSembuf.sem_flg = SEM_UNDO;

    if (semop( m_iSemid, &cSembuf, 1 ) != 0)
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

int SemLock::Init(const std::string& strPath)
{
    key_t key = ftok( strPath.c_str(), 0 );
    if (key == -1)
    {
        return errno == 0 ? -1 : errno;
    }

    return Init(key);
}

int SemLock::Init(key_t key)
{
    int iRet = 0;
    m_iSemid = semget( key, 0, 0 );
    if ( 0 > m_iSemid )
    {
        m_iSemid = semget( key, 1, IPC_CREAT | IPC_EXCL | 0660 );
        if ( 0 <= m_iSemid )
        {
            semun arg;

            arg.val = 1;
            iRet = semctl( m_iSemid, 0, SETVAL, arg );
            if ( -1 == iRet )
            {
                iRet =  errno == 0 ? -1 : errno;
                Finish();
            }
        }
        else
        {
            iRet =  errno == 0 ? -1 : errno;
        }
    }

    return iRet;
}

void SemLock::Finish()
{
    if ( 0 <= m_iSemid )
    {
        semctl( m_iSemid, 0, IPC_RMID );
        m_iSemid = -1;
    }
}

int SemLock::TryLock( char )
{
    struct sembuf cSembuf;

    cSembuf.sem_num = 0;
    cSembuf.sem_op = -1;
    cSembuf.sem_flg = SEM_UNDO|IPC_NOWAIT;

    if (semop( m_iSemid, &cSembuf, 1 ) != 0)
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

/********************* FileLock ********************/
FileLock::FileLock()
{
    m_fd = -1;
    m_bOpen = false;
}

FileLock::~FileLock()
{
    if (m_bOpen && m_fd >= 0)
    {
        close(m_fd);
    }
}

int FileLock::Lock()
{
    struct flock   flk;
    flk.l_type   = F_WRLCK;
    flk.l_whence = SEEK_SET;
    flk.l_start  = 0;
    flk.l_len    = 0;
    flk.l_pid    = getpid();

    if (fcntl(m_fd, F_SETLKW, &flk) != 0) {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

int FileLock::Unlock()
{
    struct flock   flk;
    flk.l_type   = F_UNLCK;
    flk.l_whence = SEEK_SET;
    flk.l_start  = 0;
    flk.l_len    = 0;
    flk.l_pid    = getpid();

    if (fcntl(m_fd, F_SETLKW, &flk) != 0) {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

int FileLock::Init(const std::string& filePath)
{
    if (m_fd >= 0 && m_bOpen)
    {
        close(m_fd);
    }

    m_fd = open(filePath.c_str(), O_RDWR|O_CREAT, 0666);
    if (m_fd < 0)
    {
        return errno == 0 ? -1 : errno;
    }
    m_bOpen = true;

    return 0;
}

int FileLock::Init(int fd)
{
    if (m_fd >= 0 && m_bOpen)
    {
        close(m_fd);
    }

    m_fd = fd;
    m_bOpen = false;
    return (m_fd >= 0 ? 0 : -1);
}

int FileLock::TryLock( char )
{
    struct flock   flk;
    flk.l_type   = F_WRLCK;
    flk.l_whence = SEEK_SET;
    flk.l_start  = 0;
    flk.l_len    = 0;
    flk.l_pid    = getpid();

    if (fcntl(m_fd, F_GETLK, &flk) != 0){
        return errno == 0 ? -1 : errno;
    }

    if(flk.l_type != F_UNLCK)
    {
        return -10;
    }

    return 0;
}

/********************* SemPV ********************/
SemPV::SemPV()
{
    sem_init( &m_sem, 0, 1 );
}

SemPV::~SemPV()
{
    sem_destroy( &m_sem );
}


int SemPV::WaitP()
{
    if ( sem_wait( &m_sem ) != 0 )
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

int SemPV::PostV()
{
    if ( sem_post( &m_sem ) != 0 )
    {
        return errno == 0 ? -1 : errno;
    }

    return 0;
}

