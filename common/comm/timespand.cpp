#include <stdio.h>
#include "comm/timespand.h"


TimeSpand::TimeSpand(long* psec, long* pms): m_psec(psec), m_pms(pms)
{
    reset();
}

TimeSpand::TimeSpand( void ): m_psec(NULL), m_pms(NULL)
{
    reset();
}

TimeSpand::~TimeSpand(void)
{
    if (m_psec)
    {
        *m_psec = spandSecond();
    }

    if (m_pms)
    {
        *m_pms = spandMs();
    }
}

void TimeSpand::reset( void )
{
    gettimeofday(&m_begin_t, NULL);
}

long TimeSpand::spandMs(bool breset)
{
    long ret;
    struct timeval end_t;
    gettimeofday(&end_t, NULL);
    ret = ( (end_t.tv_sec - m_begin_t.tv_sec) * 1000 + (end_t.tv_usec - m_begin_t.tv_usec) / 1000 );

    if (breset)
    {
        m_begin_t = end_t;
    }

    return ret;
}

long TimeSpand::spandSecond(bool breset)
{
    long ret;
    struct timeval end_t;
    gettimeofday(&end_t, NULL);
    ret = (end_t.tv_sec - m_begin_t.tv_sec);

    if (breset)
    {
        m_begin_t = end_t;
    }

    return ret;
}