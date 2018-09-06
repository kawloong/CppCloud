#include <string.h>
#include "comm/public.h"
#include "comm/strparse.h"
#include "redis.h"

#ifdef ENABLE_BASE64_VALUE // 对data中的数据base64处理存取;
#include "comm/base64.h"
#endif


// 连接时超时时间包括: connectTimeout [+ cmdTimeout 如果需selectno]
Redis::Redis(redis_pool_conf_t* pRedisConf): parent(NULL), inpool(false), pRedisConf_(pRedisConf), pserver(NULL)
{
    reconnect();

	if (pserver && pserver->err)
    {  
		redisFree(pserver);
		pserver = NULL;
	}
}

int Redis::getstate(void) const
{
    if (NULL == pserver)
    {
        return 1;
    }
    else
    {
        return pserver->err;
    }
}

void Redis::settime(time_t t)
{
    lastUserTime = ( (0==t)? time(NULL) : t );
}

time_t Redis::gettime(void)
{
    return lastUserTime;
}

// value参数字符串内不能有空格
int Redis::set(const char* key, const char* value)
{
    int ret;
    redisReply* r = NULL;

    do 
    {
        string strcmd("set ");
        IFBREAK_N(NULL==key || NULL==value, ERR_RDS_PARAM_INPUT);

        strcmd.append(key);
        strcmd.append(" ");
        strcmd.append(value);

        ret = _exec_cmd("REDIS_SET", r, strcmd.c_str());
        IFBREAK(ret);
        ERRLOG_IF1BRK(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK") == 0), ERR_REPLY_TYPE,
            "REDIS_SET| msg=set %s %s fail| r.type=%d| r.str=%s", key, value, r->type, r->str);
        //LOGDEBUG("ok to execute set key=%s|value=%s",key, value);  
    }
    while (0);
	
    if (r)
    {
        freeReplyObject(r);
    }

	return ret;
}

int Redis::get(string& value, const char* key, bool errlog)
{
    int ret;
    string strcmd("get ");
    redisReply* r = NULL;

    strcmd.append(key);
    ret = _exec_cmd("REDIS_GET", r, strcmd.c_str());

    if (0 == ret)
    {
        value.clear();
        if (r->type == REDIS_REPLY_STRING)
        {
            value = (r->str);
        }
        else if(r->type == REDIS_REPLY_NIL)
        {
            if (errlog)
            {
                LOGERROR("error to execute get key=%s|value=null", key);
            }
        }
        else
        {
            LOGERROR("error to execute get key=%s", key);
            ret = ERR_REPLY_TYPE;
        }
    }

    if (r)
    {
        freeReplyObject(r);  
    }

	return ret;
}

#ifdef ENABLE_BASE64_VALUE // 对data中的数据base64处理存取;
int Redis::set_ex( const char* key, const void* data, unsigned int len, int expire )
{
    string strcmd;

    strcmd.append("set ");
    strcmd.append(key);
    strcmd.append(" ");

    return _exec_base64_cmd("SET_EX", REDIS_REPLY_STATUS, "OK", strcmd, data, len, key, expire);
}

