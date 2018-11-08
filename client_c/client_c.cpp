#include <thread>
#include "client_c.hpp"
#include "cloudapp.h"
#include "config_mgr.h"
#include "listen.h"
#include "provd_mgr.h"
#include "svrconsumer.h"
#include "tcp_invoker_mgr.h"
#include "shttp_invoker_mgr.h"
#include "climanage.h"
#include "cloud/switchhand.h"
#include "comm/hepoll.h"
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

    //SwitchHand::Instance()->init(gsdk.hepo.getEPfd());
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
template<class T> int Query( T& oval, const string& fullqkey )
{
    return ConfigMgr::Instance()->query(oval, fullqkey);
}

template int Query<string>(string&, const string&);
template int Query<int>(int&, const string&);
template int Query<map<string,string>>(map<string,string>&, const string&);
template int Query<map<string,int>>(map<string,int>&, const string&);
template int Query<vector<string>>(vector<string>&, const string&);
template int Query<vector<int>>(vector<int>&, const string&);


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

// param: protocol  tcp=1 udp=2 http=3 https=4
int regProvider( const string& regname, short protocol, const string& url )
{
    return ProvdMgr::Instance()->regProvider(regname, protocol, url);
}

int regProvider( const string& regname, short protocol, int port, const string& path )
{
    static const int PROTOCOL_NUM = 4;
    static const string protArr[PROTOCOL_NUM+1] = {"tcp://", "udp://", "http://", "https://", ""};
    
    IFRETURN_N(protocol < 1 || protocol > PROTOCOL_NUM, -1);
    string url;
    string localip = CloudApp::Instance()->getLocalIP();
    url = protArr[protocol] + localip + ":" + _N(port) + path;
    
    return ProvdMgr::Instance()->regProvider(regname, protocol, url);
}

void setDesc( const string& regname, const string& desc )
{
    ProvdMgr::Instance()->setDesc(regname, desc);
}

void setWeight( const string& regname, short weight )
{
    ProvdMgr::Instance()->setWeight(regname, weight);
}

void setVersion( const string& regname, short ver )
{
    ProvdMgr::Instance()->setVersion(regname, ver);
}

void setEnable( const string& regname, bool enable )
{
    ProvdMgr::Instance()->setEnable(regname, enable);
}

void addOkCount( const string& regname, int dcount )
{
    ProvdMgr::Instance()->addOkCount(regname, dcount);
}

void addNgCount( const string& regname, int dcount )
{
    ProvdMgr::Instance()->addNgCount(regname, dcount);
}

int postOut( const string& regname )
{
    return ProvdMgr::Instance()->postOut(regname);
}

int postOut( const string& regname, bool enable )
{
    return ProvdMgr::Instance()->postOut(regname, enable);
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

int GetSvrPrvd( svr_item_t& pvd, const string& svrname )
{
    return SvrConsumer::Instance()->getSvrPrvd(pvd, svrname); 
}

int TcpRequest( string& resp, const string& reqmsg, const string& svrname )
{
    return TcpInvokerMgr::Instance()->request(resp, reqmsg, svrname);
}

int HttpGet( string& resp, const string& reqmsg, const string& svrname )
{
    return SHttpInvokerMgr::Instance()->get(resp, reqmsg, svrname);
}

int HttpPost( string& resp, const string& reqmsg, const string& svrname )
{
    return SHttpInvokerMgr::Instance()->post(resp, reqmsg, svrname);
}

void _Run( void )
{
    gsdk.hepo.run(gsdk.bExit);
}

int NotifyExit( void* ptr ) // 收到Serv通知本应用退出
{
    gsdk.bExit = true;
    return 0;
}

int Run( bool runBackgroud )
{
    int ret;

    ret = CloudApp::Instance()->setToEpollEv();
    ERRLOG_IF1RET_N(ret, ret, "RUN| msg=cloudapp setto ep fail %d", ret);

    CloudApp::Instance()->setNotifyCB("exit", NotifyExit);
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