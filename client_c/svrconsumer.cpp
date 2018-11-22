#include "svrconsumer.h"
#include "rapidjson/json.hpp"
#include "comm/strparse.h"
#include "cloud/msgid.h"
#include "cloud/homacro.h"
#include "cloud/exception.h"
#include "cloud/switchhand.h"
#include "cloudapp.h"

SvrConsumer* SvrConsumer::This = NULL;

void SvrConsumer::SvrItem::rmBySvrid( int svrid, int prvdid )
{
    vector<svr_item_t>::iterator it = svrItms.begin();
    for (; it != svrItms.end(); )
    {
        if (svrid == it->svrid)
        {
            if (0 == prvdid || prvdid == it->prvdid)
            {
                this->weightSum -= it->weight;
                it = svrItms.erase(it);
            }
        }
        else
        {
            ++it;
        }
    }
}

svr_item_t* SvrConsumer::SvrItem::randItem( void )
{
    IFRETURN_N(svrItms.empty(), NULL);
    IFRETURN_N(weightSum <= 1, &svrItms[0]);
    
    int nrd = rand()%weightSum;
    int tmpsum = 0;
    vector<svr_item_t>::iterator it0 = svrItms.begin();
    for (; it0 != svrItms.end(); ++it0)
    {
        svr_item_t* pitm = &(*it0);
        tmpsum += pitm->weight;
        if (tmpsum > nrd)
        {
            return pitm;
        }
    }

    LOGWARN("RANDSVR| msg=logic err flow| nrd=%d| weightSum=%d| tmpsum=%d",
            nrd, weightSum, tmpsum);
    return &svrItms[0];
}

SvrConsumer::SvrConsumer( void )
{
    This = this;
    m_refresh_sec = 10;
    m_inqueue = false;
    m_totalDOkCount = 0;
    m_totalDNgCount = 0;
}

SvrConsumer::~SvrConsumer( void )
{
    uninit();
}

int SvrConsumer::OnCMD_SVRSEARCH_RSP( void* ptr, unsigned cmdid, void* param )
{
    return This->onCMD_SVRSEARCH_RSP(ptr, cmdid, param);
}

int SvrConsumer::OnCMD_EVNOTIFY_REQ( void* ptr ) // provider 下线通知
{
    return This->onCMD_EVNOTIFY_REQ(ptr);
}

int SvrConsumer::onCMD_SVRSEARCH_RSP( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false);
    int ret = parseResponse(&doc);

    return ret;
}

int SvrConsumer::onCMD_EVNOTIFY_REQ( void* ptr )
{
    const Document* doc = (const Document*)ptr;
    RJSON_GETSTR_D(notify, doc);
    RJSON_GETSTR_D(regname, doc);
    RJSON_GETINT_D(svrid, doc);
    RJSON_GETINT_D(prvdid, doc);

    ERRLOG_IF1RET_N(notify!="provider_down" || 0==svrid, -113, 
        "EVNOTIFY| msg=%s", Rjson::ToString(doc).c_str());
    
    RWLOCK_WRITE(m_rwLock);
    map<string, SvrItem*>::iterator it = m_allPrvds.find(regname);
    if (it != m_allPrvds.end())
    {
        it->second->rmBySvrid(svrid, prvdid);
    }

    return 0;
}

// @param: svrList 是空格分隔启动时需要获得的服务, 服务和版本间冒号分开
int SvrConsumer::init( const string& svrList )
{
    static const char seperator = ' ';
    static const char seperator2 = ':';
    //static const int timeout_sec = 3;
    vector<string> vSvrName;

    int ret = StrParse::SpliteStr(vSvrName, svrList, seperator);
    ERRLOG_IF1RET_N(ret, -110, "CONSUMERINIT| msg=splite to vector fail %d| svrList=%s", 
        ret, svrList.c_str());

    vector<string>::const_iterator cit = vSvrName.begin();
    for (; cit != vSvrName.end(); ++cit)
    {
        vector<string> vSvrNver;
        StrParse::SpliteStr(vSvrNver, *cit, seperator2);

        const string& svrname = vSvrNver[0];
        string verStr;
        if (2 == vSvrNver.size())
        {
            StrParse::PutOneJson(verStr, "version", atoi(vSvrNver[1].c_str()), true);
        }

        string resp;
        ret = CloudApp::Instance()->begnRequest(resp, CMD_SVRSEARCH_REQ, 
                _F("{\"regname\": \"%s\", %s \"bookchange\": 1}", svrname.c_str(), verStr.c_str()), 
                false); 
        IFBREAK(ret);

        ret = parseResponse(resp);
        IFBREAK(ret);
    }

    if (0 == ret)
    {
        // srand(time(NULL))
        CloudApp::Instance()->setNotifyCB("provider_down", OnCMD_EVNOTIFY_REQ);
        ret = CloudApp::Instance()->addCmdHandle(CMD_SVRSEARCH_RSP, OnCMD_SVRSEARCH_RSP)? 0 : -111;
        appendTimerq();
    }

    return ret;
}

int SvrConsumer::parseResponse( string& msg )
{
    Document doc;
    ERRLOG_IF1RET_N(doc.ParseInsitu((char*)msg.data()).HasParseError(), -111, 
        "COMSUMERPARSE| msg=json invalid");
    
    return parseResponse(&doc);
}

