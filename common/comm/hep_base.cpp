#include "hep_base.h"
#include "strparse.h"
#include <map>
#include <string>
#include <sys/epoll.h>


using namespace std;

// 标准生成模板
static map<string, HEpBase*> s_stdobj;
static map<string, HEpBase::ProcOneFunT> s_procfunc;

HEpEvFlag::HEpEvFlag(void): m_epfd(INVALID_FD), m_actFd(INVALID_FD), m_eventFg(0), m_ptr(NULL)
{}

HEpEvFlag::~HEpEvFlag( void )
{
    if (INVALID_FD != m_actFd && m_eventFg)
    {
        LOGWARN("HEPEVDESTRUCT| msg=may be ev still active| fd=%d| evfg=0x%X", m_actFd, m_eventFg);
    }
}

// epoll_wait back, remove flag attached if EPOLLONESHOT enable
void HEpEvFlag::oneShotUpdate( void )
{
    if (EPOLLONESHOT & m_eventFg)
    {
        m_eventFg = 0;
    }
}

void HEpEvFlag::setEPfd( int epfd )
{
	m_epfd = epfd;
}

int HEpEvFlag::setActFd( int actfd )
{
	int ret = 0;
	if (INVALID_FD != m_epfd && INVALID_FD != m_actFd)
	{
		if (actfd != m_actFd) // 先要清理调旧的actFd绑定
		{
			ret = setEvt(0, 0);
		}
	}
	
	if (0 == ret)
	{
		m_actFd = actfd;
	}
	return ret;
}

int HEpEvFlag::addEvt( int eventFg )
{
    return setEvt(eventFg|m_eventFg, m_ptr);
}
int HEpEvFlag::addEvt( int eventFg, ITaskRun2* ptr )
{
    return setEvt(eventFg|m_eventFg, ptr);
}

int HEpEvFlag::rmEvt( int eventFg )
{
    return setEvt(m_eventFg&(~eventFg), m_ptr);
}

int HEpEvFlag::setEvt( int eventFg, ITaskRun2* ptr )
{
    struct epoll_event epevt;
    const char* flowdesc = "";
    int ret = 0;


    if (0 == m_eventFg)
    {
        if (eventFg)
        {
            epevt.events = eventFg;
            epevt.data.ptr = ptr;

            ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_actFd, &epevt);
            flowdesc = "epoll-add";
        }
    }
    else // has set event before
    {
        if (0 == eventFg) // remove op
        {
            ret = epoll_ctl(m_epfd, EPOLL_CTL_DEL, m_actFd, NULL);
            ptr = NULL;
            flowdesc = "epoll-del";
        }
        else if (eventFg != m_eventFg || ptr != m_ptr)
        {
            epevt.events = eventFg;
            epevt.data.ptr = ptr;
            ret = epoll_ctl(m_epfd, EPOLL_CTL_MOD, m_actFd, &epevt);
            flowdesc = "epoll-mod";
        }
    }

    if (0 == ret)
    {
        m_eventFg = eventFg;
        m_ptr = ptr;
    }
    else
    {
        LOGERROR("HEPEVENT| msg=%s event fail| epfd=%d| sockfd=%d| ev=%X", flowdesc, m_epfd, m_actFd, eventFg);
    }

    return ret;
}

HEpBase::HEpBase( void ): m_parent(NULL), m_child(NULL)
{

}

HEpBase::HEpBase( const char* name ): m_parent(NULL), m_child(NULL)
{
    RegisterClass(name, this);
}

HEpBase::~HEpBase(void)
{
    IFDELETE(m_child);
    if (m_parent)
    {
        m_parent->m_child = NULL;
    }
}

// 注册类名字不能以下划线开头，不能为空
void HEpBase::RegisterClass(const char* name, HEpBase* stdptr)
{
    if (name && name[0] && '_' != name[0] && stdptr)
    {
        s_stdobj[name] = stdptr;
    }
}

void HEpBase::RegisterFunc(const char* name, ProcOneFunT func)
{
     if (name && name[0] && func)
    {
        s_procfunc[name] = func;
    }   
}

HEpBase::ProcOneFunT HEpBase::GetProcFunc(const char* regname)
{
    if (regname)
    {
        map<string, ProcOneFunT>::iterator it = s_procfunc.find(regname);
        if (it != s_procfunc.end())
        {
            return it->second;
        }
    }

    return NULL;
}

HEpBase* HEpBase::New(const char* name)
{
    if (name)
    {
        map<string, HEpBase*>::iterator it = s_stdobj.find(name);
        if (it != s_stdobj.end())
        {
            return it->second->clone();
        }
    }

    return NULL;
}

void HEpBase::BindSon(HEpBase* parent, HEpBase* son)
{
    if (parent && son)
    {
        if (parent->m_child)
        {
            LOGERROR("BINDSON| msg=child has exist| parent=%s", parent->name());
            //IFDELETE(parent->m_child);
        }
        parent->m_child = son;
        son->m_parent = parent;
    }
}

int HEpBase::Notify(HEpBase* dst, int evtype, ...)
{
    int ret = 0;
    va_list ap;
    va_start(ap, evtype);
    ret = dst->onEvent(evtype, ap);
    va_end(ap);

    return ret;
}

int HEpBase::SendMsgEasy(HEpBase* dst, unsigned int cmdid, unsigned int seqid, int ecode, const string& desc, bool setOutAtonce)
{
    return SendMsg(dst, cmdid, seqid, setOutAtonce, "{ \"code\": %d, \"desc\": \"%s\" }", ecode, desc.c_str());
}

int HEpBase::SendMsg(HEpBase* dst, unsigned int cmdid, unsigned int seqid, const string& strda, bool setOutAtonce)
{
    int ret = Notify(dst, HEPNTF_SEND_MSG, cmdid, seqid, strda.c_str(), (unsigned int)strda.length());
    if (setOutAtonce)
    {
        ret = Notify(dst, HEPNTF_SET_EPOUT, true);
    }

    return ret;
}

int HEpBase::SendMsg(HEpBase* dst, unsigned int cmdid, unsigned int seqid, bool setOutAtonce, const char* bodyfmt, ...)
{
    char* bodybuff = NULL;
    va_list ap;
    va_start(ap, bodyfmt);
    
    int ret = vasprintf(&bodybuff, bodyfmt, ap);
    if (ret >= 0 && bodybuff != NULL)
    {
        ret = Notify(dst, HEPNTF_SEND_MSG, cmdid, seqid, bodybuff, (unsigned int)ret);
    }

    va_end(ap);
    IFFREE(bodybuff);

    if (setOutAtonce)
    {
        ret = Notify(dst, HEPNTF_SET_EPOUT, true);
    }

    return ret;
}

int HEpBase::transEvent( HEpBase* dst, int evtype, va_list ap )
{
    return dst->onEvent(evtype, ap);
}

int HEpBase::onEvent( int evtype, va_list ap )
{
    /*    va_arg(ap, int); */
    return 0;
}

// 怕Notify低效,简单参数可能用此方法传递
int HEpBase::setParam(int param1, int param2)
{
	return 0;
}

