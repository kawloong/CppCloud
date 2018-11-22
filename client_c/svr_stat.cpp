#include "svr_stat.h"
#include "cloud/switchhand.h"
#include "cloudapp.h"

SvrStat* SvrStat::This = NULL;

SvrStat::SvrStat( void )
{
    m_inqueue = false;
}

CountEntry* SvrStat::_getEntry( const string& regname, int prvdid )
{
    const string key = regname + "-" + _N(prvdid);
    auto it = m_stat.find(key);

    if (it != m_stat.end())
    {
        return &it.second;
    }

    m_stat[key].prvdid = prvdid;
    return &m_stat[key];
}

void SvrStat::addPrvdCount( const string& regname, int prvdid, bool isOk, int dcount )
{
    RWLOCK_WRITE(m_rwLock);
    CountEntry* stat = _getEntry(regname, prvdid);
    if (isOk)
    {
        stat->ok += dcount;
    }
    else
    {
        stat->ng += dcount;
    }
}

void SvrStat::addInvkCount( const string& regname, int prvdid, bool isOk, int dcount )
{
    RWLOCK_WRITE(m_rwLock);
    CountEntry* stat = _getEntry(regname, prvdid);
    
    if (isOk)
    {
        stat->ivk_ok += dcount;
        stat->ivk_dok += dcount;
    }
    else
    {
        stat->ivk_ng += dcount;
        stat->ivk_dng += dcount;
    }
}

int SvrStat::qrun( int flag, long p2 )
{
    int  ret = 0;
	m_inqueue = false;
	if (0 == flag)
    {
        string msg("{");

        {
            RWLOCK_READ(m_rwLock);
            if ( !m_PrvdCount.empty() )
            {
                msg += "\"prvd\":[";

                msg += "]";
            }
        }

        msg += "}";
    }

    return ret;
}

int SvrStat::run(int p1, long p2)
{
    return -1;
}

// 驱动定时检查任务，qrun()
int SvrStat::appendTimerq( void )
{
	int ret = 0;
	if (!m_inqueue)
	{
		int wait_time_msec =10*1000;
		ret = SwitchHand::Instance()->appendQTask(this, wait_time_msec );
		m_inqueue = (0 == ret);
		ERRLOG_IF1(ret, "APPENDQTASK| msg=append fail| ret=%d", ret);
	}
	return ret;
}