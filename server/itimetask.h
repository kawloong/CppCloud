
#ifndef ITIMER_TASK
#define ITIMER_TASK

struct ITimeQtask
{
    virtual int task_run( int flag ) = 0;
};


#endif