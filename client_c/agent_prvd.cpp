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

struct SvrOj
{
    string regname;
    int prvdid;
    int tag;

    SvrOj(): prvdid(0), tag(0) {}
};

static void conf_change_callback( const string& confname )
{
    static map<string, SvrOj> sWorkingPrvd;
    static int sTag = 0;
    map<string, string> oval;
    int idx = 0;
    int ret = 0;
    
    for (++sTag; 0 == ret; idx++)
    {
        ret = client_c::Query(oval, confFile + "/" + std::to_string(idx), true);
        if (ret || oval.empty()) break;

        string regname = oval["regname"];
        string url = oval["url"];
        string desc = oval["desc"];
        int prvdid = atoi(oval["prvdid"].c_str());
        int protocol = atoi(oval["protocol"].c_str());
        //string version = oval["version"]
        int weight = atoi(oval["weight"].c_str());
        bool enable = (oval["enable"] == "true" || oval["enable"] == "1");

        if (!regname.empty() && !url.empty() && prvdid > 0)
        {
            string key = regname + "@" + std::to_string(prvdid);
            auto itFind = sWorkingPrvd.find(key);
            if (sWorkingPrvd.end() == itFind)
            {
                client_c::RegProvider(regname, prvdid, protocol, url);
                sWorkingPrvd[key].regname = regname;
                sWorkingPrvd[key].prvdid = prvdid;
            }

            client_c::setUrl(regname, prvdid, url);
            client_c::setDesc(regname, prvdid, desc);
            client_c::setWeight(regname, prvdid, weight);
            client_c::setEnable(regname, prvdid, enable);
            //if (!version.empty()) client_c::setVersion
            if (0 == client_c::PostOut(regname, prvdid))
            {
                sWorkingPrvd[key].tag = sTag;
            }
            else
            {
                printf("client_c::PostOut(%s, %d) fail\n", regname.c_str(), prvdid);
            }
        }
        else
        {
            printf("invalid prvd config: %s:%s:%d\n", regname.c_str(), url.c_str(), prvdid);
        }

        oval.clear();
    }
    
    // 清除已删除的旧状态
    for (auto itMap = sWorkingPrvd.begin(); sWorkingPrvd.end() != itMap;)
    {
        SvrOj& svoj = itMap->second;
        if (svoj.tag < sTag)
        {
            client_c::setEnable(svoj.regname, svoj.prvdid, false);
            itMap = sWorkingPrvd.erase(itMap);
        }
        else
        {
            ++itMap;
        }
    }
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