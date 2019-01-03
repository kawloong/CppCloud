/*-------------------------------------------------------------------------
FileName     : sample_prvd.cpp
Description  : sdk使用样例 -- 服务提供者
remark       : 编译命令 g++ -Wall -g -std=c++11 sample_prvd.cpp  -o sample_prvd libsdk_cppcloud.so  ../common/libhocomm.so
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-08       create     hejl 
-------------------------------------------------------------------------*/
#include "client_c.hpp"
#include "msgprop.h"
#include <csignal>
#include <thread>
#include <unistd.h>

static const string appName = "TestPrvd";
static int listenPort = 4900;

int TcpProviderHandle(msg_prop_t*, const char*);

static void sigdeal(int signo)
{
    printf("Signal happen %d\n", signo);
    client_c::NotifyExit(NULL);
}

int main( int argc, char* argv[] )
{
    if (argc < 2)
    {
        printf("usage: sample_prvd <serv_ip:serv_port>\n");
        return -1;
    }

    string serv = argv[1];
    int ret = client_c::Init(appName, serv);

    if (ret)
    {
        printf("Init fail %d, serv=%s\n", ret, serv.c_str());
        return -2;
    }

    signal(SIGINT, sigdeal);
    signal(SIGTERM, sigdeal);

    srand(time(NULL));
    listenPort += rand()%1000;
    if ( (ret = client_c::InitTcpProvider(listenPort)) )
    {
        printf("InitTcpProvider fail %d\n", ret);
        return -3;
    }

    client_c::AddProvdFunction(TcpProviderHandle);
    if ( (ret = client_c::RegProvider(appName, 1, 1, listenPort)) )
    {
        printf("RegProvider fail %d\n", ret);
        return -4;
    }

    client_c::PostEnable(appName, 1, true);

    ret = client_c::Run(false);
    printf("Run return %d\n", ret);

    client_c::Destroy();
    return 0;
}

void threadProcess( msg_prop_t msgprop, const string& body )
{
    // ... doing job here, maybe stay long time ...
    sleep(1);

    string echo = string("{\"code\": 0, \"desc\": \"debug provider handle(thread)\", \"echo_data\": ") + body + "}";
    client_c::ProvdSendMsgAsync(&msgprop, echo);

    client_c::AddProviderStat(appName, 1, true);
}

// 服务提供处理
int TcpProviderHandle( msg_prop_t* msgprop, const char* body )
{
    printf("Provider| msg=%s\n", body);

    // do the job yourself hear
    /// ...

    static int i = 0;
    if ( (++i&0x1) == 1)
    {
        std::thread th(threadProcess, *msgprop, string(body));
        th.detach();
        return 0;
    }
    // resp back to consumer
    string echo = string("{\"code\": 0, \"desc\": \"debug provider handle\", \"echo_data\": ") + body + "}";
    client_c::ProvdSendMsgAsync(msgprop, echo);
    
    client_c::AddProviderStat(appName, 1, true);

    return 0;
}