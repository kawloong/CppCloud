/*-------------------------------------------------------------------------
summery  : 线程安全链表容器
Date  09/26/2008
Author hejl
Description  : 适用于有多线程同时访问链表的情况, 内部自带条件变量通知功能
Modification Log:
--------------------------------------------------------------------------
<日期，格式mm/dd/yyyy>   <编写人名>     hejl   <修改记录说明>
<2016-09-27>  hejl  加入延时任务功能
-------------------------------------------------------------------------*/
#ifndef __GENERAL_QUEUE_H__
#define __GENERAL_QUEUE_H__

//#include <assert.h>
#include <map>
#include <deque>
#include <algorithm>
#include <iostream>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

using namespace std; 
#define USE_QUEUE_CPP // 如果想单独使用此文件,脱离queue.cpp,则注释此行

template<bool isPtr=false>
class Bool2Type
{
	enum{eType = isPtr};
};

#ifdef USE_QUEUE_CPP
bool operator<(struct timeval l, struct timeval r);
#else
static bool operator<(struct timeval l, struct timeval r)
{
    bool ret = (l.tv_sec == r.tv_sec) ? (l.tv_usec < r.tv_usec): (l.tv_sec < r.tv_sec);
    return ret;
}
#endif

template< typename T, bool isPtr=false, typename storage=deque<T> >
class Queue
{
    //struct timeval m_basetime; // 毫秒计时起点
    map< struct timeval, deque<T> > m_dely_task;
    struct timeval m_near_time;
    typedef typename map< struct timeval,deque<T> >::iterator DTASK_ITER;
    typedef typename deque<T>::iterator DLIST_ITER;

public:
    bool append_delay(const T &pNode, int delay_ms) // 加入延后解发的任务
    {
        struct timeval destime;
        bool bnotify = true;

        if ( !pNode ) return false;

        if (delay_ms <= 0)
        {
            return append(pNode); // 即时任务
        }

        now_dtime(&destime, delay_ms);

        pthread_mutex_lock(&m_mutex);
        if (!m_dely_task.empty())
        {
            DTASK_ITER it = m_dely_task.begin();
            bnotify = (destime < it->first);
        }
        
        m_dely_task[destime].push_back(pNode);

        if (bnotify)
        {
            pthread_cond_signal(&m_cond);
        }
        
        pthread_mutex_unlock(&m_mutex);

        return true;
    }

    bool pop_delay(T &t)
    {
        bool got = false;
        struct timeval nexttime;
        
        pthread_mutex_lock(&m_mutex);
        if(m_list.empty())
        {
            got = _pop_delay_top(t, &nexttime);

            if (!got) // 未获取, 等待
            {
                struct timespec abstime;

                m_near_time.tv_sec = nexttime.tv_sec;
                m_near_time.tv_usec = nexttime.tv_usec;

                TIMEVAL_TO_TIMESPEC(&nexttime, &abstime);
                {
                    struct timeval now;
                    gettimeofday(&now, NULL);
//                     printf("Wait cond %ld sec %ld ns | now=%ld.%ld\n", // debug
//                         abstime.tv_sec, abstime.tv_nsec, now.tv_sec, now.tv_usec);
                }
                pthread_cond_timedwait(&m_cond, &m_mutex, &abstime);

                if (m_list.empty())
                {
                    got = _pop_delay_top(t, &nexttime);
                }
                else
                {
                    t = m_list.front();
                    m_list.pop_front();
                    got = true;
                }
            }
        }
        else
        {
            t = m_list.front();
            m_list.pop_front();
            got = true;
        }

        pthread_mutex_unlock(&m_mutex);

        return got;
    }

private:
    // 获得当前时间偏移delta_ms毫秒, 从t返回
    void now_dtime(struct timeval* t, int delta_ms)
    {
        const static int s_1sec_in_us = 1000000;
        gettimeofday(t, NULL);
        t->tv_sec += delta_ms/1000;
        t->tv_usec += (delta_ms%1000) * 1000;
        if (t->tv_usec > s_1sec_in_us) // 进位精确处理
        {
            t->tv_sec++;
            t->tv_usec %= s_1sec_in_us;
        }
    }

    // 调用时已加锁
    bool _pop_delay_top(T &t, struct timeval* ntime)
    {
        bool got = false;
        // 尝试提取延时任务
        if ( !m_dely_task.empty() )
        {
            struct timeval now;
            gettimeofday(&now, NULL);
            DTASK_ITER it = m_dely_task.begin();
            if (it->first < now) // 延时时间已到
            {
                DLIST_ITER lsit = it->second.begin();
                for (; lsit != it->second.end(); ++lsit)
                {
                    if (got)
                    {
                        m_list.push_back(*lsit);
                    }
                    else
                    {
                        t = *lsit;
                        got = true;
                    }
                }

                m_dely_task.erase(it);
            }
            else
            {
                ntime->tv_sec = it->first.tv_sec;
                ntime->tv_usec = it->first.tv_usec;
            }
        }
        else
        {
            now_dtime(ntime, g_defwait_time_ms);
        }

        return got;
    }


public:
    static const int def_timeout_us = 60*1000000;
	typedef void (*FUNC)(T &t);
    Queue() 
	{
        m_nListMaxSize = 10000;
		pthread_mutex_init(&m_mutex, NULL);
		pthread_cond_init(&m_cond, NULL);
        m_near_time.tv_sec = 10000;

        //gettimeofday(&m_basetime, NULL);
	}

   
	
