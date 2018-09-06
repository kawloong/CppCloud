#include "thread.h"  

Thread::Thread()
{
    m_threadId = 0;
    m_exit = false;
}

Thread::~Thread() //析构函数  
{
}  

void Thread::start()//启动线程  
{
    pthread_create(&m_threadId, NULL, ThreadRoutine, this);
}

int Thread::join()
{
    int ret = -1;
    if (m_threadId != 0)
    {
        void* status = NULL;
        if (0 == pthread_join(m_threadId, &status))
        {
            ret = (long)status;
        }
    }

    return ret;
}

void* Thread::ThreadRoutine(void* arg)//  
{  
    Thread* thread = static_cast<Thread*>(arg);//派生类指针转换成基类指针  
    return thread->run();
}

void Thread::notifyExit( void )
{
    m_exit = true;
}
