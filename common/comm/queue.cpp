#include "queue.h"


bool operator<(struct timeval l, struct timeval r)
{
    bool ret = (l.tv_sec == r.tv_sec) ? (l.tv_usec < r.tv_usec): (l.tv_sec < r.tv_sec);
    return ret;
}