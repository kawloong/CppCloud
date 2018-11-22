/*-------------------------------------------------------------------------
FileName     : agent_prvd.cpp
Description  : 代理的服务提供者 -- 即非本进程提供服务，本进程仅代理注册
remark       : 编译命令 g++ -Wall -g -std=c++11 agent_prvd.cpp  -o agent_prvd libsdk_cppcloud.so ../common/libhocomm.so
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-21       create     hejl 
-------------------------------------------------------------------------*/
#include "client_c.hpp"
#include <csignal>
#include <map>
#include <iostream>

using std::map;
static const string appName = "AgentPrvd";
static const string confFile = "agent_prvd.json";

static void sigdeal( int signo )
{
    printf("sig happen\n");
    client_c::NotifyExit(NULL);
}

static void conf_change_callback( const string& confname )
{

    map<string, string> oval;
    int idx = 0;
    int ret = 0;
    
    for (;;)
    {
        ret = client_c::Query(oval, testConfKey, true);
        if (ret || oval.empty()) break;

        string regname = oval["regname"];
        string url = oval["url"];
        string desc = oval["desc"];
        int protocol = atoi(oval["protocol"].c_str());
        int weight = atoi(oval["weight"].c_str());
        bool enable = (oval["enable"] == "true" || oval["enable"] == "1");

        if (!regname.empty() && !url.empty())
        {
            client_c::RegProvider(regname, protocol, url);
            client_c::setDesc(regname, desc);
            client_c::setWeight(regname, weight);
            client_c::setEnable(regname, enable);
            client_c::PostOut(regname);
        }

        oval.clear();
    }
    
    printf("Queue: size=%d, ret=%d\n", (int)oval.size(), ret);
}

int main( int argc, char* argv[] )
{
    if (argc < 2) 
    {
        printf("usage: agent_prvd <serv_ip:serv_port>\n");
        return -1;
    }

    int ret;
    string serv = argv[1];

    // 服务启动初始化，传入CloudServ的地址
    if ( (ret = client_c::Init(appName, serv)) )
    {
        printf("Init fail %d, serv=%s\n", ret, serv.c_str());
        return -2;
    }

    // 加载后应用所需要的配置文件
    if ( (ret = client_c::LoadConfFile(confFile)) )
    {
        printf("LoadConfFile fail %d, confFile=%s\n", ret, confFile.c_str());
        return -3;
    }
    
    conf_change_callback("");

    client_c::SetConfChangeCallBack(conf_change_callback);
    signal(SIGINT, sigdeal);
    signal(SIGTERM, sigdeal);

    // 直到应用需要退出才返回
    ret = client_c::Run(false);
    printf("Run return %d\n", ret);

    client_c::Destroy();
    printf("prog exit\n");
    return 0;
}