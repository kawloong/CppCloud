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
#include "comm/i_taskrun.h"
#include "svr_item.h"

using namespace std;


class SvrConsumer : public ITaskRun2
{
    struct SvrItem
    {
        vector<svr_item_t> svrItms;
        int weightSum;
        int callcount;
        time_t ctime;

        SvrItem(): weightSum(0), callcount(0) {}
        void rmBySvrid( int svrid, int prvdid );
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

    // interface ITaskRun2
    virtual int qrun( int flag, long p2 );
    virtual int run(int p1, long p2);

    int init( const string& svrList );
    void uninit( void );
    void setRefreshTO( int sec );

    // 获取一个服务提供者信息，用于之后发起调用。
    int getSvrPrvd( svr_item_t& pvd, const string& svrname );
    // 更新接口调用的统计信息
    void addStat( const svr_item_t& pvd, bool isOk, int dcount=1 );

private:
    int parseResponse( string& msg );
    int parseResponse( const void* ptr );

    int appendTimerq( void );
    int _postSvrSearch( const string& regname ) const;

private:
    map<string, SvrItem*> m_allPrvds;
    int m_refresh_sec;
    RWLock m_rwLock;
    bool m_inqueue;


    static SvrConsumer* This;
};

#endif