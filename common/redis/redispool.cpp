
#include "comm/public.h"
#include "redispool.h"

#define BREAK_CTRL_BEGIN do{
#define BREAK_CTRL_END  }while(0);
#define BREAKIF1(exp) if(exp){break;}
#define BREAKNIF1(exp, n) if(exp){ret=n; break;}

#define UNINIT_RETURN_N(n) if (!m_inited) return n;
#define UNINIT_RETURN if (!m_inited) return ;

RedisConnPool::RedisConnPool(void): m_conf(NULL), m_inited(false)
{
}

int RedisConnPool::init(redis_pool_conf_t* conf)
{
	int result;

	do
	{
		if (NULL == conf || NULL == conf->redisname)
		{
			result = ERR_RDS_PARAM_INPUT;
			break;
		}
		
		m_conf = conf;
        m_conn_max_process = conf->processMaxConnNum;
        m_wait_timeout = DEFAULT_TIMEOUT;
		
		m_conn_count_process = 0;
		m_totalconn_count = 0;
        m_peek_conn_count = 0;
		m_freeconn_list.clear();
		
		result = init_pool(conf->initConnNum);
		
		m_inited = true;
	}
	while(0);
	
	return result;
}

void RedisConnPool::uninit(void)
{
	if (m_inited)
	{
		m_conf = NULL;

        LockGuard lk(m_lock); // 构造加锁,析构解锁
		list<Redis*>::iterator it = m_freeconn_list.begin();
		while (it != m_freeconn_list.end())
		{
			Redis* p = *it;
			destroy_connect(p);
			++it;
		}
		
		m_freeconn_list.clear();
		m_inited = false;
	}
}

RedisConnPool::~RedisConnPool(void)
{
	if (m_inited)
	{
		uninit();
	}
}

int RedisConnPool::init_pool(unsigned int count)  //初始化连接池
{
    unsigned int num = 0;
	for (unsigned int i = 0 ; i < count ;++i)
	{
		//如果当前连接数达到最大连接数，不再创建连接，返回false,由前端创建短连接。
		//查询数据库允许的最大连接数，只使用数据库90%的连接，当达到90%时不再创建新连接
		if (!can_create_poolconn())
		{
			LOGWARN("REDISPOOLINIT| msg=full conn| connPoolname=%s|curr_connections=%d|max_connections=%d",
				m_conf->redisname, m_conn_count_process, m_conn_max_process);
            break;
		}

        Redis* rds = create_connect();
        if (rds)
        {
            m_freeconn_list.push_back(rds);
            rds->inpool = true;

            ++m_conn_count_process;
            ++m_totalconn_count;
			++num;
        }
		else
		{
			LOGERROR("error Redis connect redisname=%s", m_conf->redisname);
            break;
		}
	}
	
	if (num == count) num = 0;
	else if (num > 0) num = 0;
	else num = ERR_RDS_INIT_FAIL;

	return (num);
}

// return: 0 成功; 1 创建连接失败; 2 池中无空闲连接可用; -1未初始化
int RedisConnPool::getConnect(Redis*& rds, bool only_pool)
{
	int result;
	Redis* p = NULL;
	bool can_create = false;
	
	UNINIT_RETURN_N(ERR_RDS_PARAM_INPUT);

    {
        LockGuard lk(m_lock); // 构造加锁,析构解锁
        if (!m_freeconn_list.empty())
        {
            p = m_freeconn_list.front();
            m_freeconn_list.pop_front();
        }
        else
        {
            can_create = can_create_poolconn(); // 池是否有空余配额
        }

    }

	if ( NULL == p )
	{
		if (only_pool && !can_create)
		{
			result = ERR_REDIS_EMPTYPOOL;
		}
		else // 创建新连接返回; 若有池配额则作长连接, 否则作短连接
		{
			Redis* newrds = create_connect();
			if (newrds)
			{
				newrds->inpool = can_create;
				
				LockGuard lk(m_lock); // 构造加锁,析构解锁
				++m_totalconn_count;

				if (can_create)
				{
					++m_conn_count_process;
				}

                if (m_totalconn_count > m_peek_conn_count)
                {
                    m_peek_conn_count = m_totalconn_count;
                }
				
				rds = newrds;
				result = 0;
			}
			else
			{
				result = ERR_REDIS_CONN;
			}
		}
	}
	else
	{
		if (difftime(time(NULL), p->gettime()) > m_wait_timeout)
		{
			if (0 == p->getstate()) // 检查连接是否仍有效
			{
				rds = p;
				result = 0;
			}
			else
			{
                if (0 == p->reconnect())
                {
                    p->settime();
                    result = 0;
                    rds = p;
                    LOGWARN("REDISGETCONN| msg=reconnect ok| redisname=%s", m_conf->redisname);
                }
                else
                {
                    destroy_connect(p);
                    LockGuard lk(m_lock); // 构造加锁,析构解锁
					--m_totalconn_count;
					--m_conn_count_process;
					// m_conn_count_db
					result = ERR_REDIS_CONN;
                }
			}
		}
		else
		{
			result = 0;
			rds = p;
		}
	}
	
	return result;
}

