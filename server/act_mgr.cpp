#include "act_mgr.h"
#include "comm/strparse.h"
#include "comm/timef.h"
#include "cloud/const.h"
#include "clibase.h"
#include "climanage.h"

Actmgr::Actmgr(void)
{
	m_opLogSize = CLIOPLOG_SIZE;
	m_pchildren = CliMgr::Instance()->getAllChild();
}
Actmgr::~Actmgr(void)
{
}


int Actmgr::pickupWarnCliProfile( string& json, const string& filter_key, const string& filter_val )
{
	map<string, CliBase*>::iterator itr = m_warnLog.begin();
	
	json += "[";

	for (int i = 0; itr != m_warnLog.end(); )
	{
		CliBase* ptr = itr->second;
		
		if (m_pchildren->find(ptr) == m_pchildren->end()) // 被清除了的历史session
		{
			map<string, CliBase*>::iterator itr0 = itr; ++itr;
			m_warnLog.erase(itr0);
			continue;
		}

		if (!filter_key.empty()) // 需要过滤条件
		{
			string valdata = ptr->getProperty(filter_key);
			if (!filter_val.empty() && valdata != filter_val)
			{
				++itr;
				continue;
			}
		}

		if (i) json.append(",");
		getJsonProp(ptr, json, "");
		++i; ++itr;
	}
	
	json.append("]");
	return 0;
}

// @summery: 获取客户端的属性信息
// @param: svrid 为0时获取所有,否则单个
// @param: key 为空时获取所有属性
int Actmgr::pickupCliProfile( string& json, int svrid, const string& key )
{
	json += "[";
	if (0 == svrid)
	{
		map<CliBase*, CliInfo>::iterator itr = m_pchildren->begin();
		for (int i = 0; itr != m_pchildren->end(); ++i, ++itr)
		{
			if (i) json.append(",");
			getJsonProp(itr->first, json, key);
		}
	}
	else
	{
		CliBase* ptr = CliMgr::Instance()->getChildBySvrid(svrid);
		if (ptr)
		{
			getJsonProp(ptr, json, key);
		}
	}

	json.append("]");
	return 0;
}

// 获取某个app的一项或全部属性，以json字符串返回
void Actmgr::getJsonProp( CliBase* cli, string& outj, const string& key )
{
	outj.append("{");
	if (key.empty())
	{
		map<string, string>::const_iterator itr = cli->m_cliProp.begin();
		for (int i = 0; itr != cli->m_cliProp.end(); ++itr, ++i)
		{
			if (i > 0) outj.append(",");
			StrParse::PutOneJson(outj, itr->first, itr->second);
		}
	}
	else
	{
		map<string, string>::const_iterator itr = cli->m_cliProp.find(key);
		if (itr != cli->m_cliProp.end())
		{
			StrParse::PutOneJson(outj, itr->first, itr->second);
		}
	}
	outj.append("}");
}


void Actmgr::setCloseLog( int svrid, const string& cloLog )
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

void  Actmgr::rmCloseLog( int svrid )
{
	if (svrid > 0) m_closeLog.erase(svrid);
	else m_closeLog.clear();
}

int Actmgr::appCloseFound( CliBase* son, int clitype, const CliInfo& cliinfo )
{
	int ret = -100;
	IFRETURN_N(NULL==son, ret);

	string& strsvrid = son->m_cliProp[CONNTERID_KEY];
	if (1 == clitype)
	{
		int svrid = atoi(strsvrid.c_str());
		string jsonstr("{");
		StrParse::PutOneJson(jsonstr, CONNTERID_KEY, strsvrid, true);
		StrParse::PutOneJson(jsonstr, "name", son->m_cliProp["name"], true);
		StrParse::PutOneJson(jsonstr, SVRNAME_KEY, son->m_cliProp[SVRNAME_KEY], true);
		StrParse::PutOneJson(jsonstr, "shell", son->m_cliProp["shell"], true);
		StrParse::PutOneJson(jsonstr, "progi", son->m_idProfile, true);
		StrParse::PutOneJson(jsonstr, "begin_time", TimeF::StrFTime("%F %T", cliinfo.t0), true);
		StrParse::PutOneJson(jsonstr, "end_time", TimeF::StrFTime("%F %T", cliinfo.t2), false);
		jsonstr += "}";
		m_closeLog[svrid] = jsonstr;
	}

	// 记录操作
	string logstr = StrParse::Format("CLIENT_CLOSE| clitype=%d| svrid=%s| prog=%s| localsock=%s",
		clitype, strsvrid.c_str(), son->m_idProfile.c_str(), son->m_cliProp["name"].c_str());
	appendCliOpLog(logstr);

	ret = 0;

	return ret;
}

void Actmgr::appendCliOpLog( const string& logstr )
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
int Actmgr::pickupCliCloseLog( string& json )
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
int Actmgr::pickupCliOpLog( string& json, int nSize )
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

void Actmgr::setWarnMsg( const string& taskkey, CliBase* ptr )
{
	m_warnLog[taskkey] = ptr;
}

void Actmgr::clearWarnMsg( const string& taskkey )
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
