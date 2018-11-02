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
#include <vector>
#include <map>
#include "rapidjson/json.hpp"

using namespace std;

class ConfJson
{
public:
    int update( const Value* data );

    int query( int& oval, const string& qkey ) const;
    int query( string& oval, const string& qkey ) const;
    int query( map<string, string>& oval, const string& qkey ) const;
    int query( map<string, int>& oval, const string& qkey ) const;
    int query( vector<string>& oval, const string& qkey ) const;
    int query( vector<int>& oval, const string& qkey ) const;

    time_t getMtime( void ) const;

private:
    inline const Value* _findNode( const string& qkey ) const;
    inline int _parseVal( int& oval, const Value* node ) const;
    inline int _parseVal( string& oval, const Value* node ) const;

    template<class ValT>
    int queryMAP( map<string, ValT>& oval, const string& qkey ) const;
    template<class ValT>
    int queryVector( vector<ValT>& oval, const string& qkey ) const;

    
private:
    time_t m_mtime;
    Document m_doc;
    string m_fname;
    mutable RWLock m_rwLock;
};

#endif