/*-------------------------------------------------------------------------
FileName     : service_provider.h
Description  : 服务提供者
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-10-16       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _SERVICE_PROVIDER_H_
#define _SERVICE_PROVIDER_H_
#include <string>
#include <map>

using namespace std;
class CliBase;

struct ServiceItem
{
	string svrname;
	string url;
	string desc;
	int svrid;
	int okcount; // 成功调用次数
	int ngcount; // 失败调用次数
	int tmpnum;
	short protocol; // tcp=1 udp=2 http=3 https=4
	short version;
	short weight; // 服务权重
	short idc;
	short rack;
	bool enable;

	ServiceItem( void );
	bool valid( void ) const;
	int parse0( const string& name, CliBase* cli );
	int parse( CliBase* cli );
	void getJsonStr( string& strjson, int oweight = 0 ) const;
	void getCalcJson( string& strjson , int oweight) const;

	int score( short idc, short rack ) const;
};

class ServiceProvider
{
public:
	ServiceProvider( const string& svrName );
	~ServiceProvider( void );

	int setItem( CliBase* cli );
	void removeItme( CliBase* cli );

	// 计算返回可用服务
	int query( string& jstr, short idc, short rack, short version, short limit ) const;
	int getAllJson( string& strjson ) const;

private:
	const string m_svrName;
	map<CliBase*, ServiceItem*> m_svrItems;
};

#endif
