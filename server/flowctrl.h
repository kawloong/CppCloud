/******************************************************************* 
 *  summery: 总体服务流程控件器
 *  author:  hejl
 *  date:    2018-01-26
 *  description: 一般地包含IO复用器和队列任务器
 ******************************************************************/ 
#ifndef _FLOWCTRL_H_
#define _FLOWCTRL_H_
#include "comm/taskpool.hpp"
#include "comm/hep_base.h"
#include "comm/hepoll.h"

class FlowCtrl
{
public:
    static FlowCtrl* Instance(void);

    // 初始化时传入：任务队列数量（一般1个即可）
    int init(int tskqNum);
    int notifyExit(void);
    void uninit(void); // 收到退出信号时调用
	
	// 添加tcp监听器
    int addListen( const char*  lisnClassName, int port, const char* svrhost=NULL, int lqueue=100 );
    // 清加到某一任务线程
    int appendTask( HEpBase* tsk, int qidx, int delay_ms );

    int run( bool& bexit );

protected: // for singleton func
    FlowCtrl(void);
    ~FlowCtrl(void);
    FlowCtrl(const FlowCtrl&); // dummy
    FlowCtrl& operator=(const FlowCtrl&); // dummy

private:
    HEpoll* m_hepo; // io handle
    TaskPoolEx<HEpBase, &HEpBase::run, HEFG_QUEUERUN, HEFG_QUEUEEXIT>* m_tskq;
    HEpBase* m_listener;
    int m_tskqNum;
};

#endif
