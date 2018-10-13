/*-------------------------------------------------------------------------
FileName     : hocfg_mgr.h
Description  : 分布式配置管理
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-10-08       create     hejl 
-------------------------------------------------------------------------*/

#ifndef _HOCFG_MGR_H_
#define _HOCFG_MGR_H_
#include <string>
#include <map>
#include "comm/public.h"
#include "rapidjson/json.hpp"

using namespace std;

struct AppConfig
{
	Document doc;
	time_t mtime;
	bool isDel;

	AppConfig(void):mtime(0), isDel(false) {}
};

class HocfgMgr
{
	SINGLETON_CLASS2(HocfgMgr)
	HocfgMgr( void );
	~HocfgMgr( void );

public:
	static int OnSetConfigHandle( void* ptr, unsigned cmdid, void* param );
	static int OnGetAllCfgName( void* ptr, unsigned cmdid, void* param );
	static int OnCMD_HOCFGNEW_REQ( void* ptr, unsigned cmdid, void* param );
	static int OnCMD_BOOKCFGCHANGE_REQ( void* ptr, unsigned cmdid, void* param );

public:
	int init( const string& conf_root );
	void uninit( void );

	int query( string& result, const string& file_pattern, const string& key_pattern, bool incBase );
	string getAllCfgNameJson( int filter_flag = 2 ) const;
	int getCfgMtime( const string& file_pattern, bool incBase ) ;

	// 分布式配置互相同步最新配置
	int compareServHoCfg( int fromSvrid, const Value* jdoc );

private:
	int loads( const string& dirpath );
	bool getBaseConfigName( string& baseCfg, const string& curCfgName );
	AppConfig* getConfigByName( const string& curCfgName );
	void remove( const string& cfgname, time_t mtime );

	int parseConffile( const string& filename, const string& contents, time_t mtime ); // 读出,解析
	int save2File( const string& filename, const Value* doc ); // 持久化至磁盘

	int mergeJsonFile( Value* node0, const Value* node1, MemoryPoolAllocator<>& allc ); // 合并继承json
	int queryByKeyPattern( string& result, const Value* jdoc, 
		const string& file_pattern, const string& key_pattern ); // 输入json-key返回对应value

	// 事件通知触发
	void notifyChange( const string& filename, int mtime );

private:
	string m_cfgpath;
	map<string, AppConfig*> m_Allconfig;
	int m_seqid;
	static HocfgMgr* This;
};

#endif
