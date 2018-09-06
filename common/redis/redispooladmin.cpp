/******************************************************************* 
 *  summery:     管理Redis连接池集合实现
 *  author:      hejl
 *  date:        2016-04-11
 *  description: 以redisname为标识管理多个RedisConnPool对象
 ******************************************************************/ 
#include <cstring>
#include "comm/public.h"
#include "rapidxml/bmshxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"
#include "redispooladmin.h"
#include "redispool.h"

#define BREAK_CTRL_BEGIN do{
#define BREAK_CTRL_END  }while(0);
#define BREAKIF1(exp) if(exp){break;}
#define BREAKNIF1(exp, n) if(exp){ret=n; break;}

RedisConnPoolAdmin* RedisConnPoolAdmin::Instance(void)
{
	static RedisConnPoolAdmin sig_obj;
	return &sig_obj;
}

RedisConnPoolAdmin::RedisConnPoolAdmin(void)
{
}

RedisConnPoolAdmin::~RedisConnPoolAdmin(void)
{
	DestroyPool(NULL);
}


int RedisConnPoolAdmin::LoadPoolFromFile( const char* file )
{
    int ret = -1;
    int rdscount = 0;
    short public_selectno = 0;

    try
    {
        rapidxml::file<> fl(file); // 解析文件流
        xml_document<> doc; // parser对象
        doc.parse<0>(fl.data()); // 解析xml格式

        xml_node<>* root = doc.first_node();
        if (root)
        {
            public_selectno = BmshXml::attr_int(root, "selectno");
            xml_node<>* redisElement = root->first_node("redis");  //redisname

            while ( redisElement )
            {
                const char* redisname =  BmshXml::attr_strptr(redisElement, "redisname");

                if (redisname && m_all_conf.find(redisname) == m_all_conf.end())
                {
                    redis_pool_conf_t* pRedisConf = (redis_pool_conf_t*)malloc(sizeof(redis_pool_conf_t));

                    //TiXmlAttribute* attributeOfID = redisElement->FirstAttribute();  
                    pRedisConf->redisname =  BmshXml::attr_strdup(redisElement, "redisname");
                    pRedisConf->host = BmshXml::attr_strdup(redisElement, "host");
                    pRedisConf->pswd = BmshXml::attr_strdup(redisElement, "password");
                    pRedisConf->port = BmshXml::attr_int(redisElement, "port");
                    pRedisConf->selectno = BmshXml::attr_int(redisElement, "selectno", public_selectno);
                    pRedisConf->connectTimeout = BmshXml::attr_int(redisElement, "connectTimeout", DEF_REDIS_CONNECTTIMEOUT);
                    pRedisConf->cmdTimeout = BmshXml::attr_int(redisElement, "cmdTimeout", DEF_REDIS_CMDTIMEOUT);
                    pRedisConf->processMaxConnNum = BmshXml::attr_int(redisElement, "processMaxConnNum");
                    pRedisConf->initConnNum = BmshXml::attr_int(redisElement, "initConnNum", DEF_REDIS_INITCONNNUM);
                    if (pRedisConf->initConnNum > pRedisConf->processMaxConnNum)
                    {
                        pRedisConf->initConnNum = pRedisConf->processMaxConnNum;
                    }

                    fprintf(stdout, "redisname=%s|host=%s:%d|maxconn=%d|selectno=%d\n",
                        pRedisConf->redisname, pRedisConf->host, pRedisConf->port,
                        pRedisConf->processMaxConnNum, pRedisConf->selectno);
                    m_all_conf[pRedisConf->redisname] = pRedisConf;

                    ++rdscount;
                }

                redisElement = redisElement->next_sibling("redis");
            }

            ret = 0;
        }
        else
        {
            fprintf(stderr, "parse %s err=no found root\n", file);
            ret = ERR_RDS_PARSE_TAG;
        }
    }
    catch(runtime_error& err)
    {
        ret = ERR_RDS_LOADCONF;
    }
    catch(rapidxml::parse_error& err)
    {
        fprintf(stderr, "parse %s err=%s(%s)\n", file, err.what(), err.where<char>());
        ret = ERR_RDS_PARSE_TAG;
    }

    fprintf(stdout, "redis_conf_init| result=%d| count=%d\n", ret, rdscount);
    //ret = rdscount>0? 0: -1;
    return ret;
}

