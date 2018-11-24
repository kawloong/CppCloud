#include "provd_mgr.h"
#include "cloudapp.h"
#include "comm/public.h"
#include "comm/strparse.h"
#include "cloud/const.h"

ProvdMgr::ProvdMgr( void )
{
    m_timeout_sec = 2;
}

void ProvdMgr::uninit( void )
{
    map<string, ProviderItem*>::iterator itr = m_provider_apps.begin();
    for (; itr != m_provider_apps.end(); ++itr)
    {
        IFDELETE(itr->second);
    }
    m_provider_apps.clear();
}

ProviderItem* ProvdMgr::_getProvider( const string& regname, int prvdid )
{
    string key = regname + "%" + _N(prvdid);
    map<string, ProviderItem*>::const_iterator itr = m_provider_apps.find(key);
    if (m_provider_apps.end() != itr)
    {
        return itr->second;
    }

    LOGERROR("GETPROVIDER| msg=cannot find provider| regname=%s", regname.c_str());
    return NULL;
}

// 当cloudapp断开又重连成功后调用
int ProvdMgr::ReconnectNotifyCB( void* param )
{
    return ProvdMgr::Instance()->reconnectNotifyCB(param);
}

int ProvdMgr::reconnectNotifyCB( void* param )
{
    int ret = 0;
    map<string, ProviderItem*>::iterator itr = m_provider_apps.begin();
    for (; itr != m_provider_apps.end(); ++itr)
    {
        ret |= postOut(itr->second->regname, itr->second->prvdid);
    }

    ERRLOG_IF1(ret, "RECONNCB| msg=provd postOut| ret=%d", ret);
    return ret;
}

// 启动阶段，同步阻塞地注册一个服务
int ProvdMgr::regProvider( const string& regname, int prvdid, short protocol, const string& url )
{
    int ret;
    string key = regname + "%" + _N(prvdid);
    ProviderItem* pvd = m_provider_apps[key];
    ERRLOG_IF1RET_N(pvd, -60, "REGPROVIDER| msg=reg provider exist| regname=%s", regname.c_str());

    pvd = new ProviderItem;
    pvd->regname = regname;
    pvd->url = url;
    pvd->prvdid = prvdid;
    pvd->protocol = protocol;
    pvd->enable = false;
    pvd->weight = 100;

    ret = registRequest(pvd);
    if (0 == ret)
    {
        m_provider_apps[key] = pvd;
        pvd = NULL;
    }

    // 重连时，要重新注册
    static bool regCB = false;
    if (!regCB)
    {
        CloudApp::Instance()->setNotifyCB(RECONNOK_NOTIFYKEY, ReconnectNotifyCB);
        regCB = true;
    }

    IFDELETE(pvd);
    return ret;
}

void ProvdMgr::setDesc( const string& regname, int prvdid, const string& desc )
{
    ProviderItem* pvd = _getProvider(regname, prvdid);
    if (pvd) pvd->desc = desc;
}
void ProvdMgr::setWeight( const string& regname, int prvdid, short weight )
{
    ProviderItem* pvd = _getProvider(regname, prvdid);
    if (pvd) pvd->weight = weight;
}
void ProvdMgr::setVersion( const string& regname, int prvdid, short ver )
{
    ProviderItem* pvd = _getProvider(regname, prvdid);
    if (pvd) pvd->version = ver;
}
void ProvdMgr::setEnable( const string& regname, int prvdid, bool enable )
{
    ProviderItem* pvd = _getProvider(regname, prvdid);
    if (pvd) pvd->enable = enable;
}
void ProvdMgr::addOkCount( const string& regname, int prvdid, int dcount )
{
    ProviderItem* pvd = _getProvider(regname, prvdid);
    if (pvd) pvd->okcount += dcount;
}
void ProvdMgr::addNgCount( const string& regname, int prvdid, int dcount )
{
    ProviderItem* pvd = _getProvider(regname, prvdid);
    if (pvd) pvd->ngcount += dcount;
}

// param: noEpFlag 当启动阶段未进入io-epoll复用前传true； 有io-epoll复用的业务里传false
int ProvdMgr::postOut( const string& regname, int prvdid )
{
    ProviderItem* pvd = _getProvider(regname, prvdid);
    ERRLOG_IF1RET_N(NULL == pvd, -63, "POSTPROVIDER| msg=post no provider| "
            "regname=%s", regname.c_str());
    
    int ret = registRequest(pvd);
    return ret;
}

int ProvdMgr::postOut( const string& regname, int prvdid, bool enable )
{
    setEnable(regname, prvdid, enable);
    return postOut(regname, prvdid);
}

int ProvdMgr::registRequest( ProviderItem* pvd ) const
{
    string resp;
    int ret;
    if (!CloudApp::Instance()->isInEpRun()) // 启动时
    {
        int ret1 = CloudApp::Instance()->begnRequest(resp, CMD_SVRREGISTER_REQ,
            _F("{\"regname\": \"%s\", \"svrprop\": %s }", pvd->regname.c_str(), pvd->jsonStr().c_str()),
            false);
        string retcode;
        StrParse::PickOneJson(retcode, resp, "code");
        ret = ("0" == retcode)? 0 : -66;

        LOGOPT_EI(ret, "SVRREGISTER_REQ| msg=regist provider %s| ret1=%d resp=%s| regname=%s", 
                ret?"fail":"success", ret1, resp.c_str(), pvd->regname.c_str());
    }
    else
    {
        ret = CloudApp::Instance()->postRequest(CMD_SVRREGISTER_REQ,
            _F("{\"regname\": \"%s\", \"svrprop\": %s }", pvd->regname.c_str(), pvd->jsonStr().c_str()) );
    }

    return ret;
}