#include "climanage.h"
#include "comm/strparse.h"
#include "comm/timef.h"
//#include "act_mgr.h"
//#include "broadcastcli.h"
#include "cloud/const.h"


CliMgr::AliasCursor::AliasCursor( const string& key_beg ): 
	iter_range(CliMgr::Instance()->m_aliName2Child, key_beg, key_beg+"~")
{
}

CliMgr::AliasCursor::AliasCursor( const string& key_beg, const string& key_end ): 
	iter_range(CliMgr::Instance()->m_aliName2Child, key_beg, key_end)
{
}

IOHand* CliMgr::AliasCursor::pop(bool forceFindEach)
{
	return iter_range.pop(forceFindEach);
}

bool CliMgr::AliasCursor::empty(void)
{
	return iter_range.empty();
}


CliMgr::CliMgr(void)
{
	m_waitRmPtr = NULL;
	IOHand::Init(&CliMgr::OnChildEvent);
}

CliMgr::~CliMgr(void)
{
	IFDELETE(m_waitRmPtr);
 	map<IOHand*, CliInfo>::iterator it = m_children.begin();
	for (; it != m_children.end(); ++it)
	{
		delete it->first;
	}

	m_children.clear();
	m_aliName2Child.clear();
}

int CliMgr::newChild( int clifd, int epfd )
{
	int ret = 0;
	IOHand* worker = new IOHand;

	ret = HEpBase::Notify(worker, HEPNTF_INIT_PARAM, clifd, epfd);

	addChild(worker);
	updateCliTime(worker);
	return ret;
}

int CliMgr::addChild( IOHand* child, bool inCtrl )
{
	CliInfo& cliinfo = m_children[child];
	ERRLOG_IF1(cliinfo.t0 > 0, "ADDCLICHILD| msg=child has exist| newchild=%p| t0=%d", child, (int)cliinfo.t0);
	cliinfo.t0 = time(NULL);
	cliinfo.inControl = inCtrl;
	cliinfo.cliProp = &child->m_cliProp;
	return 0;
}

int CliMgr::addAlias2Child( const string& asname, IOHand* ptr )
{
	int ret = 0;

	map<IOHand*, CliInfo>::iterator it = m_children.find(ptr);
	if (m_children.end() == it) // 非直接子对象，暂时不处理
	{
		LOGWARN("ADDALIASCHILD| msg=ptr isnot listen's child| name=%s| ptr=%p", asname.c_str(), ptr);
		return -11;
	}

	it->second.aliasName[asname] = true;

	IOHand*& secondVal = m_aliName2Child[asname];
	if (NULL != secondVal && ptr != secondVal) // 已存在旧引用的情况下，要先清理掉旧的设置
	{
		LOGWARN("ADDALIASCHILD| msg=alias child name has exist| name=%s", asname.c_str());
		map<IOHand*, CliInfo>::iterator itr = m_children.find(secondVal);
		if (m_children.end() != itr)
		{
			itr->second.aliasName.erase(asname);
		}
		
		ret = 1;
	}
	
	secondVal = ptr;

	return ret;
}

void CliMgr::removeAliasChild( const string& asname )
{
	map<string, IOHand*>::iterator it = m_aliName2Child.find(asname);
	if (m_aliName2Child.end() != it)
	{
		map<IOHand*, CliInfo>::iterator itr = m_children.find(it->second);
		if (m_children.end() != itr)
		{
			itr->second.aliasName.erase(asname);
		}

		m_aliName2Child.erase(it);
	}
}

void CliMgr::removeAliasChild( IOHand* ptr, bool rmAll )
{
	map<IOHand*, CliInfo>::iterator it = m_children.find(ptr);
	if (it != m_children.end()) // 移除所有别名引用
	{
		string asnamestr;
		CliInfo& cliinfo = it->second;
		for (map<string, bool>::iterator itr = cliinfo.aliasName.begin(); itr != cliinfo.aliasName.end(); ++itr)
		{
			asnamestr.append( asnamestr.empty()?"":"&" ).append(itr->first);
			m_aliName2Child.erase(itr->first);
		}

		cliinfo.aliasName.clear();

		if (rmAll) // 移除m_children指针
		{
			if (!m_cliCloseConsumer.empty())
			{
				vector<CliPreCloseNotifyFunc>::iterator itrConsumer = m_cliCloseConsumer.begin();
				for (; itrConsumer != m_cliCloseConsumer.end(); ++itrConsumer)
				{
					(*itrConsumer)(it->first);
				}
			}

			m_children.erase(it);
			if (ptr != (IOHand*)m_waitRmPtr)
			{
				IFDELETE(m_waitRmPtr); // 清理前一待删对象(为避免同步递归调用)
			}

			LOGINFO("CliMgr_CHILDRM| msg=a iohand close| life=%ds| asname=%s", int(time(NULL)-cliinfo.t0), asnamestr.c_str());
			if (it->second.inControl)
			{
				m_waitRmPtr = static_cast<IOHand*>(ptr);
			}
		}
	}
}

