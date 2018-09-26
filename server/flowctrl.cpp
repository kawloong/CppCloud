#include "flowctrl.h"
#include "iohand.h"
#include "comm/hep_base.h"
#include "climanage.h"
#include "cppcloud_config.h"
#include "remote_mgr.h"
#include "switchhand.h"
#include "keepaliver.h"
#include "route_exchange.h"

HEPCLASS_IMPL(RemoteMgr, RemoteMgr);

FlowCtrl* FlowCtrl::Instance(void)
{
    static FlowCtrl s;
    return &s;
}
 
FlowCtrl::FlowCtrl(void)
{
    m_tskqNum = 1;
    m_hepo = NULL;
    m_tskq = NULL;
    m_listener = NULL;
    m_mysvrid = 0;
}

FlowCtrl::~FlowCtrl(void)
{
    IFDELETE(m_hepo);
    IFDELETE_ARR(m_tskq);
    IFDELETE(m_listener);
}

int FlowCtrl::init(int tskqNum)
{
    int ret = -1;


    if (tskqNum > 0 && NULL == m_hepo && NULL == m_tskq)
    {
        m_hepo = new HEpoll;
        m_tskq = new TaskPoolEx<HEpBase, &HEpBase::qrun>[tskqNum];
        m_tskqNum = tskqNum; 
        IOHand::Init();

        ret = m_hepo->init();

        if (0 == ret)
        {
            SwitchHand::Instance()->init(m_hepo->getEPfd());
            KeepAliver::Instance()->init();
            RouteExchage::Init(CloudConf::CppCloudServID());
            ret = RemoteMgr::Instance()->init(m_hepo->getEPfd());
        }
    }

    return ret;
}


int FlowCtrl::addListen(const char* lisnClassName, int port, const char* svrhost/*=NULL*/, int lqueue/*=100*/)
{
    IFRETURN_N(m_listener, -9);
    IFRETURN_N(NULL == lisnClassName || port <= 0, -10);

    m_listener = HEpBase::New(lisnClassName);
    ERRLOG_IF1RET_N(NULL==m_listener, -11, "FLOWADDLISTEN| msg=new listen fail| clsname=%s", lisnClassName);
    int epfd = m_hepo->getEPfd();
    int ret = HEpBase::Notify(m_listener, HEPNTF_INIT_PARAM, port, svrhost, lqueue, epfd); // Listen类的初始化

    return ret;
}

int FlowCtrl::appendTask( HEpBase* tsk, int qidx, int delay_ms )
{
    ERRLOG_IF1RET_N(NULL == tsk || qidx >= m_tskqNum, -12, "APPENDTASK| msg=append task[%d] fail| tsk=%p", qidx, tsk);
    bool bret = m_tskq[qidx].addTask(tsk, delay_ms);
    return bret? 0: -13;
}

int FlowCtrl::notifyExit( void )
{
    SwitchHand::Instance()->notifyExit();
    for (int i = 0; i < m_tskqNum; ++i)
    {
        m_tskq[i].setExit();
    }

    return 0;
}

void FlowCtrl::uninit( void )
{
    IFDELETE(m_hepo);
    IFDELETE_ARR(m_tskq);
    IFDELETE(m_listener);
    m_tskqNum = 1;
}

int FlowCtrl::run( bool& bexit )
{
    int ret = m_hepo? m_hepo->run(bexit): -7; // 调用后不返回, 一直到需要exit
    // 等待Listen资源退出……

    HEpBase::Notify(m_listener, HEFG_PROGEXIT, 0); /// #PROG_EXITFLOW(2)

    for (int i = 0; i < m_tskqNum; ++i)
    {
        m_tskq[i].unInit();
    }

    RemoteMgr::Instance()->uninit();
    SwitchHand::Instance()->join();
	CliMgr::Instance()->progExitHanele(0);
    m_hepo->unInit();

    return ret;
}