// @summery: 运行过程中动态加载mysql连接池
int RedisConnPoolAdmin::LoadPoolFromRun( const redis_pool_conf_t* conf )
{
    int ret = 0;

    BREAK_CTRL_BEGIN
    // make sure poolname not confict
    BREAKNIF1(NULL == conf || NULL == conf->redisname, ERR_RDS_PARAM_INPUT);
    
    LockGuard lk(m_lock); // 构造加锁,析构解锁
    BREAKNIF1(m_conn_pools.find(conf->redisname) != m_conn_pools.end(), ERR_RDS_DUP_INIT);
    redis_pool_conf_t* inconf = (redis_pool_conf_t*)malloc(sizeof(redis_pool_conf_t));
    inconf->port = conf->port;
    inconf->processMaxConnNum  = conf->processMaxConnNum;
    inconf->initConnNum = conf->initConnNum;
    inconf->selectno = conf->selectno;
    inconf->connectTimeout = conf->connectTimeout;
    inconf->cmdTimeout = conf->cmdTimeout;

#define ALLOCSTRCPY(member) inconf->member = (conf->member? strdup(conf->member): NULL)
    ALLOCSTRCPY(redisname);
    ALLOCSTRCPY(host);
    ALLOCSTRCPY(pswd);

    m_all_conf[conf->redisname] = inconf;

    BREAK_CTRL_END
    return ret;
}

int RedisConnPoolAdmin::RegisterPool(void)
{
    string tracelog;
    int okcnt = 0;
    // 此处可控制默认是延迟加载还是启动全加载, 目前是后者
    for (map<string, redis_pool_conf_t*>::iterator it = m_all_conf.begin();
        it != m_all_conf.end(); ++it)
    {
        redis_pool_conf_t* conf = it->second;
        tracelog.append(conf->redisname);

        if (0 == conf->processMaxConnNum)
        {
            tracelog.append("=skip ");
            continue;
        }

        if (0 == conf->initConnNum)
        {
            tracelog.append("=delay ");
            continue;
        }

        int reg_ret = RegisterPool(conf);

        if (0 == reg_ret)
        {
            tracelog.append("=ok ");
            ++okcnt;
        }
        else
        {
            char buff[32];
            snprintf(buff, sizeof(buff), "=fail%d ", reg_ret);
            tracelog.append(buff);
            LOGWARN("REGISTER_REDISPOOL| msg=register Redis pool fail| "
                "redisname=%s| ret=%d", conf->redisname, reg_ret);
        }
    }

    LOGINFO("REGISTER_REDISPOOL| msg=%d(%s)", okcnt, tracelog.c_str());
    fprintf(stdout, "load_redispool result: %s\n", tracelog.c_str());
    return 0;
}

int RedisConnPoolAdmin::RegisterPool(redis_pool_conf_t* conf)
{
	int ret = 0;
	
	BREAK_CTRL_BEGIN
	BREAKNIF1(NULL == conf, ERR_RDS_PARAM_INPUT);
	BREAKNIF1(NULL == conf->redisname, ERR_RDS_PARAM_INPUT);
    LockGuard lk(m_lock); // 构造加锁,析构解锁
	BREAKNIF1(m_conn_pools.find(conf->redisname) != m_conn_pools.end(), ERR_RDS_DUP_INIT);

	
//     if (delay)
//     {
//         m_all_conf[conf->redisname] = conf;
//     }
//     else
    {
        RedisConnPool* pool = new RedisConnPool;
        ret = pool->init(conf);

        if (0 == ret)
        {
            m_conn_pools[conf->redisname] = pool;
        }
        else
        {
            delete pool;
        }
    }
	
	BREAK_CTRL_END
	return ret;
}

