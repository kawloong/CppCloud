#ifndef _REDISPOOL_H_
#define _REDISPOOL_H_
#include <list>
#include "redis.h"
#include "comm/lock.h"

using std::list;

class RedisConnPool
{
public:
	/***************************************
	@summery: 获取池内一个连接, 通过Redis输出参数返回
	@param1:  Redis [out] 调用者传入接收的指针
	@param2:  only_pool [in] 是否仅限返回池中长连接; 当true时,无长连接时返回短连接
	@return:  0 成功执行; 其他出错; 
	@remark:  getConnect和relConnect必须匹配使用,外部不要对其delete
	**************************************/	
	int getConnect(Redis*& rds, bool only_pool = false);
	
	/***************************************
	@summery: 释放回一个连接
	@param1:  rds [in] 由getConnect获得的指针
	@return:  0 成功执行; 其他出错; 
	@remark:  getConnect和relConnect必须匹配使用,外部不要对其delete
	**************************************/	
	void relConnect(Redis* rds, bool check_connect) ; // 释放连接

public:
	RedisConnPool(void);
	~RedisConnPool(void);
	
	// 初始化池对象
	int init(redis_pool_conf_t* conf);
	// 反初始化池对象
	void uninit(void);

    // 查看对象运行状态信息
    void trace_stat(string& msg, bool includeFreeConn);

private:
	inline bool can_create_poolconn(void); // 判断是否有连接余额
	int init_pool(unsigned int count);
	
private: //const method
	Redis* create_connect(void) const;         // 创建一个连接
	void destroy_connect(Redis* p) const;      // 销毁一个连接
	
private:
	redis_pool_conf_t* m_conf;
	int m_conn_count_process;    // 进程内的已开的常连接数
	int m_conn_max_process;      // 进程内的池中允许最大连接数
	//int m_freeconn_count;      // 进程内连接池中的可用连接数(可由m_freeconn_list.size()得到)
	int m_totalconn_count;       // 进程内当前总活动连接数(包括短连接和长连接)
	int m_wait_timeout;
    int m_peek_conn_count;       // 进程内连接数峰值(某时间)
    static const int DEFAULT_TIMEOUT = 600;

	bool m_inited;               // 是否已初始化
	ThreadLock m_lock;           //线程锁
	list<Redis*> m_freeconn_list; //空闲连接
};

#endif //