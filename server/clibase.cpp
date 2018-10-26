#include "clibase.h"
#include "comm/strparse.h"
#include "comm/sock.h"
#include "cloud/exception.h"
#include <sys/epoll.h>
#include <cstring>
#include <cerrno>
#include "rapidjson/json.hpp"
#include "cloud/const.h"

HEPCLASS_IMPL(CliBase, CliBase)
int CliBase::s_my_svrid = 0;

// static
int CliBase::Init( int mysvrid )
{
	s_my_svrid = mysvrid;
	return 0;
}

CliBase::CliBase(void): m_cliType(0), m_isLocal(true), m_era(0)
{
    
}
CliBase::~CliBase(void)
{
}


int CliBase::run( int p1, long p2 )
{
	LOGERROR("CLIBASE| msg=unexcept run");
	return -1;
}

int CliBase::onEvent( int evtype, va_list ap )
{
	LOGERROR("CLIBASE| msg=unexcept onEvent");
	return -2;
}


void CliBase::setProperty( const string& key, const string& val )
{
	m_cliProp[key] = val;
}

void CliBase::setProperty( const string& key, int val )
{
	m_cliProp[key] = StrParse::Itoa(val);
}

string CliBase::getProperty( const string& key ) const
{
	map<string, string>::const_iterator itr = m_cliProp.find(key);
	if (itr != m_cliProp.end())
	{
		return itr->second;
	}

	return "";
}


int CliBase::Json2Map( const Value* objnode )
{
	IFRETURN_N(!objnode->IsObject(), -1);
	int ret = 0;
    Value::ConstMemberIterator itr = objnode->MemberBegin();
    for (; itr != objnode->MemberEnd(); ++itr)
    {
        const char* key = itr->name.GetString();
        if (itr->value.IsString())
        {
        	const char* val = itr->value.GetString();
			setProperty(key, val);
        }
		else if (itr->value.IsInt())
		{
			string val = StrParse::Itoa(itr->value.GetInt());
			setProperty(key, val);
		}
    }

    return ret;
}

void CliBase::setIntProperty( const string& key, int val )
{
	m_cliProp[key] = StrParse::Itoa(val);
}

void CliBase::updateEra( void )
{
	m_era = 1 + m_era;
}

int CliBase::getIntProperty( const string& key ) const
{
	return atoi(getProperty(key).c_str());
}

int CliBase::serialize( string& outstr ) const
{
	map<string, string>::const_iterator it = m_cliProp.begin();
	for (; it != m_cliProp.end(); ++it)
	{
		StrParse::PutOneJson(outstr, it->first, it->second, true);
	}

	return 0;
}

int CliBase::unserialize( const Value* rpJsonValue )
{
	int ret = Json2Map(rpJsonValue);
	m_era = getIntProperty("ERAN");
	if (0 == m_cliType)
	{
		m_cliType = getIntProperty(CLIENT_TYPE_KEY);
		if (1 == m_cliType)
		{
			LOGERROR("CLIUNSERIALIZE| msg=%s cannot set to clitype=1", m_idProfile.c_str());
			m_cliType = 99999999;
		}
	}
	return ret;
}