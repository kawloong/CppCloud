#include "config_mgr.h"
#include "config_json.h"
#include "cloudapp.h"
#include "comm/lock.h"
#include "cloud/exception.h"
#include "cloud/homacro.h"
#include "comm/strparse.h"
#include "cloud/const.h"
#include <vector>

static RWLock g_rwLock0;


ConfigMgr::ConfigMgr( void )
{

}
ConfigMgr::~ConfigMgr( void )
{
    uninit();
}

void ConfigMgr::uninit( void )
{
    RWLOCK_WRITE(g_rwLock0);
    map<string, ConfJson*>::iterator it = m_jcfgs.begin();
    for (; m_jcfgs.end() != it; ++it)
    {
        delete it->second;
    }

    m_jcfgs.clear();
}

// @summery: 启动时同步下载所需的配置文件到本地
// @param: confName 以:分隔的配置文件名列表
// @return: 0成功；其他失败应退出进程
int ConfigMgr::initLoad( const string& confName )
{
    static const char seperator = ':';
    static const int timeout_sec = 3;
    vector<string> vFname;
    int ret = StrParse::SpliteStr(vFname, confName, seperator);
    ERRLOG_IF1RET_N(ret, -50, "CFGINIT| msg=splite to vector fail %d| confName=%s", ret, confName.c_str());

    vector<string>::const_iterator cit = vFname.begin();
    RWLOCK_WRITE(g_rwLock0);
    for (; cit != vFname.end(); ++cit)
    {
        // 同步地下载完所有配置 （CMD_GETCONFIG_REQ）
        const string& fname = *cit;
        string resp;
        
        ret = CloudApp::Instance()->syncRequest(resp, CMD_GETCONFIG_REQ, 
                _F("{\"%s\": \"%s\", \"%s\": 1}", 
                HOCFG_FILENAME_KEY, fname.c_str(), HOCFG_INCLUDEBASE_KEY), timeout_sec); 
        IFBREAK(ret);

        Document doc;
        ERRLOG_IF1BRK(doc.ParseInsitu((char*)resp.data()).HasParseError(), -51, 
            "CONFINIT| msg=parse jfile fail| name=%s| filelen=%zu", fname.c_str(), resp.length());
        RJSON_GETINT_D(code, &doc);
        ERRLOG_IF1BRK(code || !doc.HasMember("contents"), -52, 
            "CONFINIT| msg=maybe no file exist| name=%s| resp=%s", fname.c_str(), Rjson::ToString(&doc).c_str());
        CloudApp::Instance()->setNotifyCB("cfg_change", OnCMD_EVNOTIFY_REQ);
        m_mainConfName = CloudApp::Instance()->getMConf();

        ConfJson* cjn = m_jcfgs[fname];
        if (NULL == cjn)
        {
            cjn = new ConfJson;
            ret = cjn->update(&doc);
            if (ret)
            {
                IFDELETE(cjn);
                ret = -53;
                break;
            }
            m_jcfgs[fname] = cjn;

            // 订阅变化推送
            ret = CloudApp::Instance()->syncRequest(resp, CMD_BOOKCFGCHANGE_REQ, 
                _F("{\"%s\": \"%s\", \"%s\": \"1\"}", HOCFG_FILENAME_KEY, fname.c_str(), HOCFG_INCLUDEBASE_KEY),
                timeout_sec);
            ERRLOG_IF1(ret, "CONFINIT| msg=book cfgchange fail %d| name=%s", ret, fname.c_str());
        }
        else
        {
            LOGERROR("CONFINIT| msg=nodef flow| name=%s", fname.c_str());
        }
    }

    return ret;
}


int ConfigMgr::OnCMD_EVNOTIFY_REQ( void* ptr )
{
    return This->onCMD_EVNOTIFY_REQ(ptr);
}
int ConfigMgr::onCMD_EVNOTIFY_REQ( void* ptr )
{
    const Document* pdoc = (const Document*)ptr;
    RJSON_GETSTR_D(filename, pdoc);
    RJSON_GETINT_D(mtime, pdoc);

    // 全部配置reload
    {
        RWLOCK_READ(g_rwLock0);
        map<string, ConfJson*>::iterator it = m_jcfgs.begin();
        for (; m_jcfgs.end() != it; ++it)
        {
            if (it->second->getMtime() < mtime)
            {
                CloudApp::Instance()->postRequest(CMD_GETCONFIG_REQ, 
                    _F("{\"%s\": \"%s\", \"%s\": 1, \"%s\": %d}", 
                    HOCFG_FILENAME_KEY, it->first.c_str(), HOCFG_INCLUDEBASE_KEY, 
                    HOCFG_GT_MTIME_KEY, it->second->getMtime()));
            }
        }
    }

    return 0;
}

