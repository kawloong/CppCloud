#include "timef.h"


time_t TimeF::TodayBegin( void )
{
    time_t today0;
    time_t now = time(NULL);
    struct tm ntm;

    localtime_r(&now, &ntm);
    ntm.tm_hour = 0;
    ntm.tm_min = 0;
    ntm.tm_sec = 0;

    today0 = mktime(&ntm);
    return today0;
}

time_t TimeF::HourPos( int dhour )
{
    time_t hour;
    time_t now = time(NULL);
    struct tm ntm;

    localtime_r(&now, &ntm);
    ntm.tm_min = 0;
    ntm.tm_sec = 0;
    hour = mktime(&ntm);

    if (0 != dhour)
    {
        hour += (hour*3600);
    }

    return hour;
}

const char* TimeF::StrFTime( const char* fmt, time_t t )
{
    static __thread char strtm[64];
    struct tm tmdata;
    size_t ret;

    if (t <= 0)
    {
        t = time(NULL);
    }

    localtime_r(&t, &tmdata);
    ret = strftime(strtm, sizeof(strtm), fmt, &tmdata);
    return ret>0? strtm: "";
}
