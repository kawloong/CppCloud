
#ifndef _I_TASKRUN_H_
#define _I_TASKRUN_H_

// 要实现此接口的类才能放入置任务池处理
struct ITaskRun
{
    // @param flag, 正常调用传0, 退出清理传其他
    virtual int run_task(int flag) = 0;
};

struct ITaskRun2
{
    virtual int run(int p1, long p2) = 0;
};



#endif
