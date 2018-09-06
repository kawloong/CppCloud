/******************************************************************* 
 *  summery: 管理sql连接池集合
 *  author:  hejl
 *  date:    2016-04-10
 *  description: 以redisname为标识管理多个RedisConnPool对象
 ******************************************************************/ 

#ifndef _REDISPOOLADMIN_H_
#define _REDISPOOLADMIN_H_
#include <map>
#include "comm/lock.h"
#include "redis.h"

using namespace std;
class RedisConnPool;


/******************************************************************
文件依赖  : comm/lock.h comm/public.h tinyxml/xx.h  hiredis/hiredis.h
so依赖    : libhiredis.so

类依赖关系: RedisConnPoolAdmin  --> RedisConnPool   -->   Redis     -->  redisContext
            缓存连接池管理(单例)    缓存连接池            缓存类         Redis api接口
            包括多个池对象          池中有多个缓存类      封装Redis操作  hiredis实现
使用示例:
        Redis* rds;
        const char redisfileconf[] = "/etc/bmsh_redis_connpoll_conf.xml"; // 来自xml配置
        // program begin
        assert( RedisConnPoolAdmin::Instance()->LoadPoolFromFile(redisfileconf) );

        // ... 业务流程 ...
        if (0 == RedisConnPoolAdmin::Instance()->GetConnect(rds, "redis1"))
        {
            // 调用Redis相关方法
            rds->set("test_key1", "value1");
            // ...

            RedisConnPoolAdmin::Instance()->ReleaseConnect(rds);
        }
        // ... 业务流程 ...

        // program end
        RedisConnPoolAdmin::Instance()->DestroyPool(NULL);
*******************************************************************/

class RedisConnPoolAdmin
{
public:
	static RedisConnPoolAdmin* Instance(void); // sigleton
	
public:
	/***************************************
	@summery: 从配置文件解析池信息, 加载m_all_conf中
    @param1:  file 来自xml中读入的配置, 默认是"bmsh_redis_connpoll_conf.xml"
	@return: 0 成功执行;
	**************************************/
	int LoadPoolFromFile(const char* file);

    /***************************************
	@summery: 注册一个连接池,每个进程池通过conf->poolname标识
    @param1:  conf 来自xml中读入的配置
    @param2:  delay 是否延时创建池, 暂不实现
	@return: 0 成功执行; 1 参数出错; 2 poolname出错; 3 已注册过; 4 创建池失败
    **************************************/
    int LoadPoolFromRun(const redis_pool_conf_t* conf);
	
	/***************************************
	@summery: 根据池名字销毁一个连接池
	@param1:  redisname 要销毁的连接池名
	@return: 0 成功执行; 其他出错; 
	**************************************/	
	int DestroyPool(const char* redisname);
	
	/***************************************
	@summery: 根据池名字,获得池内一个Redis连接
	@param1:  predis [out] 接收返回的Redis对象指针
	@param2:  redisname [in] 要销毁的连接池名
	@return:  0 成功执行; 
	@remark:  GetConnect与ReleaseConnect要求成对使用
	**************************************/	
	int GetConnect(Redis*& predis, const char* redisname);
	
	/***************************************
	@summery: 释放一个由GetConnect接收到的连接对象
	@param1:  predis 连接对象, 由GetConnect获得
	@return:  0 成功执行; 其他出错; 
	@remark:  GetConnect与ReleaseConnect要求成对使用
	**************************************/	
	int ReleaseConnect(Redis* predis);
	
public:
	// 打印连接池的当前状态
	void showPoolStatus(const char* redisname);
    void showPoolStatus(string& strbak, const char* redisname);
	
private:
	/***************************************
	@summery: 注册一个连接池,每个进程池通过conf->redisname标识
    @param1:  conf 来自xml中读入的配置
    @param2:  delay 是否延时创建池 // 暂不实现
	@return: 0 成功执行; 1 参数出错; 2 redisname出错; 3 已注册过; 4 创建池失败
	**************************************/
    int RegisterPool(redis_pool_conf_t* conf);
    int RegisterPool(void);

protected: // 单实例对象
	RedisConnPoolAdmin(void);
	~RedisConnPoolAdmin(void);

private:
    ThreadLock m_lock;
	map<string, RedisConnPool*> m_conn_pools;
    map<string, redis_pool_conf_t*> m_all_conf; // 非启动时初始化的池
	
};

#endif //
