/******************************************************************* 
 *  summery:        统计程序运行时间类
 *  author:         hejl
 *  date:           2016-06-30
 *  description:    
 ******************************************************************/
#ifndef _TIMESPAND_H__
#define _TIMESPAND_H__
#include <sys/time.h>

/* remark:
*       psec是总的耗时毫秒数;
*       pms是总的耗时秒数
*       psec和pms独立计算的,之间无累加关系; 
*/

class TimeSpand
{
public:
    TimeSpand(void);
    TimeSpand(long* psec, long* pms);
    ~TimeSpand(void);
    void reset(void);

    // 返回中途期间的时间: 即用构造对象到调用下面方法时的用时
    long spandMs(bool breset = false);
    long spandSecond(bool breset = false);

protected:
    long* m_psec;
    long* m_pms;
    struct timeval m_begin_t;
};

#endif