int RedisConnPoolAdmin::DestroyPool(const char* redisname)
{
    LockGuard lk(m_lock); // 构造加锁,析构解锁

	if (redisname)
	{
		map<string, RedisConnPool*>::iterator it = m_conn_pools.find(redisname);
		if (m_conn_pools.end() != it)
		{
			delete it->second;
			m_conn_pools.erase(it);
		}

        map<string, redis_pool_conf_t*>::iterator it2 = m_all_conf.find(redisname);
        if (it2 != m_all_conf.end())
        {
            redis_pool_conf_t* conf = it2->second;
            IFFREE_C(conf->redisname);
            IFFREE_C(conf->host);
            IFFREE_C(conf->pswd);

            IFFREE_C(conf);
            m_all_conf.erase(it2);
        }
	}
	else
	{
		map<string, RedisConnPool*>::iterator it = m_conn_pools.begin();
		for (; it != m_conn_pools.end(); ++it)
		{
			delete it->second;
		}

		map<string, redis_pool_conf_t*>::iterator it2 = m_all_conf.begin();
        for (; it2 != m_all_conf.end(); ++it2)
        {
            redis_pool_conf_t* conf = it2->second;
            IFFREE_C(conf->redisname);
            IFFREE_C(conf->host);
            IFFREE_C(conf->pswd);

            IFFREE_C(conf);
        }

        m_conn_pools.clear();
        m_all_conf.clear();
	}
	
	return 0;
}
	
int RedisConnPoolAdmin::GetConnect(Redis*& predis, const char* redisname)
{
    int ret;
    RedisConnPool* pool = NULL;
    map<string, RedisConnPool*>::iterator it;

    {
        LockGuard lk(m_lock); // 构造加锁,析构解锁
        it = m_conn_pools.find(redisname);
        if (m_conn_pools.end() != it)
        {
            pool = it->second;
        }
    }

    if (pool) // 与DestroyPool()并发时有风险; 后考虑
    {
		ret = pool->getConnect(predis);
	}
	else
	{
		// log unexcept redisname or unregister poolname
		ret = ERR_RDS_INVALID_NAME;
        map<string, redis_pool_conf_t*>::iterator it = m_all_conf.find(redisname);
        if (it != m_all_conf.end())
        {
            redis_pool_conf_t* conf = it->second;
            ret = RegisterPool(conf);
            if (0 == ret)
            {
                ret = GetConnect(predis, redisname);
            }
        }
	}
	
	return ret;
}

int RedisConnPoolAdmin::ReleaseConnect(Redis* predis)
{
	if (predis)
	{
		RedisConnPool* pool = (RedisConnPool*)predis->parent;
        if (pool)
        {
            pool->relConnect(predis, false);
        }
        else
        {
            LOGERROR("RELEASE_CONNECT| msg=Redis object miss parent| ptr=%p", predis);
            delete predis;
        }
	}
	
	return 0;
}

void RedisConnPoolAdmin::showPoolStatus( const char* redisname )
{
    string logmsg;

    showPoolStatus(logmsg, redisname);
    if (!logmsg.empty())
    {
        LOGDEBUG("%s", logmsg.c_str());
    }
}

void RedisConnPoolAdmin::showPoolStatus( string& strbak, const char* redisname )
{
    RedisConnPool* pool = NULL;
    string keyname;
    map<string, RedisConnPool*>::iterator it;
    LockGuard lk(m_lock); // 构造加锁,析构解锁

    if (redisname)
    {
        keyname = redisname;
    }

    if (!keyname.empty())
    {
        it = m_conn_pools.find(keyname);
        if (m_conn_pools.end() != it)
        {
            pool = it->second;
            pool->trace_stat(strbak, true);
            strbak.append("<hr>");
        }
        else if (m_all_conf.find(keyname) != m_all_conf.end())
        {
            strbak.append("redisname=%s not init| ", redisname);
            strbak.append("<hr>");
        }
    }
    else
    {
        for (it = m_conn_pools.begin(); it != m_conn_pools.end(); ++it)
        {
            pool = it->second;
            pool->trace_stat(strbak, false);
        }
    }
}

