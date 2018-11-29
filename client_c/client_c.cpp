#include <thread>
#include <csignal>
#include "client_c.hpp"
#include "cloudapp.h"
#include "config_mgr.h"
#include "listen.h"
#include "provd_mgr.h"
#include "svrconsumer.h"
#include "tcp_invoker_mgr.h"
#include "shttp_invoker_mgr.h"
#include "climanage.h"
#include "msgprop.h"
#include "svr_stat.h"
#include "asyncprvdmsg.h"
#include "cloud/switchhand.h"
#include "comm/hepoll.h"
#include "comm/strparse.h"
#include "cloud/msgid.h"

namespace client_c
{

struct SdkMember
{
    HEpoll hepo;
    Listen listen;
    string mainconf;
    std::thread* pthread;

    bool bCloudApp;
    bool bProvider;
    bool bInvoker;

    bool bExit;

    SdkMember(): pthread(NULL), bCloudApp(false), bProvider(false), 
        bInvoker(false), bExit(false) {}
} gsdk;


// 初始化，连接cppcloud-serv，下载主配置文件
int Init( const string& appName, const string& servHostPort, int appid /*=0*/ )
{
    int ret;

    if (appid > 0)
    {
        CloudApp::Instance()->setSvrid(appid);
    }

    gsdk.hepo.init();

    
    ret = CloudApp::Instance()->init(gsdk.hepo.getEPfd(), servHostPort, appName);
    if (0 == ret)
    {
        gsdk.bCloudApp = true;
        gsdk.mainconf = CloudApp::Instance()->getMConf();

        if (CloudApp::Instance()->isInitOk())
        {
            if (!gsdk.mainconf.empty())
            {
                ret = ConfigMgr::Instance()->initLoad(gsdk.mainconf);
            }
        }
        else
        {
            ret = -6;
        }
    }
    
    return ret;
}


// 加载配置指定文件(download)
int LoadConfFile( const string& fname )
{
    return ConfigMgr::Instance()->initLoad(fname);
}

string GetMConfName( void )
{
    return gsdk.mainconf;
}

// 配置查询T类型限于 [string, int, map<string,string>, map<string,int>, vector<string>, vector<int>]
template<class T> int Query( T& oval, const string& fullqkey, bool wideVal )
{
    return ConfigMgr::Instance()->query(oval, fullqkey, wideVal);
}

template int Query<string>(string&, const string&, bool);
template int Query<int>(int&, const string&, bool);
template int Query<map<string,string>>(map<string,string>&, const string&, bool);
template int Query<map<string,int>>(map<string,int>&, const string&, bool);
template int Query<vector<string>>(vector<string>&, const string&, bool);
template int Query<vector<int>>(vector<int>&, const string&, bool);

void SetConfChangeCallBack( CONF_CHANGE_CB cb )
{
    ConfigMgr::Instance()->setChangeCallBack(cb);
}

int InitTcpProvider( int listenPort )
{
    return InitTcpProvider("0.0.0.0", listenPort, 100);
}

int InitTcpProvider( const string& host, int listenPort, int lqueue )
{
    int ret;

    ret = gsdk.listen.init(host, listenPort, lqueue, gsdk.hepo.getEPfd());
    gsdk.bProvider = true;
    return ret;
}

// 针对CMD_TCP_SVR_REQ命令的处理函数
int AddProvdFunction( CMD_HAND_FUNC func )
{
    return AddCmdFunction(CMD_TCP_SVR_REQ, func);
}

int AddCmdFunction( unsigned cmdid, CMD_HAND_FUNC func )
{
    IOHand::AddCmdHandle(cmdid, func);
    return 0;
}

// 可在io线程中调用此方法比ProvdSendMsgAsync更高效
int ProvdSendMsg( const msg_prop_t* msgprop, const string& msg )
{
    int ret = -1;
    // 检查目标是否存在
    IOHand* iohand = (IOHand*)msgprop->iohand;
    if (CliMgr::Instance()->getCliInfo(iohand)) // 有多线程风险
    {
        ret = iohand->sendData(msgprop->cmdid | CMDID_MID, msgprop->seqid, msg.c_str(), msg.length(), true);
    }

    return ret;
}

// 异步发送(线程安全)
int ProvdSendMsgAsync( const msg_prop_t* msgprop, const string& msg )
{
    static ASyncPrvdMsg asyncPrvd;
    int ret = asyncPrvd.pushMessage(msgprop, msg);

    return ret;
}

// param: protocol  tcp=1 udp=2 http=3 https=4
int RegProvider( const string& regname, int prvdid, short protocol, const string& url )
{
    return ProvdMgr::Instance()->regProvider(regname, prvdid, protocol, url);
}

int RegProvider( const string& regname, int prvdid, short protocol, int port, const string& path )
{
    static const int PROTOCOL_NUM = 4;
    static const string protArr[PROTOCOL_NUM+1] = {"tcp://", "udp://", "http://", "https://", ""};
    
    IFRETURN_N(protocol < 1 || protocol > PROTOCOL_NUM, -1);
    string url;
    string localip = CloudApp::Instance()->getLocalIP();
    url = protArr[protocol-1] + localip + ":" + _N(port) + path;
    
    return ProvdMgr::Instance()->regProvider(regname, prvdid, protocol, url);
}

void setUrl( const string& regname, int prvdid, const string& url )
{
    ProvdMgr::Instance()->setUrl(regname, prvdid, url);
}

void setDesc( const string& regname, int prvdid, const string& desc )
{
    ProvdMgr::Instance()->setDesc(regname, prvdid, desc);
}

void setWeight( const string& regname, int prvdid, short weight )
{
    ProvdMgr::Instance()->setWeight(regname, prvdid, weight);
}

void setVersion( const string& regname, int prvdid, short ver )
{
    ProvdMgr::Instance()->setVersion(regname, prvdid, ver);
}

void setEnable( const string& regname, int prvdid, bool enable )
{
    ProvdMgr::Instance()->setEnable(regname, prvdid, enable);
}

void AddProviderStat( const string& regname, int prvdid, bool isOk, int dcount )
{
    //ProvdMgr::Instance()->addOkCount(regname, prvdid, dcount);
    SvrStat::Instance()->addPrvdCount(regname, isOk, prvdid, 0, dcount);
}

int PostOut( const string& regname, int prvdid )
{
    return ProvdMgr::Instance()->postOut(regname, prvdid);
}

int PostEnable( const string& regname, int prvdid, bool enable ) // 为避免与postOut重载引2义
{
    return ProvdMgr::Instance()->postOut(regname, prvdid, enable);
}

int InitInvoker( const string& svrList )
{
    gsdk.bInvoker = true;
    return SvrConsumer::Instance()->init(svrList);
}

void SetRefreshTimeOut( int sec )
{
    SvrConsumer::Instance()->setRefreshTO(sec);
}

void SetReportStatTime( int sec )
{
    SvrStat::Instance()->setDelaySecond(sec);
}

int GetSvrPrvd( svr_item_t& pvd, const string& svrname )
{
    return SvrConsumer::Instance()->getSvrPrvd(pvd, svrname); 
}

// 更新接口调用的统计信息
void AddInvokerStat( const svr_item_t& pvd, bool isOk, int dcount )
{
    return SvrConsumer::Instance()->addStat(pvd, isOk, dcount);
}

int TcpRequest( string& resp, const string& reqmsg, const string& svrname )
{
    return TcpInvokerMgr::Instance()->request(resp, reqmsg, svrname);
}

int HttpGet( string& resp, const string& path, const string& queryStr, const string& svrname )
{
    return SHttpInvokerMgr::Instance()->get(resp, path, queryStr, svrname);
}

int HttpPost( string& resp, const string& path, const string& reqBody, const string& svrname )
{
    return SHttpInvokerMgr::Instance()->post(resp, path, reqBody, svrname);
}

void _Run( void )
{
    CloudApp::Instance()->setEpThreadID();
    gsdk.hepo.run(gsdk.bExit);
}

int NotifyExit( void* ptr ) // 收到Serv通知本应用退出
{
    static int count = 0;
    gsdk.bExit = true;
    
    LOGDEBUG("CLIENTC_NOTIFYEXIT| msg=notify exit| count=%d", count);
    SwitchHand::Instance()->notifyExit();
    
    if (gsdk.pthread && count < 2)
    {
        pthread_kill(gsdk.pthread->native_handle(), SIGTERM);
        ++count;
    }

    return 0;
}

int Run( bool runBackgroud )
{
    int ret;

    ret = CloudApp::Instance()->setToEpollEv();
    ERRLOG_IF1RET_N(ret, ret, "RUN| msg=cloudapp setto ep fail %d", ret);

    CloudApp::Instance()->setNotifyCB("exit", NotifyExit);
    SwitchHand::Instance()->init(gsdk.hepo.getEPfd());
    if (runBackgroud)
    {
        gsdk.pthread = new std::thread(_Run);
        ret = 0;
    }
    else
    {
        ret = gsdk.hepo.run(gsdk.bExit);
    }

    return ret;
}

void Destroy( void )
{
    
    SwitchHand::Instance()->join();
    if (gsdk.pthread)
    {
        gsdk.pthread->join();
        IFDELETE(gsdk.pthread);
    }

    if (gsdk.bInvoker)
    {
        SvrConsumer::Instance()->uninit();
    }

    if (gsdk.bProvider)
    {
        CliMgr::Instance()->progExitHanele(0);
        ProvdMgr::Instance()->uninit();
    }
    
    if (gsdk.bCloudApp)
    {
        ConfigMgr::Instance()->uninit();
        SwitchHand::Instance()->notifyExit();
        SwitchHand::Instance()->join();
        CloudApp::Instance()->uninit();
    }

    gsdk.hepo.unInit();
}

};