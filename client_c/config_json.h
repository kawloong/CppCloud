/*-------------------------------------------------------------------------
FileName     : config_json.h
Description  : 分布式配置客户端类
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-10-30       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _CONFIG_JSON_H_
#define _CONFIG_JSON_H_
#include "comm/public.h"
#include "comm/lock.h"
#include <string>
#include "rapidjson/json.hpp"

class ConfJson
{
public:
    int update( const Value* data );

    int query( int& oval, const string& qkey );
    int query( string& oval, const string& qkey );
    int query( map<string, string>& oval, const string& qkey );
    int query( map<string, int>& oval, const string& qkey );
    int query( vector<string>& oval, const string& qkey );
    int query( vector<int>& oval, const string& qkey );


private:
    time_t m_mtime;
    Document m_doc;
    RWLock m_rwLock;
};

#endif