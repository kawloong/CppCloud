#include "shttp_invoker_mgr.h"
#include "svrconsumer.h"
#include "cloud/msgid.h"
#include "comm/strparse.h"
#include "comm/simplehttp.h"


SHttpInvokerMgr::SHttpInvokerMgr( void )
{
    m_eachLimitCount = 5;
    m_invokerTimOut_sec = 3;
}

SHttpInvokerMgr::~SHttpInvokerMgr( void )
{
}

void SHttpInvokerMgr::setLimitCount( int n )
{
     m_eachLimitCount = n;
}

string SHttpInvokerMgr::adjustUrlPath( const string& url, const string& path ) const
{
    if (path.empty()) return url;
    if (url.empty()) return path;
    char ch1 = *url.rbegin();
    char ch2 = *path.begin();

    string ret;
    if ('/' == ch1)
    {
        ret = url + (('/'==ch2)? path.substr(1) : path);
    }
    else
    {
        ret = url + (('/'==ch2)? "": "/") + path;
    }

    return ret;
}

// example: http://ip:port/method/pa?param1=v1&param2=v2
// param: path="/pa"; qstr="param1=v1&param2=v2"
int SHttpInvokerMgr::get( string& resp, const string& path, const string& qstr, const string& svrname )
{
    int ret;
    svr_item_t pvd;

    ret = SvrConsumer::Instance()->getSvrPrvd(pvd, svrname);
    ERRLOG_IF1RET_N(ret, ret, "GETPROVIDER| msg=getSvrPrvd fail %d| svrname=%s", ret, svrname.c_str());

    do
    {
        string hostp = _F("%s:%p", pvd.host.c_str(), pvd.port);
        CSimpleHttp http(adjustUrlPath(pvd.url, path) + "?" + qstr);
        http.setTimeout(m_invokerTimOut_sec*1000);
        ret = http.doGet();
        ERRLOG_IF1BRK(ret, ret, "GETPROVIDER| msg=http.doGet fail %d(%s)| svrname=%s", 
                ret, http.getErrMsg().c_str(), svrname.c_str());

        resp = http.getResponse();
        string status = http.getHttpStatus();
        ret = (status == "200" ? 0: -116);
        ERRLOG_IF1(ret, "GETPROVIDER| msg=http status is %s| svrname=%s| err=%s", 
                status.c_str(), svrname.c_str(), http.getErrMsg().c_str());        
    }
    while(0);

    SvrConsumer::Instance()->addStat(pvd, 0 == ret);
    return ret;
}


int SHttpInvokerMgr::post( string& resp, const string& path, const string& reqbody, const string& svrname )
{
    int ret;
    svr_item_t pvd;

    ret = SvrConsumer::Instance()->getSvrPrvd(pvd, svrname);
    ERRLOG_IF1RET_N(ret, ret, "GETPROVIDER| msg=getSvrPrvd fail %d| svrname=%s", ret, svrname.c_str());

    do
    {
        string hostp = _F("%s:%p", pvd.host.c_str(), pvd.port);
        CSimpleHttp http(adjustUrlPath(pvd.url, path));
        http.setTimeout(m_invokerTimOut_sec*1000);
        ret = http.doPost(reqbody);
        ERRLOG_IF1BRK(ret, ret, "GETPROVIDER| msg=http.doPost fail %d| svrname=%s", ret, svrname.c_str());

        resp = http.getResponse();
        string status = http.getHttpStatus();
        ret = (status == "200" ? 0: -116);
        ERRLOG_IF1(ret, "GETPROVIDER| msg=http status is %s| svrname=%s| err=%s", 
                status.c_str(), svrname.c_str(), http.getErrMsg().c_str());        
    }
    while(0);

    SvrConsumer::Instance()->addStat(pvd, 0 == ret);
    return ret;
}