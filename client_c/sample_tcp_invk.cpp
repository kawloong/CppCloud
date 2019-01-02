/*-------------------------------------------------------------------------
FileName     : sample_tcp_invk.cpp
Description  : sdk使用样例 -- 服务消费者
remark       : 编译命令 g++ -Wall -g -std=c++11 sample_tcp_invk.cpp  -o sample_tcp_invk libsdk_cppcloud.so  ../common/libhocomm.so
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-20       create     hejl 
-------------------------------------------------------------------------*/
#include "client_c.hpp"
#include <csignal>
#include <iostream>
#include <thread>

static const string appName = "TestInvk";
static const string invkName = "TestPrvd";

static void sigdeal(int signo)
{
    printf("Signal happen %d\n", signo);
    client_c::NotifyExit(NULL);
}

void thread_func( const string& msg )
{
    string resp;
    int ret = client_c::TcpAioRequest(resp, msg, invkName);
    printf("TcpAio Resp-%d: %s\n", ret, resp.c_str());
}

int main( int argc, char* argv[] )
{
    if (argc < 2)
    {
        printf("usage: sample_tcp_invk <serv_ip:serv_port>\n");
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

    //client_c::SetRefreshTimeOut(10);
    client_c::SetRequestTimeout(10, "");

    ret = client_c::Run(true);
    printf("Run return %d\n", ret);

    // 调用服务提供者的接口
    string resp;
    string req;

    string line;
    printf("please input string to request, q to exit: \n");
    while (getline(std::cin, line)) // 测试运行时改变配置，app能实时得知变化
    {
        if (line == "q")
        {
            kill(0, SIGTERM);
            break;
        }

        req = string("{ \"cmd\": \"") + line + "\"}";
        std::thread th(thread_func, req);
        th.detach();
    }

    client_c::Destroy();
    return 0;
}

