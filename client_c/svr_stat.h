/*-------------------------------------------------------------------------
FileName     : svr_stat.h
Description  : 服务治理 -- 调用统计
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-22       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _SVR_STAT_H_
#define _SVR_STAT_H_
#include <string>
#include <map>
#include "comm/public.h"
#include "comm/lock.h"
#include "comm/i_taskrun.h"


using namespace std;


class SvrStat : public ITaskRun2
{
    struct CountEntry
    {
        string regname;
        int svrid; // 当作为提供者身份时 可为0
        int prvdid;

        unsigned pvd_ok; // 提供者统计的成员
        unsigned pvd_ng; // 提供者统计的成员

        unsigned ivk_ok; // 调用者统计 全量
        unsigned ivk_ng; // 调用者统计 全量
        unsigned ivk_dok; // 调用者统计 增量
        unsigned ivk_dng; // 调用者统计 增量

        CountEntry(): svrid(0), prvdid(0), pvd_ok(0), pvd_ng(0), 
            ivk_ok(0), ivk_ng(0), ivk_dok(0), ivk_dng(0) {}
        string jsonStr( void ) const;
        void cleanDeltaCount( void );
    };

    SINGLETON_CLASS2(SvrStat);
    SvrStat( void );
    ~SvrStat( void );

public:
    // 服务提供者的统计计数
    void addPrvdCount( const string& regname, bool isOk, int prvdid = 0, int svrid = 0, int dcount = 1 );
    // 调用者（消费者）统计调用信息
    void addInvkCount( const string& regname, bool isOk, int prvdid = 0, int svrid = 0, int dcount = 1 );

    // 设置上报延时 (参数为0时关闭上报消费统计)
    void setDelaySecond( int sec );
    
    // interface ITaskRun2
    virtual int qrun( int flag, long p2 );
    virtual int run(int p1, long p2);

private:
    CountEntry* _getEntry( const string& regname, int svrid, int prvdid );
    int appendTimerq( void );

private:
    RWLock m_rwLock;
    bool m_inqueue;
    int m_delayTimeSec;
    
    map<string, CountEntry> m_stat;

    static SvrStat* This;
};

#endif