IOHand* CliMgr::getChildByName( const string& asname )
{
	IOHand* ptr = NULL;
	map<string,IOHand*>::iterator it = m_aliName2Child.find(asname) ;
	if (it != m_aliName2Child.end())
	{
		ptr = it->second;
	}
	return ptr;
}

CliInfo* CliMgr::getCliInfo( IOHand* child )
{
	map<IOHand*, CliInfo>::iterator itr = m_children.find(child);
	return (itr!=m_children.end() ? &itr->second : NULL);
}

int CliMgr::getLocalAllCliJson( string& jstr )
{
	int ret = 0;

	jstr += "[";
	map<IOHand*, CliInfo>::iterator it = m_children.begin();
	for (int i = 0; it != m_children.end(); ++it)
	{
		IOHand* ptr = it->first;
		//if (ptr->isLocal() && ptr->getCliType() > 1)
		{
			if (i > 0) jstr += ",";
			jstr += "{";
			//ptr->serialize(jstr);
			//StrParse::PutOneJson(jstr, "ERAN", 
			//StrParse::Format("%s:%d:%d ", 
			//	ptr->getProperty(CONNTERID_KEY).c_str(), 
			//	ptr->m_era, it->second.t1), false); // 无逗号结束
			jstr += "}";
			++i;
			++ret;
		}
	}
	jstr += "]";

	return ret;
}

void CliMgr::updateCliTime( IOHand* child )
{
	CliInfo* clif = getCliInfo(child);
	ERRLOG_IF1RET(NULL==clif, "UPDATECLITIM| msg=null pointer at %p", child);
	string uniqstr = child->getProperty("fd");
	time_t now = time(NULL);

	if (uniqstr.empty()) return;
	if (now < clif->t1 + 20) return; // 超20sec才刷新

	if (clif->t1 >= clif->t0)
	{
		string atimkey = StrParse::Format("%s%ld@", CLI_PREFIX_KEY_TIMEOUT, clif->t1);
		atimkey += uniqstr;
		removeAliasChild(atimkey);
	}

	clif->t1 = now;
	child->setIntProperty("atime", now);
	string atimkey = StrParse::Format("%s%ld@", CLI_PREFIX_KEY_TIMEOUT, clif->t1);
	atimkey += uniqstr;
	int ret = addAlias2Child(atimkey, child);
	ERRLOG_IF1(ret, "UPDATECLITIM| msg=add alias atimkey fail| ret=%d| mi=%s", ret, child->m_idProfile.c_str());
}

int CliMgr::OnChildEvent( int evtype, va_list ap )
{
	return CliMgr::Instance()->onChildEvent(evtype, ap);
}

int CliMgr::onChildEvent( int evtype, va_list ap )
{
	int ret = -100;
	if (HEPNTF_SOCK_CLOSE == evtype) // 删除时要注意避免递归
	{
		IOHand* son = va_arg(ap, IOHand*);
		int clitype = va_arg(ap, int);

		ERRLOG_IF1RET_N(NULL==son || m_children.find(son)==m_children.end(), -101, 
		"CHILDEVENT| msg=HEPNTF_SOCK_CLOSE ev-param invalid| son=%s| clitype=%d", son->m_idProfile.c_str(), clitype); 

		CliInfo& cliinfo = m_children[son];
		cliinfo.t2 = time(NULL);
		//Actmgr::Instance()->appCloseFound(son, clitype, cliinfo);

		removeAliasChild(son, true);
		son = NULL;
		ret = 0;
	}
	else if (HEPNTF_SET_ALIAS == evtype)
	{
		IOHand* son = va_arg(ap, IOHand*);
		const char* asname = va_arg(ap, const char*);
		ret = addAlias2Child(asname, son);
	}
	else if (HEPNTF_UPDATE_TIME == evtype)
	{
		IOHand* son = va_arg(ap, IOHand*);
		updateCliTime(son);
		ret = 0;
	}

	return ret;
}


// ep线程调用此方法通知各子对象退出
int CliMgr::progExitHanele( int flg )
{
	LOGDEBUG("CLIMGREXIT| %s", selfStat(true).c_str());
	map<IOHand*, CliInfo>::iterator it = m_children.begin();
	for (; it != m_children.end(); )
	{
		map<IOHand*, CliInfo>::iterator preit = it;
		++it;
		preit->first->run(HEFG_PEXIT, 2); /// #PROG_EXITFLOW(5)
	}

	LOGDEBUG("CLIMGREXIT| %s", selfStat(true).c_str());
	return 0;
}

string CliMgr::selfStat( bool incAliasDetail )
{
	string adetail;

	if (incAliasDetail)
	{
		map<string, IOHand*>::const_iterator it = m_aliName2Child.begin();
		for (; it != m_aliName2Child.end(); ++it)
		{
			adetail += it->first;
			StrParse::AppendFormat(adetail, "->%p ", it->second);
		}
	}

	return StrParse::Format("child_num=%zu| wptr=%p| alias_num=%zu(%s)", 
		m_children.size(), m_waitRmPtr, m_aliName2Child.size(), adetail.c_str());
}	
