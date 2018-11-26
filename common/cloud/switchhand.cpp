#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/epoll.h>
#include "comm/sock.h"
#include "switchhand.h"


std::thread* SwitchHand::s_thread = NULL;

SwitchHand::SwitchHand(void)
{
    m_pipe[0] = INVALID_FD;
    m_pipe[1] = INVALID_FD;
    bexit = false;
}

SwitchHand::~SwitchHand(void)
{
    IFCLOSEFD(m_pipe[0]);
    IFCLOSEFD(m_pipe[1]);
    //IFDELETE(s_thread);
    join();
}

void SwitchHand::init( int epFd )
{
    int ret = pipe(m_pipe);
    if (0 == ret && INVALID_FD != epFd)
    {
        sock_nonblock(m_pipe[0]);
        sock_nonblock(m_pipe[1]);
        m_epCtrl.setEPfd(epFd);
        m_epCtrl.setActFd(m_pipe[0]); // read pipe

        if (s_thread == NULL)
        {
            s_thread = new std::thread(TimeWaitThreadFunc, this); 
        }
    }
    else
    {
        LOGERROR("SWITCHINIT| msg=init swichhand fail| epFd=%d| ret=%d", epFd, ret);
    }
}

void SwitchHand::join( void )
{
    if (s_thread)
    {
        s_thread->join();
        IFDELETE(s_thread);
    }
}

int SwitchHand::setActive( char fg )
{
    int ret;
    ret = write(m_pipe[1], &fg, 1);
    ERRLOG_IF1(ret<0, "SWITCHSETACT| msg=write fail %d| err=%s", ret, strerror(errno));
    ret = m_epCtrl.setEvt(EPOLLIN, this);
    ERRLOG_IF1(ret, "SWITCHSETACT| msg=m_epCtrl.setEvt fail %d| err=%s", ret, strerror(errno));
    return ret;
}

// thread-safe
int SwitchHand::appendQTask( ITaskRun2* tsk, int delay_ms )
{
    return tskwaitq.append_delay(tsk, delay_ms)? 0: -46;
}

// 此方法运行于io-epoll线程
int SwitchHand::run( int flag, long p2 )
{
    int ret = 0;
    if (EPOLLIN & flag) // 有数据可读
    {
        char buff[32] = {0};
        unsigned beg = 0;
        ret = Sock::read(m_pipe[0], buff, beg, 31);
        char prech=' ';

        IFRETURN_N(ERRSOCK_AGAIN == ret, 0);
        ERRLOG_IF1( ret < 0, "SWITCHRUN| msg=pipe brokeren?| ret=%d| err=%d %s| fd=%d",
            ret, Sock::geterrno(m_pipe[0]), strerror(Sock::geterrno(m_pipe[0])), m_pipe[0] );
        for (unsigned i = 0; i < beg; ++i)
        {
            if (prech == buff[i]) continue;
            prech = buff[i];
            switch (prech)
            {
                case 'q': // 执行队列任务(注意qrun里面不要阻塞)
                {
                    ITaskRun2* item = NULL;
                    while ( tskioq.pop(item, 0) )
                    {
                        item->qrun(0, 0);
                    }
                }
                break;

                case 'k':
                break;

                default:
                    LOGERROR("SWITCHRUN| msg=unexcept pipe msg| buff=%s", buff);
                    ret = -76;
                break;
            }
        }
    }
    else
    {
        if (HEFG_PEXIT == flag)
        {
            notifyExit();
        }
        else
        {
            LOGERROR("SWITCHRUN| msg=maybe pipe fail| err=%s| param=%d-%ld", strerror(errno), flag, p2);
            m_epCtrl.setEvt(0, 0);
            IFCLOSEFD(m_pipe[0]);
            IFCLOSEFD(m_pipe[1]);
            pipe(m_pipe);
        }
    }

    return ret;
}
 
int SwitchHand::qrun( int flag, long p2 )
{
    LOGERROR("SWITCHRUN| msg=undefine flow qrun");
    return -1;
}

int SwitchHand::onEvent( int evtype, va_list ap )
{
    return 0;
}

void SwitchHand::notifyExit( void )
{
    bexit = true;
    tskwaitq.wakeup();
    tskioq.wakeup();
    m_epCtrl.setEvt(0, NULL);
}

void SwitchHand::TimeWaitThreadFunc( SwitchHand* This )
{
    ITaskRun2* item = NULL;
    
    while ( !This->bexit ) 
    {
        if (This->tskwaitq.pop_delay(item))
        {
            bool bret = This->tskioq.append(item);
            if (bret)
            {
                This->setActive('q');
            }
            else
            {
                LOGERROR("SwitchHand| msg=append task fail| qsize=%d", This->tskioq.size());
                This->tskwaitq.append_delay(item, 5000);
            }

            item = NULL;
        }
    }
}