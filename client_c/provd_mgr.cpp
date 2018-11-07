#include "provd_mgr.h"
#include "cloudapp.h"
#include "comm/public.h"
#include "comm/strparse.h"

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

ProviderItem* ProvdMgr::_getProvider( const string& regname )
{
    map<string, ProviderItem*>::const_iterator itr = m_provider_apps.find(regname);
    if (m_provider_apps.end() != itr)
    {
        return itr->second;
    }

    LOGERROR("GETPROVIDER| msg=cannot find provider| regname=%s", regname.c_str());
    return NULL;
}


// 同步阻塞地注册一个服务
int ProvdMgr::regProvider( const string& regname, short protocol, const string& url )
{
    int ret;
    ProviderItem* pvd = m_provider_apps[regname];
    ERRLOG_IF1RET_N(pvd, -60, "REGPROVIDER| msg=reg provider exist| regname=%s", regname.c_str());

    pvd = new ProviderItem;
    pvd->regname = regname;
    pvd->url = url;
    pvd->protocol = protocol;
    pvd->enable = false;

    string resp;
    ret = CloudApp::Instance()->syncRequest(resp, CMD_SVRREGISTER_REQ,
            _F("{\"regname\": \"%s\", \"svrprop\": %s }", regname.c_str(), pvd->jsonStr().c_str()),
            m_timeout_sec);
    if (0 == ret)
    {
        LOGINFO("REGPROVIDER| msg=reg provider %s| response=%s", regname.c_str(), resp.c_str());
        string retcode;
        StrParse::PickOneJson(retcode, resp, "code");
        if (retcode == "0")
        {
            m_provider_apps[regname] = pvd;
            pvd = NULL;
            ret = 0;
        }
        else
        {
            ret = -61;
        }
    }

    IFDELETE(pvd);
    return ret;
}

void ProvdMgr::setDesc( const string& regname, const string& desc )
{
    ProviderItem* pvd = _getProvider(regname);
    if (pvd) pvd->desc = desc;
}
void ProvdMgr::setWeight( const string& regname, short weight )
{
    ProviderItem* pvd = _getProvider(regname);
    if (pvd) pvd->weight = weight;
}
void ProvdMgr::setVersion( const string& regname, short ver )
{
    ProviderItem* pvd = _getProvider(regname);
    if (pvd) pvd->version = ver;
}
void ProvdMgr::setEnable( const string& regname, bool enable )
{
    ProviderItem* pvd = _getProvider(regname);
    if (pvd) pvd->enable = enable;
}
void ProvdMgr::addOkCount( const string& regname, int dcount )
{
    ProviderItem* pvd = _getProvider(regname);
    if (pvd) pvd->okcount += dcount;
}
void ProvdMgr::addNgCount( const string& regname, int dcount )
{
    ProviderItem* pvd = _getProvider(regname);
    if (pvd) pvd->ngcount += dcount;
}

int ProvdMgr::postOut( const string& regname )
{
    ProviderItem* pvd = _getProvider(regname);
    ERRLOG_IF1RET_N(NULL == pvd, -63, "POSTPROVIDER| msg=post no provider| "
            "regname=%s", regname.c_str());
    
    int ret = CloudApp::Instance()->postRequest(CMD_SVRREGISTER_REQ,
            _F("{\"regname\": \"%s\", \"svrprop\": %s }", 
            regname.c_str(), pvd->jsonStr().c_str()));
    return ret;
}

int ProvdMgr::postOut( const string& regname, bool enable )
{
    setEnable(regname, enable);
    return postOut(regname);
}