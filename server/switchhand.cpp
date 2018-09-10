#include "switchhand.h"


SwitchHand::SwitchHand(void)
{
    m_pipe[0] = INVALID_FD;
    m_pipe[1] = INVALID_FD;
}

SwitchHand::~SwitchHand(void)
{
    IFCLOSEFD(m_pipe[0]);
    IFCLOSEFD(m_pipe[1]);
}

void SwitchHand::init( int epFd )
{
    int ret = pipe(m_pipe);
    if (0 == ret && INVALID_FD != epFd)
    {
        m_epCtrl.setEPfd(epFd);
        m_epCtrl.setActFd(m_pipe[0]); // read pipe
    }
    else
    {
        LOGERROR("SWITCHINIT| msg=init swichhand fail| epFd=%d| ret=%d", epFd, ret);
    }
}

int SwitchHand::setActive( char fg )
{
    write(m_pipe[1], fg, 1);
    m_epCtrl.setEvt(EPOLLIN, this);
}

int SwitchHand::run( int flag, long p2 )
{

}

int SwitchHand::onEvent( int evtype, va_list ap )
{
    
}
