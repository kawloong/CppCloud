#include "shttp_invoker_mgr.h"
#include "svrconsumer.h"
#include "cloud/msgid.h"
#include "comm/strparse.h"
#include "comm/simplehttp.h"


SHttpInvokerMgr::SHttpInvokerMgr( void )
{
    m_eachLimitCount = 5;
}

SHttpInvokerMgr::~SHttpInvokerMgr( void )
{
}

void SHttpInvokerMgr::setLimitCount( int n )
{
     m_eachLimitCount = n;
}


int SHttpInvokerMgr::get( string& resp, const string& reqmsg, const string& svrname )
{
    int ret;
    svr_item_t pvd;

    ret = SvrConsumer::Instance()->getSvrPrvd(pvd, svrname);
    ERRLOG_IF1RET_N(ret, ret, "GETPROVIDER| msg=getSvrPrvd fail %d| svrname=%s", ret, svrname.c_str());

    string hostp = _F("%s:%p", pvd.host.c_str(), pvd.port);
    CSimpleHttp http(pvd.url + "?" + reqmsg);
    http.setTimeout(m_invokerTimOut_sec*1000);
    ret = http.doGet();
    ERRLOG_IF1RET_N(ret, ret, "GETPROVIDER| msg=http.doGet fail %d| svrname=%s", ret, svrname.c_str());

    resp = http.getResponse();
    string status = http.getHttpStatus();
    ret = (status == "200" ? 0: -116);
    ERRLOG_IF1(ret, "GETPROVIDER| msg=http status is %s| svrname=%s| err=%s", 
            status.c_str(), svrname.c_str(), http.getErrMsg().c_str());

    return ret;
}


int SHttpInvokerMgr::post( string& resp, const string& reqmsg, const string& svrname )
{
    int ret;
    svr_item_t pvd;

    ret = SvrConsumer::Instance()->getSvrPrvd(pvd, svrname);
    ERRLOG_IF1RET_N(ret, ret, "GETPROVIDER| msg=getSvrPrvd fail %d| svrname=%s", ret, svrname.c_str());

    string hostp = _F("%s:%p", pvd.host.c_str(), pvd.port);
    CSimpleHttp http(pvd.url);
    http.setTimeout(m_invokerTimOut_sec*1000);
    ret = http.doPost(reqmsg);
    ERRLOG_IF1RET_N(ret, ret, "GETPROVIDER| msg=http.doPost fail %d| svrname=%s", ret, svrname.c_str());

    resp = http.getResponse();
    string status = http.getHttpStatus();
    ret = (status == "200" ? 0: -116);
    ERRLOG_IF1(ret, "GETPROVIDER| msg=http status is %s| svrname=%s| err=%s", 
            status.c_str(), svrname.c_str(), http.getErrMsg().c_str());

    return ret;
}