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
#include <iostream>

static const string appName = "TestConf";
static const string testConfKey = "/author";
static const string testConfKey2 = "test2.json/keyTest2";

static void sigdeal(int signo)
{
    printf("sig happen\n");
    client_c::NotifyExit(NULL);
}

int main( int argc, char* argv[] )
{
    if (argc < 2) 
    {
        printf("usage: sample_conf <serv_ip:serv_port>\n");
        return -1;
    }

    string serv = argv[1];
    int ret = client_c::Init(appName, serv);

    if (ret)
    {
        printf("Init fail %d, serv=%s\n", ret, serv.c_str());
        return -2;
    }

    string mainConfFile = client_c::GetMConfName();
    printf("Main Configue File is %s\n", mainConfFile.c_str());
    client_c::LoadConfFile("test1.json test2.json");
    
    string oval;
    ret = client_c::Query(oval, testConfKey, true);
    printf("Queue: %s => %s\n", testConfKey.c_str(), oval.c_str());
    ret = client_c::Query(oval, testConfKey2, true);
    printf("Queue: %s => %s\n", testConfKey2.c_str(), oval.c_str());

    signal(SIGINT, sigdeal);
    signal(SIGTERM, sigdeal);

    ret = client_c::Run(true);
    printf("Run return %d\n", ret);

    string line;
    printf("input query-key to get value (or 'q' to exit demo):\n");
    while (getline(std::cin, line)) // 测试运行时改变配置，app能实时得知变化
    {
        if (line == "q")
        {
            kill(0, SIGTERM);
            break;
        }

        ret = client_c::Query(oval, line, true);
        printf("Query [%s] => %s (%d)\n", line.c_str(), oval.c_str(), ret);
        printf("input query-key to get value (or 'q' to exit demo):\n");
    }

    client_c::Destroy();
    printf("prog exit\n");
    return 0;
}