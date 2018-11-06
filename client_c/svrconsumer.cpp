#include "svrconsumer.h"
#include "rapidjson/json.hpp"
#include "comm/strparse.h"
#include "cloud/msgid.h"
#include "cloud/homacro.h"
#include "cloud/exception.h"
#include "cloudapp.h"

// 把url里的主机和端口解析出来
bool svr_item_t::parseUrl( void )
{
    // url example: http://192.168.1.12:8000/path
    string tmp;
    int ret = StrParse::SplitURL(tmp, host, port, tmp, tmp, url);
    return 0 == ret;
}

void SvrConsumer::SvrItem::rmBySvrid( int svrid )
{
    vector<svr_item_t>::iterator it0 = svrItms.begin();
    for (; it0 != svrItms.end(); )
    {
        vector<svr_item_t>::iterator it = it0++;
        if (svrid == it->svrid)
        {
            this->weightSum -= it->weight;
            svrItms.erase(it);
        }
    }
}

svr_item_t* SvrConsumer::SvrItem::randItem( void )
{
    IFRETURN_N(svrItms.empty(), NULL);
    IFRETURN_N(weightSum <= 1, &svrItms[0]);
    
    int nrd = rand()%weightSum;
    int tmpsum = 0;
    vector<svr_item_t>::iterator it0 = svrItms.begin();
    for (; it0 != svrItms.end(); ++it0)
    {
        svr_item_t* pitm = &(*it0);
        tmpsum += pitm->weight;
        if (tmpsum > nrd)
        {
            return pitm;
        }
    }

    LOGWARN("RANDSVR| msg=logic err flow| nrd=%d| weightSum=%d| tmpsum=%d",
            nrd, weightSum, tmpsum);
    return &svrItms[0];
}

SvrConsumer::SvrConsumer( void )
{
    This = this;
    m_refresh_sec = 5*60;
}

SvrConsumer::~SvrConsumer( void )
{
    uninit();
}

int SvrConsumer::OnCMD_SVRSEARCH_RSP( void* ptr, unsigned cmdid, void* param )
{
    return This->onCMD_SVRSEARCH_RSP(ptr, cmdid, param);
}

int SvrConsumer::OnCMD_EVNOTIFY_REQ( void* ptr ) // provider 下线通知
{
    return This->onCMD_EVNOTIFY_REQ(ptr);
}

int SvrConsumer::onCMD_SVRSEARCH_RSP( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false);
    int ret = parseResponse(&doc); 

    return ret;
}

int SvrConsumer::onCMD_EVNOTIFY_REQ( void* ptr )
{
    const Document* doc = (const Document*)ptr;
    RJSON_GETSTR_D(notify, doc);
    RJSON_GETSTR_D(regname, doc);
    RJSON_GETINT_D(svrid, doc);

    ERRLOG_IF1RET_N(notify!="provider_down" || 0==svrid, -113, 
        "EVNOTIFY| msg=%s", Rjson::ToString(doc).c_str());
    
    RWLOCK_WRITE(m_rwLock);
    map<string, SvrItem*>::iterator it = m_allPrvds.find(regname);
    if (it != m_allPrvds.end())
    {
        it->second->rmBySvrid(svrid);
    }

    return 0;
}

// @param: svrList 是空格分隔启动时需要获得的服务, 服务和版本间冒号分开
int SvrConsumer::init( const string& svrList )
{
    static const char seperator = ' ';
    static const char seperator2 = ':';
    static const int timeout_sec = 3;
    vector<string> vSvrName;

    int ret = StrParse::SpliteStr(vSvrName, svrList, seperator);
    ERRLOG_IF1RET_N(ret, -110, "CONSUMERINIT| msg=splite to vector fail %d| svrList=%s", 
        ret, svrList.c_str());

    vector<string>::const_iterator cit = vSvrName.begin();
    for (; cit != vSvrName.end(); ++cit)
    {
        vector<string> vSvrNver;
        StrParse::SpliteStr(vSvrNver, *cit, seperator2);

        const string& svrname = vSvrNver[0];
        int ver = (2 == vSvrNver.size()) ? atoi(vSvrNver[1].c_str()) : 1;

        string resp;
        ret = CloudApp::Instance()->syncRequest(resp, CMD_SVRSEARCH_REQ,
                _F("{\"regname\": \"%s\", \"%s\": %d, \"bookchange\": 1}", 
                    svrname.c_str(), ver), 
                timeout_sec); 
        IFBREAK(ret);

        ret = parseResponse(resp);
        IFBREAK(ret);
    }

    if (0 == ret)
    {
        // srand(time(NULL))
        CloudApp::Instance()->setNotifyCB("provider_down", OnCMD_EVNOTIFY_REQ);
        ret = CloudApp::Instance()->addCmdHandle(CMD_SVRSEARCH_RSP, OnCMD_SVRSEARCH_RSP);
    }

    return ret;
}

int SvrConsumer::parseResponse( string& msg )
{
    Document doc;
    ERRLOG_IF1RET_N(doc.ParseInsitu((char*)msg.data()).HasParseError(), -111, 
        "COMSUMERPARSE| msg=json invalid");
    
    return parseResponse(&doc);
}

int SvrConsumer::parseResponse( const void* ptr )
{
    const Document* doc = (const Document*)ptr;

    RJSON_GETINT_D(code, doc);
    RJSON_GETSTR_D(desc, doc);
    const Value* pdata = NULL;
    Rjson::GetArray(&pdata, "data", doc);
    ERRLOG_IF1RET_N(0 != code || NULL == pdata, -112, 
        "COMSUMERPARSE| msg=resp fail %d| err=%s", code, desc.c_str());
    
    int ret = 0;
    Value::ConstValueIterator itr = pdata->Begin();
    string regname;
    SvrItem* prvds = new SvrItem;
    prvds->ctime = time(NULL);
    for (; itr != pdata->End(); ++itr)
    {
        svr_item_t svitm;
        const Value* node = &(*itr);
        if (regname.empty())
        {
            RJSON_GETSTR(regname, node);
        }
        
        RJSON_VGETSTR(svitm.url, "url", node);
        RJSON_VGETINT(svitm.svrid, "svrid", node);
        RJSON_GETINT_D(weight, node);
        RJSON_GETINT_D(protocol, node);
        bool validurl = svitm.parseUrl();
        ERRLOG_IF1BRK(!validurl, -113, "COMSUMERPARSE| msg=invalid url found| "
            "url=%s| svrname=%s", svitm.url.c_str(), regname.c_str());

        svitm.weight = weight;
        svitm.protocol = protocol;
        prvds->weightSum += weight;
        prvds->svrItms.push_back(svitm);
    }

    if (ret)
    {
        IFDELETE(prvds);
    }
    else
    {
        RWLOCK_WRITE(m_rwLock);
        SvrItem* oldi = m_allPrvds[regname];
        IFDELETE(oldi);
        m_allPrvds[regname] = prvds;        
    }

    return ret;
}

void SvrConsumer::uninit( void )
{
    RWLOCK_WRITE(m_rwLock);
    map<string, SvrItem*>::iterator it = m_allPrvds.begin();
    for (; it != m_allPrvds.end(); ++it)
    {
        delete it->second;
    }

    m_allPrvds.clear();
}

 void SvrConsumer::setRefreshTO( int sec )
 {
     m_refresh_sec = sec;
 }

int SvrConsumer::getSvrPrvd( svr_item_t& pvd, const string& svrname )
{
    RWLOCK_READ(m_rwLock);
    
}
