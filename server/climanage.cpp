#include "climanage.h"
#include "comm/strparse.h"
#include "comm/timef.h"
#include "act_mgr.h"
#include "cloud/const.h"

const char g_jsonconf_file[] = ".scomm_svrid.txt";

CliMgr::AliasCursor::AliasCursor( const string& key_beg ): 
	iter_range(CliMgr::Instance()->m_aliName2Child, key_beg, key_beg+"~")
{
}

CliMgr::AliasCursor::AliasCursor( const string& key_beg, const string& key_end ): 
	iter_range(CliMgr::Instance()->m_aliName2Child, key_beg, key_end)
{
}

CliBase* CliMgr::AliasCursor::pop(void)
{
	return iter_range.pop();
}


CliMgr::CliMgr(void)
{
	m_waitRmPtr = NULL;
	m_localEra = 0;
}
CliMgr::~CliMgr(void)
{
	IFDELETE(m_waitRmPtr);
}

int CliMgr::addChild( HEpBase* chd )
{
	CliBase* child = dynamic_cast<CliBase*>(chd);
	return child? addChild(child) : -22;
}

int CliMgr::addChild( CliBase* child )
{
	CliInfo& cliinfo = m_children[child];
	ERRLOG_IF1(cliinfo.t0 > 0, "ADDCLICHILD| msg=child has exist| newchild=%p| t0=%d", child, (int)cliinfo.t0);
	cliinfo.t0 = time(NULL);
	cliinfo.cliProp = &child->m_cliProp;
	return 0;
}

int CliMgr::addAlias2Child( const string& asname, CliBase* ptr )
{
	int ret = 0;

	map<CliBase*, CliInfo>::iterator it = m_children.find(ptr);
	if (m_children.end() == it) // 非直接子对象，暂时不处理
	{
		LOGWARN("ADDALIASCHILD| msg=ptr isnot listen's child| name=%s", asname.c_str());
		return -11;
	}

	it->second.aliasName[asname] = true;

	CliBase*& secondVal = m_aliName2Child[asname];
	if (NULL != secondVal && ptr != secondVal) // 已存在旧引用的情况下，要先清理掉旧的设置
	{
		LOGWARN("ADDALIASCHILD| msg=alias child name has exist| name=%s", asname.c_str());
		map<CliBase*, CliInfo>::iterator itr = m_children.find(secondVal);
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
	map<string, CliBase*>::iterator it = m_aliName2Child.find(asname);
	if (m_aliName2Child.end() != it)
	{
		map<CliBase*, CliInfo>::iterator itr = m_children.find(it->second);
		if (m_children.end() != itr)
		{
			itr->second.aliasName.erase(asname);
		}

		m_aliName2Child.erase(it);
	}
}

void CliMgr::removeAliasChild( CliBase* ptr, bool rmAll )
{
	map<CliBase*, CliInfo>::iterator it = m_children.find(ptr);
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
			m_children.erase(it);
			if (ptr != (CliBase*)m_waitRmPtr)
			{
				IFDELETE(m_waitRmPtr); // 清理前一待删对象(为避免同步递归调用)
			}

			LOGINFO("CliMgr_CHILDRM| msg=a iohand close| dt=%ds| asname=%s", int(time(NULL)-cliinfo.t0), asnamestr.c_str());
			if (!ptr->isLocal())
			{
				m_waitRmPtr = static_cast<IOHand*>(ptr);
			}
		}
	}
}

// 分布式的情况下cli有多种对象，所以以不同后缀区别
CliBase* CliMgr::getChildBySvrid( int svrid )
{
	static const char* svr_subfix[] = {"_C", "_S", "_I", "_s", NULL};
	CliBase* ret = NULL;
	string strsvrid = StrParse::Itoa(svrid);

	for (int i = 0; NULL != svr_subfix[i] && NULL == ret; ++i)
	{
		ret = getChildByName(strsvrid + svr_subfix[i]);
	}

	return ret;
}

CliBase* CliMgr::getChildByName( const string& asname )
{
	CliBase* ptr = NULL;
	map<string,CliBase*>::iterator it = m_aliName2Child.find(asname) ;
	if (it != m_aliName2Child.end())
	{
		ptr = it->second;
	}
	return ptr;
}

CliInfo* CliMgr::getCliInfo( CliBase* child )
{
	map<CliBase*, CliInfo>::iterator itr = m_children.find(child);
	return (itr!=m_children.end() ? &itr->second : NULL);
}

void CliMgr::updateCliTime( CliBase* child )
{
	CliInfo* clif = getCliInfo(child);
	ERRLOG_IF1RET(NULL==clif, "UPDATECLITIM| msg=null pointer at %p", child);
	string svrid = child->getProperty(CONNTERID_KEY);
	time_t now = time(NULL);

	if (now < clif->t1 + 20) return; // 超20sec才刷新

	if (clif->t1 >= clif->t0)
	{
		string atimkey = StrParse::Format("%s%ld@", CLI_PREFIX_KEY_TIMEOUT, clif->t1);
		atimkey += svrid;
		removeAliasChild(atimkey);
	}

	clif->t1 = now;
	child->setIntProperty("atime", now);
	string atimkey = StrParse::Format("%s%ld@", CLI_PREFIX_KEY_TIMEOUT, clif->t1);
	atimkey += svrid;
	int ret = addAlias2Child(atimkey, child);
	ERRLOG_IF1(ret, "UPDATECLITIM| msg=add alias atimkey fail| ret=%d| mi=%s", ret, child->m_idProfile.c_str());
}

void CliMgr::setProperty( CliBase* dst, const string& key, const string& val )
{
	dst->m_cliProp[key] = val;
}

string CliMgr::getProperty( CliBase* dst, const string& key )
{
	return dst->getProperty(key);
}

int CliMgr::onChildEvent( int evtype, va_list ap )
{
	int ret = -100;
	if (HEPNTF_SOCK_CLOSE == evtype)
	{
		IOHand* son = va_arg(ap, IOHand*);
		int clitype = va_arg(ap, int);

		ERRLOG_IF1RET_N(NULL==son || m_children.find(son)==m_children.end(), -101, 
		"CHILDEVENT| msg=HEPNTF_SOCK_CLOSE ev-param invalid| son=%s| clitype=%d", son->m_idProfile.c_str(), clitype); 

		CliInfo& cliinfo = m_children[son];
		cliinfo.t2 = time(NULL);
		Actmgr::Instance()->appCloseFound(son, clitype, cliinfo);

		removeAliasChild(son, true);
		ret = 0;
	}
	else if (HEPNTF_SET_ALIAS == evtype)
	{
		IOHand* son = va_arg(ap, IOHand*);
		const char* asname = va_arg(ap, const char*);
		ret = addAlias2Child(asname, son);
	}

	return ret;
}

// ep线程调用此方法通知各子对象退出
int CliMgr::progExitHanele( int flg )
{
	map<CliBase*, CliInfo>::iterator it = m_children.begin();
	for (; it != m_children.end(); )
	{
		map<CliBase*, CliInfo>::iterator preit = it;
		++it;
		if (preit->first->isLocal())
		{
			preit->first->run(HEFG_PEXIT, 2); /// #PROG_EXITFLOW(5)
		}
	}

	return 0;
}