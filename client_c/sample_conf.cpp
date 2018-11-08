/*-------------------------------------------------------------------------
FileName     : sample_conf.cpp
Description  : sdk使用样例 -- 分布式配置
remark       : 编译命令 g++ -Wall -g -std=c++11 sample_conf.cpp  -o sample_conf  ../common/libhocomm.so libsdk_cppcloud.so
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-08       create     hejl 
-------------------------------------------------------------------------*/
#include "client_c.hpp"
#include <csignal>

static const string appName = "TestConf";
static const string testConfKey = "/key1";

static void sigdeal(int signo)
{
    client_c::NotifyExit(NULL);
}

int main( int argc, char* argv[] )
{
    if (argc < 2) return -1;
    string serv = argv[1];

    int ret = client_c::Init(appName, serv);

    if (ret)
    {
        printf("Init fail %d, serv=%s\n", ret, serv.c_str());
        return -2;
    }

    string mainConfFile = client_c::GetMConfName();
    printf("Main Configue File is %s\n", mainConfFile.c_str());
    
    string oval;
    ret = client_c::Query(oval, testConfKey);
    printf("Queue: %s=%s\n", testConfKey.c_str(), oval.c_str());

    signal(SIGINT, sigdeal);
    signal(SIGTERM, sigdeal);

    ret = client_c::Run(false);
    printf("Run return %d\n", ret);

    client_c::Destroy();
    return 0;
}