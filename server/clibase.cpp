#include "clibase.h"
#include "comm/strparse.h"
#include "comm/sock.h"
#include "exception.h"
#include <sys/epoll.h>
#include <cstring>
#include <cerrno>

HEPCLASS_IMPL(CliBase, CliBase)


// static
int CliBase::Init( void )
{
	return 0;
}

CliBase::CliBase(void): m_cliType(0), m_isLocal(true)
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

string CliBase::getProperty( const string& key )
{
	map<string, string>::const_iterator itr = m_cliProp.find(key);
	if (itr != m_cliProp.end())
	{
		return itr->second;
	}

	return "";
}

