#include "climanage.h"
#include "comm/strparse.h"
#include "comm/timef.h"

const char g_jsonconf_file[] = ".scomm_svrid.txt";

CliMgr::CliMgr(void)
{
	m_waitRmPtr = NULL;
	m_opLogSize = CLIOPLOG_SIZE;
}
CliMgr::~CliMgr(void)
{
	IFDELETE(m_waitRmPtr);
}

int CliMgr::addChild( HEpBase* child )
{
	CliInfo& cliinfo = m_children[child];
	ERRLOG_IF1(cliinfo.t0 > 0, "ADDCLICHILD| msg=child has exist| newchild=%p| t0=%d", child, (int)cliinfo.t0);
	cliinfo.t0 = time(NULL);
	return 0;
}

int CliMgr::addAlias2Child( const string& asname, HEpBase* ptr )
{
	int ret = 0;

	map<HEpBase*, CliInfo>::iterator it = m_children.find(ptr);
	if (m_children.end() == it) // 非直接子对象，暂时不处理
	{
		LOGWARN("ADDALIASCHILD| msg=ptr isnot listen's child| name=%s", asname.c_str());
		return -1;
	}

	it->second.aliasName[asname] = true;

	HEpBase*& secondVal = m_aliName2Child[asname];
	if (NULL != secondVal && ptr != secondVal) // 已存在旧引用的情况下，要先清理掉旧的设置
	{
		LOGWARN("ADDALIASCHILD| msg=alias child name has exist| name=%s", asname.c_str());
		map<HEpBase*, CliInfo>::iterator itr = m_children.find(secondVal);
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
	map<string, HEpBase*>::iterator it = m_aliName2Child.find(asname);
	if (m_aliName2Child.end() != it)
	{
		map<HEpBase*, CliInfo>::iterator itr = m_children.find(it->second);
		if (m_children.end() != itr)
		{
			itr->second.aliasName.erase(asname);
		}

		m_aliName2Child.erase(it);
	}
}

void CliMgr::removeAliasChild( HEpBase* ptr, bool rmAll )
{
	map<HEpBase*, CliInfo>::iterator it = m_children.find(ptr);
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
			if (ptr != m_waitRmPtr)
			{
				IFDELETE(m_waitRmPtr); // 清理前一待删对象(为避免同步递归调用)
			}

			LOGINFO("CliMgr_CHILDRM| msg=a iohand close| dt=%ds| asname=%s", int(time(NULL)-cliinfo.t0), asnamestr.c_str());
			m_waitRmPtr = ptr;
		}
	}
}

HEpBase* CliMgr::getChildBySvrid( int svrid )
{
	HEpBase* ptr = NULL;
	map<string,HEpBase*>::iterator it = m_aliName2Child.find(StrParse::Itoa(svrid)) ;
	if (it != m_aliName2Child.end())
	{
		ptr = it->second;
	}
	return ptr;
}

int CliMgr::pickupWarnCliProfile( string& json, const string& filter_key, const string& filter_val )
{
	map<string, HEpBase*>::iterator itr = m_warnLog.begin();
	
	json += "[";

	for (int i = 0; itr != m_warnLog.end(); )
	{
		HEpBase* ptr = itr->second;
		if (m_children.find(ptr) == m_children.end()) // 被清除了的历史session
		{
			map<string, HEpBase*>::iterator itr0 = itr; ++itr;
			m_warnLog.erase(itr0);
			continue;
		}

		if (!filter_key.empty()) // 需要过滤条件
		{
			string valdata;
			HEpBase::Notify(ptr, HEPNTF_GET_PROPT, filter_key.c_str(), &valdata);
			if (!filter_val.empty() && valdata != filter_val)
			{
				++itr;
				continue;
			}
		}

		if (i) json.append(",");
		HEpBase::Notify(ptr, HEPNTF_GET_PROPT_JSONALL, "", &json);
		++i; ++itr;
	}
	
	json.append("]");
	return 0;
}

// @summery: 获取客户端的属性信息
// @param: svrid 为0时获取所有,否则单个
// @param: key 为空时获取所有属性
int CliMgr::pickupCliProfile( string& json, int svrid, const string& key )
{
	json += "[";
	if (0 == svrid)
	{
		map<HEpBase*, CliInfo>::iterator itr = m_children.begin();
		for (int i = 0; itr != m_children.end(); ++i, ++itr)
		{
			if (i) json.append(",");
			if (key.empty())
			{
		 		HEpBase::Notify(itr->first, HEPNTF_GET_PROPT_JSONALL, key.c_str(), &json);
			}
			else
			{
				string valdata;
		 		HEpBase::Notify(itr->first, HEPNTF_GET_PROPT, key.c_str(), &valdata);
				json += "{";
				StrParse::PutOneJson(json, key, valdata);
				json += "}";
			}
		}
	}
	else
	{
		map<string, HEpBase*>::iterator itr = m_aliName2Child.find(StrParse::Itoa(svrid));
		if (itr != m_aliName2Child.end())
		{
			if (key.empty())
			{
		 		HEpBase::Notify(itr->second, HEPNTF_GET_PROPT_JSONALL, key.c_str(), &json);
			}
			else
			{
				string valdata;
		 		HEpBase::Notify(itr->second, HEPNTF_GET_PROPT, key.c_str(), &valdata);
				StrParse::PutOneJson(json, key, valdata);
			}
		}
	}

	json.append("]");
	return 0;
}


