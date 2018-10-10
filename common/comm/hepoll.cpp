#include <sys/epoll.h>
#include <cerrno>
#include "hepoll.h"
#include "sock.h"
#include "public.h"
#include "hep_base.h"


#define MAX_EVENT_NUMBER 1024

HEpoll::HEpoll( void ): m_epfd(INVALID_FD), m_lifd(INVALID_FD)
{

}

HEpoll::~HEpoll( void )
{
    unInit();
}

int HEpoll::init( void ) // int port, const char* svrhost /* = NULL*/, int lqueue /*=100*/
{
    int ret = EP_INIT_FAIL;
    do
    {
        m_epfd = epoll_create(100);
        IFBREAK(INVALID_FD == m_epfd);
        ret = 0;
    }
    while (0);

    return ret;
}

int HEpoll::run( bool& bexit )
{
    int ret = 0;
    struct epoll_event backEvs[MAX_EVENT_NUMBER];

    while ( !bexit )
    {
        int number = epoll_wait(m_epfd, backEvs, MAX_EVENT_NUMBER, -1);
        IFBREAK_N((number < 0) && (errno != EINTR), EP_WAIT_FAIL);

        for (int i = 0; i < number; i++)
        {
            /*** EPOLLIN=1 EPOLLOUT=4 EPOLLERR=8 EPOLLHUP=16 ***/
            ITaskRun2* eptr = (ITaskRun2*)backEvs[i].data.ptr;
            ret = eptr->run(backEvs[i].events, 0);
        }
    }

    return ret;
}

void HEpoll::unInit( void )
{
    IFCLOSEFD(m_epfd);
    IFCLOSEFD(m_lifd);
}


