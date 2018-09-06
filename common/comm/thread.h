#ifndef _THREAD_H_  
#define _THREAD_H_  

#include <pthread.h>  

// 线程基类  
class Thread
{
public:
    Thread();  
    virtual ~Thread(); 

    void start(void); //线程的启动方法  
    int join(void); //等待线程结束并且返回退出码
    virtual void notifyExit(void);

protected:
    virtual void* run(void) = 0;
    pthread_t m_threadId; //线程ID

    bool m_exit;

private:
    static void* ThreadRoutine(void* arg);//线程入口函数  也是静态全局函数 
    //纯虚函数 //线程执行体run,通过继承类实现不同的线程函数run  

};  

#endif // _THREAD_H_
