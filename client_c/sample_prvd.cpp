/*-------------------------------------------------------------------------
FileName     : sample_prvd.cpp
Description  : sdk使用样例 -- 服务提供者
remark       : 编译命令 g++ -Wall -g -std=c++11 sample_prvd.cpp  -o sample_prvd  ../common/libhocomm.so libsdk_cppcloud.so
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-08       create     hejl 
-------------------------------------------------------------------------*/
#include "client_c.hpp"
#include <csignal>

static const string appName = "TestPrvd";
static const int listenPort = 4900;

int TcpProviderHandle(msg_prop_t*, const char*);

static void sigdeal(int signo)
{
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

    if ( (ret = client_c::InitTcpProvider(listenPort)) )
    {
        printf("InitTcpProvider fail %d\n", ret);
        return -3;
    }

    client_c::AddProvdFunction(TcpProviderHandle);
    if ( (ret = client_c::regProvider(appName, 1, listenPort)) )
    {
        printf("regProvider fail %d\n", ret);
        return -4;
    }

    client_c::postOut(appName);

    ret = client_c::Run(false);
    printf("Run return %d\n", ret);

    client_c::Destroy();
    return 0;
}

int TcpProviderHandle( msg_prop_t* msgprop, const char* body )
{

}