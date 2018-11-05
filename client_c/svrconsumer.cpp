#include "svrconsumer.h"
#include "rapidjson/json.hpp"
#include "comm/strparse.h"
#include "cloud/msgid.h"
#include "cloudapp.h"


SvrConsumer::SvrConsumer( void )
{
    This = this;
}

SvrConsumer::~SvrConsumer( void )
{
    
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

}
int SvrConsumer::onCMD_EVNOTIFY_REQ( void* ptr )
{
    
}

// @param: svrList 是空格分隔启动时需要获得的服务
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

    }

    return ret;
}

int SvrConsumer::parseResponse( string& msg )
{
    Document doc;

    ERRLOG_IF1RET_N(doc.ParseInsitu((char*)msg.data()).HasParseError(), -111, 
        "COMSUMERPARSE| msg=json invalid");
    
    RJSON_GETINT_D(code, &doc);
    RJSON_GETSTR_D(desc, &doc);
    const Value* pdata = NULL;
    Rjson::GetArray(&pdata, "data", &doc);
    ERRLOG_IF1RET_N(0 != code || NULL == pdata, -112, 
        "COMSUMERPARSE| msg=resp fail %d| err=%s", code, desc.c_str());
    
    Value::ConstValueIterator itr = pdata->Begin();
    string regname;
    for (; itr != pdata->End(); ++itr)
    {
        svr_item_t svitm;
        const Value* node = &(*itr);
        if (regname.empty())
        {
            RJSON_GETSTR(regname, regname, node);
        }
        
        RJSON_VGETSTR(svitm.url, "url", node);
        RJSON_GETINT(svitm.svrid, "svrid" node);
        RJSON_GETINT_D(weight, node);
        svitm.weight = weight;
        RJSON_GETINT_D(protocol, node);
        svitm.protocol = protocol;

    }

    return 0;
}

void SvrConsumer::uninit( void )
{
    map<string, SvrItem*>::iterator it = m_allPrvds.begin();
    for (; it != m_allPrvds.end(); ++it)
    {
        delete it->second;
    }

    m_allPrvds.clear();
}

