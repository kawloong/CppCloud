#include "climanage.h"
#include "comm/strparse.h"
#include "comm/timef.h"
#include "act_mgr.h"

const char g_jsonconf_file[] = ".scomm_svrid.txt";

CliMgr::CliMgr(void)
{
	m_waitRmPtr = NULL;
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
			if (ptr->isLocal())
			{
				m_waitRmPtr = static_cast<IOHand*>(ptr);
			}
		}
	}
}

CliBase* CliMgr::getChildBySvrid( int svrid )
{
	return getChildByName(StrParse::Itoa(svrid));
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
		map<string,string>* prop = (map<string,string>*)va_arg(ap, void*);

		ERRLOG_IF1RET_N(m_children.find(son)==m_children.end() || NULL==prop, -101, 
		"CHILDEVENT| msg=HEPNTF_SOCK_CLOSE ev-param invalid| son=%s| clitype=%d", son->m_idProfile.c_str(), clitype); 

		CliInfo& cliinfo = m_children[son];

		cliinfo.t2 = time(NULL);
		string strsvrid = (*prop)["svrid"];

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