/*-------------------------------------------------------------------------
FileName     : svrconsumer.h
Description  : 服务发现之消费者管理
remark       : 服务消费者获取服务地址列表
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-02       create     hejl 
-------------------------------------------------------------------------*/
#include <string>
#include <map>
#include <vector>
#include "comm/public.h"
#include "comm/lock.h"


using namespace std;

struct svr_item_t
{
    string url;
    string version;
    string host;
    
    int port;
    int svrid;
    short protocol;
    short weight;

    svr_item_t(): port(0), svrid(0), protocol(0), weight(0) {}
};

class SvrConsumer
{
    struct SvrItem
    {
        vector<svr_item_t> svrItms;
        int weightSum;
        int callcount;
        time_t ctime;

        SvrItem(): weightSum(0), callcount(0) {}
    };

    SINGLETON_CLASS2(SvrConsumer);
    SvrConsumer( void );
    ~SvrConsumer( void );

public:
    static int OnCMD_SVRSEARCH_RSP( void* ptr, unsigned cmdid, void* param );
    static int OnCMD_EVNOTIFY_REQ( void* ptr ); // provider 下线通知

    int onCMD_SVRSEARCH_RSP( void* ptr, unsigned cmdid, void* param );
    int onCMD_EVNOTIFY_REQ( void* ptr ); // provider 下线通知

    int init( const string& svrList );
    void uninit( void );

private:
    int parseResponse( string& msg );

private:
    map<string, SvrItem*> m_allPrvds;
    RWLock m_rwLock;
    static SvrConsumer* This;
};