int SvrConsumer::parseResponse( const void* ptr )
{
    const Document* doc = (const Document*)ptr;

    RJSON_GETINT_D(code, doc);
    RJSON_GETSTR_D(desc, doc);
    const Value* pdata = NULL;
    Rjson::GetArray(&pdata, "data", doc);
    ERRLOG_IF1RET_N(0 != code || NULL == pdata, -112, 
        "COMSUMERPARSE| msg=resp fail %d| err=%s", code, desc.c_str());
    
    int ret = 0;
    Value::ConstValueIterator itr = pdata->Begin();
    string regname;
    string prvdLog;
    SvrItem* prvds = NULL;

    for (int i = 0; itr != pdata->End(); ++itr, ++i)
    {
        svr_item_t svitm;
        const Value* node = &(*itr);
        if (NULL == prvds)
        {
            prvds = new SvrItem;
            prvds->ctime = time(NULL);
            RJSON_GETSTR(regname, node);
        }
        
        RJSON_VGETSTR(svitm.url, "url", node);
        RJSON_VGETINT(svitm.svrid, "svrid", node);
        RJSON_VGETINT(svitm.prvdid, "prvdid", node);
        RJSON_GETINT_D(weight, node);
        RJSON_GETINT_D(protocol, node);
        bool validurl = svitm.parseUrl();
        ERRLOG_IF1BRK(!validurl, -113, "COMSUMERPARSE| msg=invalid url found| "
            "url=%s| svrname=%s", svitm.url.c_str(), regname.c_str());

        svitm.weight = weight;
        svitm.protocol = protocol;
        prvds->weightSum += weight;
        prvds->svrItms.push_back(svitm);

        if (i <= 3)
        {
            StrParse::AppendFormat(prvdLog, "%d@%s,", svitm.svrid, svitm.url.c_str());
        }
    }

    LOGOPT_EI(prvdLog.empty(), "PROVDLIST| regname=%s| svitem=%s", regname.c_str(), prvdLog.c_str());
    if (ret)
    {
        IFDELETE(prvds);
    }
    else
    {
        if (prvds)
        {
            RWLOCK_WRITE(m_rwLock);
            SvrItem* oldi = m_allPrvds[regname];
            IFDELETE(oldi);
            m_allPrvds[regname] = prvds;
        }
    }

    return ret;
}

void SvrConsumer::uninit( void )
{
    RWLOCK_WRITE(m_rwLock);
    map<string, SvrItem*>::iterator it = m_allPrvds.begin();
    for (; it != m_allPrvds.end(); ++it)
    {
        delete it->second;
    }

    m_allPrvds.clear();
}

void SvrConsumer::setRefreshTO( int sec )
{
    m_refresh_sec = sec;
}

int SvrConsumer::getSvrPrvd( svr_item_t& pvd, const string& svrname )
{
    RWLOCK_READ(m_rwLock);
    map<string, SvrItem*>::iterator it = m_allPrvds.find(svrname);
    IFRETURN_N(it == m_allPrvds.end(), -1);
    svr_item_t* itm = it->second->randItem();
    int ret = -114;
    if (itm)
    {
        pvd = *itm;
        ret = 0;
    }

    return ret;
}

void SvrConsumer::addOkCount( const string& regname, int prvdid, int dcount = 1 )
{
    m_totalDOkCount += dcount;
    m_okDCount[regname + "-" + _N(prvdid)] += dcount;
}

void SvrConsumer::addNgCount( const string& regname, int prvdid, int dcount = 1 )
{
    m_totalDNgCount += dcount;
    m_ngDCount[regname + "-" + _N(prvdid)] += dcount;
}

int SvrConsumer::_postSvrSearch( const string& svrname ) const
{
    int ret = CloudApp::Instance()->postRequest( CMD_SVRSEARCH_REQ, 
        _F("{\"regname\": \"%s\", \"bookchange\": 1}", 
                svrname.c_str()) );

    ERRLOG_IF1(ret, "POSTREQ| msg=post CMD_SVRSEARCH_REQ fail| "
            "ret=%d| regname=%s", ret, svrname.c_str());
    
    return ret;
}

// 驱动定时检查任务，qrun()
int SvrConsumer::appendTimerq( void )
{
	int ret = 0;
	if (!m_inqueue)
	{
		int wait_time_msec =m_refresh_sec*1000;
		ret = SwitchHand::Instance()->appendQTask(this, wait_time_msec );
		m_inqueue = (0 == ret);
		ERRLOG_IF1(ret, "APPENDQTASK| msg=append fail| ret=%d", ret);
	}
	return ret;
}

// 定时任务：检查是否需刷新服务提供者信息
int SvrConsumer::qrun( int flag, long p2 )
{
	int  ret = 0;
	m_inqueue = false;
	if (0 == flag)
	{
        time_t now = time(NULL);
        RWLOCK_READ(m_rwLock);
		auto itr = m_allPrvds.begin();
        for (; itr != m_allPrvds.end(); ++itr)
        {
            SvrItem* svitm = itr->second;
            if (svitm->svrItms.empty() || svitm->ctime < now - m_refresh_sec)
            {
                ret = _postSvrSearch(itr->first);
            }
        }

        LOGDEBUG("SVRCONSUMER| msg=checking svr valid");
        appendTimerq();
	}
	else if (1 == flag)
	{
		// exit handle
	}

	return ret;
}

int SvrConsumer::run(int p1, long p2)
{
    LOGERROR("INVALID_FLOW");
    return -1;
}