// param: check_connect 如果此察觉连接可能断开时,传入true,回收检查
void RedisConnPool::relConnect(Redis *rds, bool check_connect)  // 释放连接
{
	UNINIT_RETURN;
	if (rds && rds->inpool) // 长连接
	{
        if (check_connect && 0 != rds->getstate()) //rds连接已不正常
        {
            destroy_connect(rds);
            LockGuard lk(m_lock); // 构造加锁,析构解锁
            --m_conn_count_process;
            --m_totalconn_count;
        }
        else
        {
            rds->settime();
            LockGuard lk(m_lock); // 构造加锁,析构解锁
            m_freeconn_list.push_back(rds);
        }
	}
	else // 短连接
	{
		destroy_connect(rds);
        LockGuard lk(m_lock); // 构造加锁,析构解锁
        --m_totalconn_count;
	}
}

void RedisConnPool::destroy_connect(Redis* p) const
{
	if (p)
	{
		delete p;
	}
}

// remark: 返回的值要求调用者释放
Redis* RedisConnPool::create_connect() const
{
    Redis* rds = NULL;

    rds = new Redis(m_conf);
    if (0 == rds->getstate())
    {
        rds->parent = (void*)this;
        rds->settime();
    }
    else
    {
        delete rds;
        rds = NULL;
    }

    return rds;
}

// 是否还可以创建新连接加入池
inline bool RedisConnPool::can_create_poolconn(void)
{
	return (m_conn_count_process < m_conn_max_process);
}

// 查看连接池运行状态信息
void RedisConnPool::trace_stat( string& msg, bool includeFreeConn )
{
    char strbuf[512];
    int now = (int)time(NULL);

    // format: dbconn=当前进程总连接数/当前DB全局连接数/DB全局允许最大连接数;
    //         poolconn=池内空闲连接数/池内所有长连接数/池允许最大连接数
    //         dbtime_d=最后一次查全局连接数距今的秒数
    snprintf(strbuf, sizeof(strbuf),
        "redisname=%s(%s:%u)| totalconn=%d| poolconn=%d/%d/%d (%d)| "
        "seletno=%d| connTimeout=%ds| cmdTimeout=%ds",
        m_conf->redisname, m_conf->host, m_conf->port,
        m_totalconn_count, (int)m_freeconn_list.size(), 
        m_conn_count_process, m_conn_max_process, m_peek_conn_count,
        m_conf->selectno, m_conf->connectTimeout, m_conf->cmdTimeout); 

    msg.append(strbuf);

    if (includeFreeConn)
    {
        list<Redis*>::iterator it = m_freeconn_list.begin();
        while (it != m_freeconn_list.end())
        {
            Redis* p = *it;
            int err = p->getstate();
            snprintf(strbuf, sizeof(strbuf),
                "\nredis_state=%d| ativetime=%ds",
                err, now-(int)p->gettime());
            msg.append(strbuf);
            ++it;
        }
    }
}
