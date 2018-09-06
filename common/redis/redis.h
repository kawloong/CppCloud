#ifndef _REDIS_H_
#define _REDIS_H_
#include <map>
#include <list>
#include <string>
#include <hiredis/hiredis.h>  

using std::map;
using std::list;
using std::string;

const int g_cmd_maxlength = 4096;

enum redis_conf_defaullt_val
{
    DEF_REDIS_CONNECTTIMEOUT = 2,
    DEF_REDIS_CMDTIMEOUT = 3,
    DEF_REDIS_INITCONNNUM = 1,
};

enum _redis_err_
{
    ERR_RDS_INNER_SYS = 100, // 内部错误
    ERR_RDS_PARAM_INPUT,     // 输入参数有误
    ERR_RDS_DATA_TOOBIG,     // 数据量过大
    ERR_RDS_INIT_FAIL,       // 初始化失败
    ERR_RDS_DONOT_INIT,      // 未初始化
    ERR_RDS_DUP_INIT,        // 重复初始化
    ERR_RDS_LOADCONF,        // 加载配置文件失败
    ERR_RDS_PARSE_TAG,       // 解析<tag>失败
    ERR_RDS_INVALID_NAME,    // 无效池名字

    // redis error
    ERR_DISCONNECT = 200, // 连接失败
    ERR_CMD_REPLY, 
    ERR_REPLY_NULL, 
    ERR_REPLY_TYPE,
    ERR_64DECODE,         // base64解码失败
    ERR_REDIS_EMPTYPOOL, // 池中无可用连接
    ERR_REDIS_CONN,      // 创建连接失败
};

/************************************************************************
format: xml configure
<redises>
<redis redisname=myredis1 host='127.0.0.1' port='6379' />
<redis redisname='myredis2' host='127.0.0.1' port=6379 processMaxConnNum=1 initConnNum=1 />
</redises>
*************************************************************************/

struct redis_pool_conf_t
{
    const char* redisname;
    const char* host;
    const char* pswd; // 验证密码
    short port;
    short selectno; // 分区
    int processMaxConnNum; // 最大连接数
    int initConnNum; // 进程初始启动时创建的长连接数, 取值[0-processMaxConnNum]
    int connectTimeout;// 连接超时 (sec unit)
    int cmdTimeout;
};

class Redis
{
public:

    Redis(redis_pool_conf_t* pRedisConf);
	~Redis();

    void settime(time_t t = 0);
    time_t gettime(void);

    // reutnr: 0 已连接; 1 断开或未连接; 2 出错
    int getstate(void) const;
    int reconnect(void);

	//static Redis* getInstance();
    int set(const char* key, const char* value);
    int get(string& value, const char* key, bool errlog = false); // errlog=当不存在时打印error log

    // 链表操作
    int rpush(const char* key, const char* value, bool addQuote=false);
    int lpop(string& val, const char* key);
    int llen(unsigned int& len, const char* key);
    int lrange(list<string>& result, const char* key, int beg, int end);
    int lindex(string& val, const char* key, int idx);
    int lset(const char* key, int idx, const char* value);
    int lpush(const char* key, const char* value, bool addQuote=false);
    int rpop(string& val, const char* key);

#ifdef ENABLE_BASE64_VALUE // 对data中的数据base64处理存取;
    // 设置二进制数据的方法
    int set_ex(const char* key, const void* data, unsigned int len, int expire = -1);
    int get_ex(void*& data, unsigned int& len, const char* key); // 注意: data非空返回时由调用者释放
    int rpush_ex(const char* key, const void* data, unsigned int len);
#endif

    // undefine
    int hset(const char* key, const char* field, const char* value);
    int hget(string& value, const char* key, const char* field);
    int hgetall(const char* key, map<string, string>& hashdata);

	int hmset(const char* key, const char* strhash);
    int hmset(const char* key, const map<string, string>& hashdata);

    // 删除操作
	int del(const char* key);

    // 设置超时
    int expire(const char* key, int sec);

    // 权限认证
    int auth(const char* password);

    // debug method
    int test_cmd( const char* cmd );
    int exec_vcmd(const char* from, redisReply*& r, char trytime, const char* cmdFormat, ...);

private:
    int _exec_cmd(const char* from, redisReply*& r, const char* cmd, char trytime = 2);
    inline int _exec_base64_cmd(const char* from, int r_type, const char* r_str,
        string& strcmd, const void* data, unsigned int len, const char* key, int expire);

    int settimeout(int timeout_s);
    int select_no(int no);

public:
    void* parent;
    bool inpool; // 池中长连接

private:
	redis_pool_conf_t* pRedisConf_;
	redisContext* pserver;
    time_t lastUserTime;//创建或获取连接时的时间，单位秒。

};

#endif //_redis_H_
