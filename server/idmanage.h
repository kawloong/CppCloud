/******************************************************************* 
 *  summery:        数字ObjectId同步管理类(暂时不用)
 *  author:         hejl
 *  date:           2016-10-17
 *  description:    使用共享内存保证多进程同步访问
 ******************************************************************/
#ifndef _IDMANAGE_H_
#define _IDMANAGE_H_
#include <string>
#include "comm/lock.h"
#include "comm/public.h"

using std::string;
const char SHMDATA_VERSION = 1;
const int VAL_COUNT = 4; 
enum id_index_t
{
    IDX_PROBE, // 探针id
    IDX_ONOFF, // 上下线id
};

class IdMgr
{
    struct ShmData
    {
        char ver;
        short hostid;
        int addsum; // 统计flush后增加了的量
        long long num;
    };

    SINGLETON_CLASS2(IdMgr);
private:
    IdMgr(void);
    ~IdMgr(void);

public:
    int init(int key);
    void uninit(void);

    int set(long long val);
    long long getObjectId(void);

private:
    key_t m_key;
    int m_shmid; // 共享内存标识;
    ShmData* m_ptr;
    SemLock m_lock;
};

#endif