void CliMgr::setProperty( HEpBase* dst, const string& key, const string& val )
{
	HEpBase::Notify(dst, HEPNTF_SET_PROPT, key.c_str(), val.c_str());
}

string CliMgr::getProperty( HEpBase* dst, const string& key )
{
	string oval;
	HEpBase::Notify(dst, HEPNTF_GET_PROPT, key.c_str(), &oval);

	return oval;
}

void CliMgr::setCloseLog( int svrid, const string& cloLog )
{
	if (cloLog.empty())
	{
		m_closeLog.erase(svrid);
	}
	else 
	{
		m_closeLog[svrid] = cloLog;
	}
}

void  CliMgr::rmCloseLog( int svrid )
{
	if (svrid > 0) m_closeLog.erase(svrid);
	else m_closeLog.clear();
}

int CliMgr::onChildEvent( int evtype, va_list ap )
{
	int ret = -100;
	if (HEPNTF_SOCK_CLOSE == evtype)
	{
		HEpBase* son = va_arg(ap, HEpBase*);
		int clitype = va_arg(ap, int);
		map<string,string>* prop = (map<string,string>*)va_arg(ap, void*);

		ERRLOG_IF1RET_N(m_children.find(son)==m_children.end() || NULL==prop, -101, 
		"CHILDEVENT| msg=HEPNTF_SOCK_CLOSE ev-param invalid| son=%s| clitype=%d", son->m_idProfile.c_str(), clitype); 

		CliInfo& cliinfo = m_children[son];

		cliinfo.t2 = time(NULL);
		string strsvrid = (*prop)["svrid"];

		if (1 == clitype) // 网监进程
		{
			int svrid = atoi(strsvrid.c_str());
			string jsonstr("{");
			StrParse::PutOneJson(jsonstr, "svrid", strsvrid, true);
			StrParse::PutOneJson(jsonstr, "name", (*prop)["name"], true);
			StrParse::PutOneJson(jsonstr, "svrname", (*prop)["svrname"], true);
			StrParse::PutOneJson(jsonstr, "shell", (*prop)["shell"], true);
			StrParse::PutOneJson(jsonstr, "progi", son->m_idProfile, true);
			StrParse::PutOneJson(jsonstr, "begin_time", TimeF::StrFTime("%F %T", cliinfo.t0), true);
			StrParse::PutOneJson(jsonstr, "end_time", TimeF::StrFTime("%F %T", cliinfo.t2), false);
			jsonstr += "}";
			m_closeLog[svrid] = jsonstr;
		}

		// 记录操作
		string logstr = StrParse::Format("CLIENT_CLOSE| clitype=%d| svrid=%s| prog=%s| localsock=%s",
			clitype, strsvrid.c_str(), son->m_idProfile.c_str(), (*prop)["name"].c_str());
		appendCliOpLog(logstr);

		removeAliasChild(son, true);
		ret = 0;
	}
	else if (HEPNTF_SET_ALIAS == evtype)
	{
		HEpBase* son = va_arg(ap, HEpBase*);
		const char* asname = va_arg(ap, const char*);
		ret = addAlias2Child(asname, son);
	}

	return ret;
}

void CliMgr::appendCliOpLog( const string& logstr )
{
	string stritem = TimeF::StrFTime("%F %T", time(NULL));
	int i = 0;

	stritem.append(": ");
	for (const char* ch = logstr.c_str(); 0 != ch[i]; ++i ) // 去除引号
	{
		stritem.append(1, '\"' == ch[i]? ' ' : ch[i]);
	}

	m_cliOpLog.push_back(stritem);
	if ((int)m_cliOpLog.size() > m_opLogSize)
	{
		m_cliOpLog.pop_front(); 
	}
}

// 获取已掉线的客户信息
int CliMgr::pickupCliCloseLog( string& json )
{
	json += "[";
	
	map<int, string>::const_iterator itr = m_closeLog.begin();
	for (int i = 0; itr != m_closeLog.end(); ++itr)
	{
		if (i++ > 0) json.append(",");
		json += itr->second;
	}

	json += "]";
	return 0;
}

// 获取客户行为日志信息
int CliMgr::pickupCliOpLog( string& json, int nSize )
{
	json += "[";

	list<string>::reverse_iterator itr = m_cliOpLog.rbegin();
	for (int i = 0; i < nSize && itr != m_cliOpLog.rend(); ++itr)
	{
		if (i++ > 0) json.append(",");
		json += "\"" + *itr + "\"";
	}

	json += "]";
	return 0;
}

void CliMgr::setWarnMsg( const string& taskkey, HEpBase* ptr )
{
	m_warnLog[taskkey] = ptr;
}

void CliMgr::clearWarnMsg( const string& taskkey )
{
	if (0 == taskkey.compare("all"))
	{
		m_warnLog.clear();
	}
	else
	{
		m_warnLog.erase(taskkey);
	}
	
}

// ep线程调用此方法通知各子对象退出
int CliMgr::progExitHanele( int flg )
{
	map<HEpBase*, CliInfo>::iterator it = m_children.begin();
	for (; it != m_children.end(); )
	{
		map<HEpBase*, CliInfo>::iterator preit = it;
		++it;
		preit->first->run(HEFG_PEXIT, 2); /// #PROG_EXITFLOW(5)
	}

	return 0;
}