int ConfigMgr::OnCMD_GETCONFIG_RSP( void* ptr, unsigned cmdid, void* param )
{
    return This->onCMD_GETCONFIG_RSP(ptr, cmdid, param);
}
int ConfigMgr::onCMD_GETCONFIG_RSP( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false);
    int ret = 0;

    do
    {
        string fname;
        RJSON_GETINT_D(code, &doc);
        RJSON_VGETSTR(fname, HOCFG_FILENAME_KEY, &doc);

        ERRLOG_IF1BRK(code || !doc.HasMember("contents"), -52, 
            "CONFCHANGE| msg=maybe no file exist| name=%s| resp=%s", fname.c_str(), Rjson::ToString(&doc).c_str());
        
        RWLOCK_WRITE(g_rwLock0);
        ConfJson* cjn = m_jcfgs[fname];
        if (NULL == cjn)
        {
            cjn =  new ConfJson;
            ret = cjn->update(&doc);
            LOGOPT_EI(ret, "CONFCHANGE| msg=a new cfgfile reach| fname=%s| ret=%d", fname.c_str(), ret);

            if (ret)
            {
                IFDELETE(cjn);
                break;
            }
        }
        else
        {
            cjn->update(&doc);
            _clearCache();
        }
    }
    while(0);

    return 0;
}

// ValT must be [string, int, map<string,string>, map<string,int>, vector<string>, vector<int>]
template<class ValT>
int ConfigMgr::_query( ValT& oval, const string& fullqkey, map<string, ValT >& cacheMap ) const
{
    static const char seperator_ch = '/';
    {
        RWLOCK_READ(g_rwLock0);
        IFRETURN_N(0 == _tryGetFromCache(oval, fullqkey, cacheMap), 0);
    }
    
    string fname;
    string qkey;
    if (!qkey.empty())
    {
        if (seperator_ch == qkey[0]) // 使用主配置
        {
            fname = m_mainConfName;
            qkey = fullqkey;
        }
        else
        {
            string::size_type pos = fullqkey.find(seperator_ch);
            if (pos != string::npos)
            {
                fname = fullqkey.substr(0, pos);
                qkey = fullqkey.substr(pos+1);
            }
        }
    }

    ERRLOG_IF1RET_N(fname.empty() || qkey.empty(), -54, "CONFQUERY| msg=invalid param| fullqkey=%s", fullqkey.c_str());
    int ret = 0;
    {
        RWLOCK_WRITE(g_rwLock0);
        map<string,ConfJson*>::const_iterator it = m_jcfgs.find(fname);
        ERRLOG_IF1RET_N(m_jcfgs.end() == it, -55, "CONFQUERY| msg=invalid filename| fullqkey=%s", fullqkey.c_str());
        ret = it->second->query(oval, qkey);
        // if (0 == ret)
        {
            cacheMap[fullqkey] = oval;
        }
    }

    return ret;
}

void ConfigMgr::_clearCache( void )
{
    m_cacheStr.clear();
    m_cacheInt.clear();
    m_cacheMapStr.clear();
    m_cacheMapInt.clear();
    m_cacheVecStr.clear();
    m_cacheVecInt.clear();
}

// 在缓存当中查询，成功查到返回0，不存在返回1 
template<class ValT>
int ConfigMgr::_tryGetFromCache( ValT& oval, const string& fullqkey, const map<string, ValT >& cacheMap ) const
{
    int ret = 1;
    typedef typename map<string, ValT >::const_iterator CONST_INTERATOR;

    CONST_INTERATOR itr = cacheMap.find(fullqkey);
    if (cacheMap.end() != itr)
    {
        oval = itr->second;
        ret = 0;
    }

    return ret;
}

int ConfigMgr::query( int& oval, const string& fullqkey )
{
    return _query(oval, fullqkey, m_cacheInt);
}
int ConfigMgr::query( string& oval, const string& fullqkey )
{
    return _query(oval, fullqkey, m_cacheStr);
}

int ConfigMgr::query( map<string, string>& oval, const string& fullqkey )
{
    return _query(oval, fullqkey, m_cacheMapStr);
}
int ConfigMgr::query( map<string, int>& oval, const string& fullqkey )
{
    return _query(oval, fullqkey, m_cacheMapInt);
}
int ConfigMgr::query( vector<string>& oval, const string& fullqkey )
{
    return _query(oval, fullqkey, m_cacheVecStr);
}
int ConfigMgr::query( vector<int>& oval, const string& fullqkey )
{
    return _query(oval, fullqkey, m_cacheVecInt);
}
