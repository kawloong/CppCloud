/*-------------------------------------------------------------------------
FileName     : svrconsumer.h
Description  : 服务发现之消费者管理
remark       : 服务消费者获取服务地址列表
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-02       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _SVRCONSUMER_H_
#define _SVRCONSUMER_H_
#include <string>
#include <map>
#include <vector>
#include "comm/public.h"
#include "comm/lock.h"
#include "svr_item.h"

using namespace std;


class SvrConsumer
{
    struct SvrItem
    {
        vector<svr_item_t> svrItms;
        int weightSum;
        int callcount;
        time_t ctime;

        SvrItem(): weightSum(0), callcount(0) {}
        void rmBySvrid( int svrid );
        svr_item_t* randItem( void );
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
    void setRefreshTO( int sec );

    // 获取一个服务提供者信息，用于之后发起调用。
    int getSvrPrvd( svr_item_t& pvd, const string& svrname );

private:
    int parseResponse( string& msg );
    int parseResponse( const void* ptr );

private:
    map<string, SvrItem*> m_allPrvds;
    int m_refresh_sec;
    RWLock m_rwLock;
    static SvrConsumer* This;
};

#endif