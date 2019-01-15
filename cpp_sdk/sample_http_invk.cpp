/*-------------------------------------------------------------------------
FileName     : sample_http_invk.cpp
Description  : sdk使用样例 -- 服务消费者
remark       : 编译命令 g++ -Wall -g -std=c++11 sample_http_invk.cpp  -o sample_http_invk libsdk_cppcloud.so  ../common/libhocomm.so
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-24       create     hejl 
-------------------------------------------------------------------------*/
#include "client_c.hpp"
#include <csignal>
#include <iostream>

static const string appName = "TestHttpInvk";
static const string invkName = "httpApp1";

static void sigdeal(int signo)
{
    printf("Signal happen %d\n", signo);
    client_c::NotifyExit(NULL);
}

int main( int argc, char* argv[] )
{
    if (argc < 2)
    {
        printf("usage: sample_http_invk <serv_ip:serv_port>\n");
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

    if ( (ret = client_c::InitInvoker(invkName)) )
    {
        printf("InitInvoker fail %d\n", ret);
        return -3;
    }

    client_c::SetRefreshTimeOut(10); // 刷新提供端的时间间隔

    ret = client_c::Run(true);
    printf("Run return %d\n", ret);

    // 调用服务提供者的接口
    string resp;
    string req = "param1=value1&param2=v2";
    ret = client_c::HttpGet(resp, "", req, invkName);
    printf("Call [%s] response: %s\n", invkName.c_str(), resp.c_str());

    string line;
    while (getline(std::cin, line)) // 测试运行时改变配置，app能实时得知变化
    {
        if (line == "q")
        {
            kill(0, SIGTERM);
            break;
        }

        req = string("param1=") + line;
        ret = client_c::HttpGet(resp, "", req, invkName);
        printf("http Resp-%d: %s\n", ret, resp.c_str());
    }

    client_c::Destroy();
    return 0;
}