// @param: data [out] 接收到的指针需调用方free释放
int Redis::get_ex( void*& data, unsigned int& len, const char* key )
{
    int ret = 0;
    redisReply* r = NULL;
    data = NULL;
    len = 0;

    do 
    {
        IFBREAK_N(NULL==key, ERR_PARAM_INPUT);

        string strcmd("get ");
        strcmd.append(key);
        ret = _exec_cmd("GETEX", r, strcmd.c_str());
        IFBREAK(ret);

        if (REDIS_REPLY_NIL == r->type)
        {
            LOGDEBUG("REDIS_HGETEX| msg=hget %s return null", key);
            ret = 0;
            break;
        }

        ERRLOG_IF1BRK(r->type!=REDIS_REPLY_STRING, ERR_REPLY_TYPE,
            "REDIS_HGETEX| msg=reply type| r->type=%d| key=%s| err=%s",
            r->type, key, pserver->errstr);


        //unsigned int outlen = 0;
        //char* pvalue = Tools::getInstance()->base64_decode(r->str, (int)strlen(r->str), &outlen);
        void* outbuf = NULL;
        int deslen = Base64::Decode(r->str, (int)strlen(r->str), &outbuf);
        ERRLOG_IF1BRK(deslen<0, ERR_64DECODE, "REDIS_HGETEX| msg=reply base64 decode fail| "
            "r->type=%d| key=%s| err=%s| deslen=%d", r->type, key, pserver->errstr, deslen);

        LOGDEBUG("REDIS_HGETEX| msg=get %s return %u bytes", key, deslen);

        // IFFREE(pvalue); 需由外部释放
        data = outbuf;
        len = deslen;
        ret = 0;
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}

int Redis::rpush_ex( const char* key, const void* data, unsigned int len )
{
    string strcmd;

    strcmd.append("rpush ");
    strcmd.append(key);
    strcmd.append(" ");

    return _exec_base64_cmd("RPUSH_EX", REDIS_REPLY_INTEGER, NULL, strcmd, data, len, key, -1);
}
#endif

// 注意如果value中有空格,会分拆成多条 // addQuote - nowork
int Redis::rpush(const char* key, const char* value, bool addQuote)
{
    int ret;
    redisReply* r = NULL;

    do 
    {
        string strcmd("rpush ");
        IFBREAK_N(NULL==key || NULL==value, ERR_RDS_PARAM_INPUT);

        strcmd.append(key);
        strcmd.append(" ");
        //if (addQuote) strcmd.append("\"");
        strcmd.append(value);
        //if (addQuote) strcmd.append("\"");

        ret = _exec_cmd("REDIS_RPUSH", r, strcmd.c_str());
        IFBREAK(ret);
        ERRLOG_IF1BRK(!(r->type == REDIS_REPLY_INTEGER), ERR_REPLY_TYPE,
            "REDIS_RPUSH| msg=rpush %s %s fail| r.type=%d| r.str=%s", key, value, r->type, r->str);
        //LOGDEBUG("ok to execute rpush key=%s|value=%s",key, value);  
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}

int Redis::lpop( string& val, const char* key )
{
    int ret;
    redisReply* r = NULL;

    do
    {
        IFBREAK_N(NULL==key, ERR_RDS_PARAM_INPUT);
        string strcmd = StrParse::Format("lpop %s", key);

        ret = _exec_cmd("REDIS_LPOP", r, strcmd.c_str());
        IFBREAK(ret);

        ERRLOG_IF0BRK(r->type==REDIS_REPLY_STRING , ERR_REPLY_TYPE,
            "REDIS_LPOP| msg=%s fail| r.type=%d| r.str=%s", strcmd.c_str(), r->type, r->str);
        val = r->str? r->str: "";
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}


int Redis::hset(const char* key, const char* field, const char* value)
{
    int ret;
    redisReply* r = NULL;

    do 
    {
        string strcmd("hset ");
        IFBREAK_N(NULL==key || NULL==field || NULL==value, ERR_RDS_PARAM_INPUT);

        strcmd += string(key) + " " + field + " " + value;
        ret = _exec_cmd("REDIS_HSET", r, strcmd.c_str());

        IFBREAK(ret);
        ERRLOG_IF0BRK(r->type == REDIS_REPLY_INTEGER, ERR_REPLY_TYPE,
            "REDIS_SET| msg=%s fail| r.type=%d| r.str=%s", strcmd.c_str(), r->type, r->str);

        LOGDEBUG("ok to execute %s", strcmd.c_str());
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}

int Redis::hget(string& value, const char* key, const char* field)
{
    int ret;
    redisReply* r = NULL;

    do 
    {
        string strcmd("hget ");
        IFBREAK_N(NULL==key || NULL==field, ERR_RDS_PARAM_INPUT);

        strcmd.append(key);
        strcmd.append(" ");
        strcmd.append(field);

        ret = _exec_cmd("REDIS_HGET", r, strcmd.c_str());
        IFBREAK(ret);

        value.clear();
        if (REDIS_REPLY_STRING == r->type)
        {
            value = r->str;
        }
        else
        {
            ERRLOG_IF0BRK(r->type==REDIS_REPLY_NIL, ERR_REPLY_TYPE,
                "REDIS_HMSET| msg=hget %s %s fail| r.type=%d| r.str=%s", key, field, r->type, r->str);
        }

        LOGDEBUG("ok to execute hget key=%s| value=%s",key, value.c_str());  
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}

/*
Redis> HGETALL people
1) "jack"          # 域
2) "Jack Sparrow"  # 值
3) "gump"
4) "Forrest Gump"
*/
int Redis::hgetall(const char* key, map<string, string>& hashdata)
{
    int ret;
    redisReply* r = NULL;

    do 
    {
        string strcmd("hgetall ");
        IFBREAK_N(NULL==key, ERR_RDS_PARAM_INPUT);

        strcmd.append(key);

        ret = _exec_cmd("REDIS_HGETALL", r, strcmd.c_str());
        IFBREAK(ret);

        char* filed = NULL;
        for (unsigned int i = 0; i < r->elements; ++i)
        {  
            redisReply* childReply = r->element[i];  

            //get命令返回的数据类型是string 对于不存在key的返回值，其类型为REDIS_REPLY_NIL
            if (REDIS_REPLY_STRING == childReply->type)
            {
                if (NULL == filed)
                {
                    filed = childReply->str;
                }
                else
                {
                    hashdata[filed] = childReply->str;
                    filed = NULL;
                }
            }
        }

        LOGDEBUG("ok to execute hgetall key=%s| field_count=%d", key, (int)r->elements);
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}


// hashdata中的值不能有空格存在
int Redis::hmset(const char* key, const map<string, string>& hashdata)
{
    string strhash;

    map<string, string>::const_iterator it = hashdata.begin();
    for(; it != hashdata.end(); ++it)
    {
        strhash.append(" ");
        strhash.append(it->first);
        strhash.append(" ");
        strhash.append(it->second);
    }

    return hmset(key, strhash.c_str());
}

// strhash格式: field1 value1 field2 value2 ...
int Redis::hmset(const char* key, const char* strhash)
{
    int ret;
    redisReply* r = NULL;

    do 
    {
        string strcmd("hmset ");
        IFBREAK_N(NULL==key || NULL==strhash, ERR_RDS_PARAM_INPUT);

        strcmd.append(key);
        strcmd.append(" ");
        strcmd.append(strhash);

        ret = _exec_cmd("REDIS_HMSET", r, strcmd.c_str());
        IFBREAK(ret);

        ERRLOG_IF0BRK(r->type==REDIS_REPLY_STATUS && 0==strcasecmp(r->str,"OK"), ERR_REPLY_TYPE,
            "REDIS_HMSET| msg=hmset %s %s fail| r.type=%d| r.str=%s", key, strhash, r->type, r->str);
        LOGDEBUG("ok to execute hmset key=%s|strhash=%s",key, strhash);  
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}



Redis::~Redis()
{
    if (pserver)
    {  
        redisFree(pserver);
        pserver = NULL;
    }
}

int Redis::reconnect( void )
{
    if (pserver)
    {
        redisFree(pserver);
    }

    struct timeval timeout;
    timeout.tv_sec = pRedisConf_->connectTimeout;
    timeout.tv_usec = 0;

    //pserver= redisConnect(pRedisConf_->host, pRedisConf_->port);
    pserver= redisConnectWithTimeout(pRedisConf_->host, pRedisConf_->port, timeout);
    if (pserver)
    {
        int ret;

        ret = auth(pRedisConf_->pswd);
        ERRLOG_IF1(ret, "REDIS_OPT| msg=auth fail| name=%s| ret=%d| err=%s",
            pRedisConf_->redisname, ret, pserver->errstr);

        ret = settimeout(pRedisConf_->cmdTimeout); // 经测试: 会立即返回
        ERRLOG_IF1(ret, "REDIS_OPT| msg=set cmd timeout fail| name=%s| ret=%d| err=%s",
            pRedisConf_->redisname, ret, pserver->errstr);
        ret = select_no(pRedisConf_->selectno); // 经测试: 会阻塞上面设置的时间
        if (ret)
        {
            LOGERROR("REDIS_OPT| msg=selectno fail| name=%s(%s:%u)| ret=%d| err=%s",
                pRedisConf_->redisname, pRedisConf_->host, pRedisConf_->port, ret, pserver->errstr);
            return ret;
        }
    }

    return (pserver && 0 == pserver->err)? 0 : 1;
}

int Redis::auth(const char* password)
{
   int ret = 0;
   redisReply* r = NULL;

    if (NULL != password && 0 != password[0])
    {
        r = (redisReply*)redisCommand(pserver, "auth %s", password);

        if (r)
        {
            //ret = ( REDIS_REPLY_STATUS == r->type && 0 == strcasecmp(r->str,"OK") )? 0 : 1;
            freeReplyObject(r);
        }
        else
        {
            ret = ERR_REPLY_NULL;
        }
    }
    
    return ret;
}

int Redis::select_no(int no)
{
    int ret = 0;
    redisReply* r = NULL;

    if (no > 0)
    {
        char cmd[16];
        snprintf(cmd, sizeof(cmd), "select %d", pRedisConf_->selectno);
        // 会受redisSetTimeout影响
        r = (redisReply*)redisCommand(pserver, cmd); // success return: REDIS_REPLY_STATUS+"OK"

        if (r)
        {
            ret = ( REDIS_REPLY_STATUS == r->type && 0 == strcasecmp(r->str,"OK") )? 0 : 1;
            freeReplyObject(r);
        }
        else
        {
            ret = ERR_REPLY_NULL;
        }
    }
    
    return ret;
}

int Redis::settimeout(int timeout_s)
{
    struct timeval tv;
    tv.tv_sec = timeout_s;
    tv.tv_usec = 0;

    int ret = (REDIS_OK == redisSetTimeout(pserver, tv))? 0: 1;
    return ret;
}

int Redis::test_cmd( const char* cmd )
{
    /* hiredis.h define:
    #define REDIS_REPLY_STRING 1
    #define REDIS_REPLY_ARRAY 2
    #define REDIS_REPLY_INTEGER 3
    #define REDIS_REPLY_NIL 4
    #define REDIS_REPLY_STATUS 5
    #define REDIS_REPLY_ERROR 6
    */

    int ret;
    redisReply* r = NULL;

    ret = _exec_cmd("TEST", r, cmd, 1);
    if (r)
    {
        LOGDEBUG("RedisTest| cmd=%s| ret=%d| r.type=%d| r.str=%s| rdserr=%d(%s)",
            cmd, ret, r->type, r->str, pserver->err, pserver->errstr);
        freeReplyObject(r);
    }
    else
    {
        LOGDEBUG("RedisTest| cmd=%s| ret=%d| r=null", cmd, ret);
    }

    return 0;
}

int Redis::_exec_cmd(const char* from, redisReply*& r, const char* cmd, char trytime)
{
    int ret = -1;
    char cnt = 0;
    string strcmd(cmd);

    while (++cnt <= trytime)
    {
        if (getstate())
        {
            ret = reconnect();
            ERRLOG_IF1BRK(ret, ERR_DISCONNECT, "%s| msg=reconnect fail| ret=%d| err=%s",
                from, ret, pserver->errstr);
        }

        r = (redisReply*)redisCommand(pserver, strcmd.c_str()); 
        if (NULL == r) // try again
        {
            LOGERROR("%s| msg=execute-%d (%s) reply* return null| err=%s", from, cnt, cmd, pserver->errstr);
            reconnect();
            ret = ERR_REPLY_NULL;
            continue;
        }

        ret = 0;
        break;
    }

    return ret;
}


#ifdef ENABLE_BASE64_VALUE // 对data中的数据base64处理存取;
int Redis::_exec_base64_cmd( const char* from, int r_type, const char* r_str,
            string& strcmd, const void* data, unsigned int len, const char* key, int expire )
{
    int ret = 0;
    redisReply* r = NULL;
    char* pvalue = NULL;
    bool sucess;

    do 
    {
        IFBREAK_N(NULL==data || 0==len, ERR_PARAM_INPUT);


        // 以下内存有多次分配的情况,可考虑优化
        //int encode_len = Tools::getInstance()->base64_encode(data, len, &pvalue);
        int encode_len = Base64::Encode(data, len, &pvalue);

        if (encode_len > g_cmd_maxlength-32)
        {
            LOGERROR("%s| msg=encode too long| key=%s| encodelen=%d| rawlen=%u",
                from, key, encode_len, len);
            ret = ERR_DATA_TOOBIG; 
            break;
        }

        strcmd.append(pvalue);

        ret = _exec_cmd(from, r, strcmd.c_str());
        IFBREAK(ret);
        sucess = (r_type == r->type);
        if (sucess && NULL != r_str)
        {
            sucess = (0 == strcasecmp(r->str,"OK"));
        }

        ERRLOG_IF1BRK( !sucess, ERR_REPLY_TYPE,
             "%s| msg=reply type| r.type=%d| r.str=%s| key=%s| err=%s",
            from, r->type, r->str, key, pserver->errstr );

        if (expire > 0)
        {
            char cmdbuff[512];
            snprintf(cmdbuff, sizeof(cmdbuff), "expire %s %d", key, expire);
            freeReplyObject(r);
            r = NULL;
            ret = _exec_cmd(from, r, cmdbuff);
        }
        
        ret = 0;
    }
    while (0);

    IFFREE(pvalue);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}
#endif

int Redis::expire(const char* key, int sec)
{
    int ret = 0;
    redisReply* r = NULL;

    do 
    {
        IFBREAK_N(NULL==key || 0==key[0], ERR_RDS_PARAM_INPUT);

        if (sec > 0)
        {
            char cmdbuff[512];
            snprintf(cmdbuff, sizeof(cmdbuff), "expire %s %d", key, sec);
            ret = _exec_cmd("SET_EXPIRE", r, cmdbuff);
        }
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}

int Redis::del(const char* key)
{
    int ret;
    redisReply* r = NULL;

    do
    {
        string strcmd("del ");
        IFBREAK_N(NULL==key, ERR_RDS_PARAM_INPUT);

        strcmd.append(key);

        ret = _exec_cmd("REDIS_DEL", r, strcmd.c_str());
        IFBREAK(ret);

        ERRLOG_IF0BRK(r->type==REDIS_REPLY_INTEGER , ERR_REPLY_TYPE,
            "REDIS_DEL| msg=%s fail| r.type=%d| r.str=%s", strcmd.c_str(), r->type, r->str);

        //LOGDEBUG("ok to execute hmset key=%s", strcmd.c_str());
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}

int Redis::llen( unsigned int& len, const char* key )
{
    int ret;
    redisReply* r = NULL;

    do
    {
        string strcmd("llen ");
        IFBREAK_N(NULL==key, ERR_RDS_PARAM_INPUT);

        strcmd.append(key);

        ret = _exec_cmd("REDIS_LLEN", r, strcmd.c_str());
        IFBREAK(ret);

        ERRLOG_IF0BRK(r->type==REDIS_REPLY_INTEGER , ERR_REPLY_TYPE,
            "REDIS_LLEN| msg=%s fail| r.type=%d| r.str=%s", strcmd.c_str(), r->type, r->str);

        len = r->integer;
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}

int Redis::lrange( list<string>& result, const char* key, int beg, int end )
{
    int ret;
    redisReply* r = NULL;
    char buff[64];

    do
    {
        string strcmd("lrange ");
        IFBREAK_N(NULL==key, ERR_RDS_PARAM_INPUT);
        snprintf(buff, sizeof(buff), " %d %d", beg, end);
        strcmd += key;
        strcmd += buff;

        ret = _exec_cmd("REDIS_LRANGE", r, strcmd.c_str());
        IFBREAK(ret);

        ERRLOG_IF0BRK(r->type==REDIS_REPLY_ARRAY , ERR_REPLY_TYPE,
            "REDIS_LRANGE| msg=%s fail| r.type=%d| r.str=%s", strcmd.c_str(), r->type, r->str);

        for (unsigned int i = 0; i < r->elements; ++i)
        {  
            redisReply* childReply = r->element[i];  

            //get命令返回的数据类型是string 对于不存在key的返回值，其类型为REDIS_REPLY_NIL
            if (childReply->str)
            {
                result.push_back(childReply->str);
            }
        }
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}

// @return: key不存在或超出最大index里返回ERR_REPLY_TYPE;
int Redis::lindex( string& val, const char* key, int idx )
{
    int ret;
    redisReply* r = NULL;

    do
    {
        IFBREAK_N(NULL==key, ERR_RDS_PARAM_INPUT);
        string strcmd = StrParse::Format("lindex %s %d", key, idx);

        ret = _exec_cmd("REDIS_LINDEX", r, strcmd.c_str());
        IFBREAK(ret);

        ERRLOG_IF0BRK(r->type==REDIS_REPLY_STRING , ERR_REPLY_TYPE,
            "REDIS_LINDEX| msg=%s fail| r.type=%d| r.str=%s", strcmd.c_str(), r->type, r->str);

        val = r->str? r->str: "";
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}

// @return: key不存在或超出最大index里返回ERR_REPLY_TYPE;
int Redis::lset( const char* key, int idx, const char* value )
{
    int ret;
    redisReply* r = NULL;

    do
    {
        IFBREAK_N(NULL==key||NULL==value, ERR_RDS_PARAM_INPUT);
        string strcmd = StrParse::Format("lset %s %d %s", key, idx, value);

        ret = _exec_cmd("REDIS_LSET", r, strcmd.c_str());
        IFBREAK(ret);

        ERRLOG_IF0BRK(r->type==REDIS_REPLY_STATUS && strcasecmp(r->str,"OK") == 0 , ERR_REPLY_TYPE,
            "REDIS_LSET| msg=%s fail| r.type=%d| r.str=%s", strcmd.c_str(), r->type, r->str);
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}


// 注意如果value中有空格,会分拆成多条 // addQuote - nowork
int Redis::lpush(const char* key, const char* value, bool addQuote)
{
    int ret;
    redisReply* r = NULL;

    do 
    {
        string strcmd("lpush ");
        IFBREAK_N(NULL==key || NULL==value, ERR_RDS_PARAM_INPUT);

        strcmd.append(key);
        strcmd.append(" ");
        //if (addQuote) strcmd.append("\"");
        strcmd.append(value);
        //if (addQuote) strcmd.append("\"");

        ret = _exec_cmd("REDIS_LPUSH", r, strcmd.c_str());
        IFBREAK(ret);
        ERRLOG_IF1BRK(!(r->type == REDIS_REPLY_INTEGER), ERR_REPLY_TYPE,
            "REDIS_LPUSH| msg=lpush %s %s fail| r.type=%d| r.str=%s", key, value, r->type, r->str);
        //LOGDEBUG("ok to execute lpush key=%s|value=%s",key, value);  
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}

int Redis::rpop( string& val, const char* key )
{
    int ret;
    redisReply* r = NULL;

    do
    {
        IFBREAK_N(NULL==key, ERR_RDS_PARAM_INPUT);
        string strcmd = StrParse::Format("rpop %s", key);

        ret = _exec_cmd("REDIS_RPOP", r, strcmd.c_str());
        IFBREAK(ret);

        ERRLOG_IF0BRK(r->type==REDIS_REPLY_STRING , ERR_REPLY_TYPE,
            "REDIS_RPOP| msg=%s fail| r.type=%d| r.str=%s", strcmd.c_str(), r->type, r->str);
        val = r->str? r->str: "";
    }
    while (0);

    if (r)
    {
        freeReplyObject(r);
    }

    return ret;
}


// summery: 调用redisCommand() 实现外部可扩展, 可以避免字符串存在空格问题
int Redis::exec_vcmd(const char* from, redisReply*& r, char trytime, const char* cmdFormat, ...)
{
    int ret = -1;
    char cnt = 0;
    va_list ap;

    va_start(ap, cmdFormat);

    while (++cnt <= trytime)
    {
        if (getstate())
        {
            ret = reconnect();
            ERRLOG_IF1BRK(ret, ERR_DISCONNECT, "%s| msg=reconnect fail| ret=%d| err=%s",
                from, ret, pserver->errstr);
        }

        r = (redisReply*)redisvCommand(pserver, cmdFormat, ap); 
        if (NULL == r) // try again
        {
            LOGERROR("%s| msg=execute-%d (%s ..) reply* return null| err=%s", from, cnt, cmdFormat, pserver->errstr);
            reconnect();
            ret = ERR_REPLY_NULL;
            continue;
        }

        ret = 0;
        break;
    }

    va_end(ap);
    return ret;
}

