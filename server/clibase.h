/*-------------------------------------------------------------------------
FileName     : clibase.h
Description  : 代表客户连接对象
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-10       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _CLIBASE_H_
#define _CLIBASE_H_
#include <arpa/inet.h>
#include "comm/hep_base.h"
#include "comm/queue.h"
#include "iobuff.h"

class CliBase: public HEpBase
{
public:
    HEPCLASS_DECL(CliBase, CliBase);
    CliBase(void);
    virtual ~CliBase(void);

	static int Init( void );

public: // interface HEpBase
    virtual int run( int flag, long p2 );
	virtual int onEvent( int evtype, va_list ap );

	// 自定义属性的操作
    void setProperty( const string& key, const string& val );
    string getProperty( const string& key );
	int getCliType(void) {return m_cliType; }
	int setCliType(int tp) { m_cliType=tp; }
	string getCliSockName() { return m_cliName; }
	bool isLocal(void) {return m_isLocal; }


protected:
	string m_cliName;
	int m_cliType; // 何种类型的客户端: 1 sevr端服务; 10 监控进程; 20 web serv; 30 观察进程; 40
	bool m_isLocal; 

public:
	map<string, string> m_cliProp; // 客户属性
};

#endif
