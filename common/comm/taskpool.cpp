#include "taskpool.h"

TaskPool::TaskPool( void )
{
    m_threadcount = 3; // default线程数
    m_exit =false;
}

int TaskPool::init( int threadcount )
{
    m_threadcount = threadcount; // 最大线程数
    m_exit =false;
    return 0;
}

int TaskPool::unInit( void )
{
    pthread_t tid;

    if (!m_exit)
    {
        setExit();
    }

    while (m_tid.size() > 0)
    {
        if (m_tid.pop(tid))
        {
            pthread_join(tid, NULL);
        }
    }

    m_tasks.each(ClsTask, true);

    return 0;
}

void TaskPool::setExit( void )
{
    m_exit = true;
    m_tasks.wakeup();
}

int TaskPool::addTask( ITaskRun* task, int delay_ms )
{
    int ret = 0;
    int taskcount;
    int threadcount; // 当前线程数

    do
    {
        ERRLOG_IF1BRK(m_exit, -1, "ADDTASK| msg=wait exiting");
        IFBREAK_N(NULL==task, -2);

        ERRLOG_IF1BRK(!m_tasks.append_delay(task, delay_ms), -3, "ADDTASK| msg=append fail| size=%d",
            m_tasks.size());

        taskcount = m_tasks.size();
        threadcount = (int)m_tid.size();
        if (taskcount > threadcount && threadcount < m_threadcount)
        {
            pthread_t tid;
            ret = pthread_create(&tid, NULL, _ThreadFun, this);
            m_tid.append(tid);
            LOGDEBUG("TASKTHREAD| msg=create task thread| tid=%x| thread=%d/%d",
                (int)tid, threadcount+1, m_threadcount);
        }
    }
    while (0);

    return ret;
}

// 线程进口函数
void* TaskPool::_ThreadFun( void* arg )
{
    TaskPool* This = (TaskPool*)arg;

    while (!This->m_exit)
    {
        ITaskRun* tsk = NULL;
        This->m_tasks.pop_delay(tsk);
        
        if (tsk && !This->m_exit)
        {
            tsk->run_task(0);
        }
    }

    LOGDEBUG("TASKTHREAD| msg=normal thread exit| tid=%x", (int)pthread_self());
    return 0;
}

int TaskPool::size( void )
{
    return m_tasks.size();
}

void TaskPool::ClsTask( ITaskRun*& task )
{
    if (task)
    {
        task->run_task(1);
    }
}