    virtual ~Queue()
	{
		desDel(Bool2Type<isPtr>() );
		
		pthread_cond_broadcast(&m_cond);
		pthread_mutex_unlock(&m_mutex);
		pthread_cond_destroy(&m_cond);
		pthread_mutex_destroy(&m_mutex);
		
	}

    void wakeup(void)
    {
        pthread_cond_broadcast(&m_cond);
    }

    void SetMaxSize(unsigned int nMaxSize)
    {
//         if (nMaxSize < 100 || nMaxSize > 1000000)
//         {
//             return;
//         }
        m_nListMaxSize = nMaxSize;
    }

	void desDel(Bool2Type<false>)
	{
		
	}
	void desDel(Bool2Type<true>)
	{
		//cout << "调用删除析构\n";
		
		pthread_mutex_lock(&m_mutex);
		typename storage::iterator iter;
		for ( iter = m_list.begin(); iter !=m_list.end(); iter++ )
		{
			if(NULL != *iter)
			delete ( *iter );
		}
		pthread_mutex_unlock(&m_mutex);
		
	}
    void insert(int index,  const T &pNode)
	{
        pthread_mutex_lock(&m_mutex);
        typename storage::iterator iter = m_list.begin();
        advance( iter, index);
        m_list.insert( iter, pNode);
		pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mutex);
	}
	
    bool append(const T &pNode, int nMaxWaitSeconds = 0)
    {
        bool bRet = false;

        int nSeconds = 0;
        while (!bRet)
        {
            nSeconds++;
            pthread_mutex_lock(&m_mutex);
            if (m_list.size() < m_nListMaxSize)
            {
                m_list.push_back(pNode);
                bRet = true;
                pthread_cond_signal(&m_cond);
            }
            pthread_mutex_unlock(&m_mutex);  

            if (!bRet)
            {
                if (nSeconds > nMaxWaitSeconds) break;
                sleep(1);
            }
        }

        return bRet;         
    }


	//单位微妙
	int waitToNotEmpty(int timeout=def_timeout_us)
	{
		int nRet = 0;
		pthread_mutex_lock(&m_mutex);
		if(m_list.empty())
		{
			struct timespec abstime;
			this->maketime(&abstime, timeout);
			
			pthread_cond_timedwait(&m_cond,&m_mutex,&abstime);
		}

		if(m_list.empty())
		{
			nRet = -1;
		}
		
		pthread_mutex_unlock(&m_mutex);
		return nRet;
	}
	
	//从队列中删除元素单位微秒1us=1/1000000s
	bool pop(T &t, int timeout_us=def_timeout_us) //单位微秒
	{
		pthread_mutex_lock(&m_mutex);
		
		if(m_list.empty())
		{
            if (-1 == timeout_us) // 永久等待
            {
                pthread_cond_wait(&m_cond, &m_mutex);
            }
            else if (0 == timeout_us)
            {
                pthread_mutex_unlock(&m_mutex);
                return false;
            }            
            else
            { // 必须带-lpthread链接生成
			    struct timespec abstime;
			    this->maketime(&abstime, timeout_us);
			    pthread_cond_timedwait(&m_cond, &m_mutex, &abstime);
            }
		}
		
		if(m_list.empty())
		{
			pthread_mutex_unlock(&m_mutex);
			return false;
		}

		t = m_list.front();
		m_list.pop_front();
		
		pthread_mutex_unlock(&m_mutex);
		return true;
	}

	
    int size()
	{
		int nSize = 0;
		pthread_mutex_lock(&m_mutex);
		nSize = m_list.size();
        nSize += ( (1 + m_dely_task.size())>>1 );
		pthread_mutex_unlock(&m_mutex);
		return nSize;
	}

	void each(FUNC func, bool rmall)
    {
        DTASK_ITER it;
		pthread_mutex_lock(&m_mutex);
		
		typename storage::iterator iter = m_list.begin();
		for(; iter != m_list.end(); iter++)
		{
			(*func)(*iter);
		}

        it = m_dely_task.begin();
        for (; it != m_dely_task.end(); ++it)
        {
            DLIST_ITER lsit = it->second.begin();
            for (; lsit != it->second.end(); ++lsit)
            {
                T t = *lsit;
                func(t);
            }
        }

        if (rmall)
        {
            m_list.clear();
            m_dely_task.clear();
        }
		
		pthread_mutex_unlock(&m_mutex);
		return ;
		
	}

    
protected:
	//微秒1s=1000ms=1000000us=1000000000ns
	void maketime(struct timespec *abstime, int timeout)
	{
		struct timeval now;
		gettimeofday(&now, NULL);
		abstime->tv_sec = now.tv_sec + timeout/1000000;
		abstime->tv_nsec = now.tv_usec*1000 + (timeout%1000000)*1000;
		return;
	}

	//bool m_bIsPtr;
    //存放数据 的列表
    storage  m_list;
      //元素指针
    typedef typename storage::iterator NODE_ITEM;
    //线程锁
	pthread_mutex_t m_mutex;
	pthread_cond_t  m_cond;
	//bool m_bInit;
    unsigned int m_nListMaxSize;
    static const int g_defwait_time_ms = 100000000;
};



#endif



