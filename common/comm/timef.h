/******************************************************************* 
 *  summery:        公共时间格式处理类
 *  author:         hejl
 *  date:           2017-03-23
 *  description:
 ******************************************************************/
#include <time.h>

class TimeF
{
public:
    static time_t TodayBegin(void); // 调用时当天的起始timestamp

    static time_t HourPos(int dhour); // 整点小时timestamp

    static const char* StrFTime(const char* fmt, time_t t); // 输出所需格式的string（fmt参考strftime()）
};