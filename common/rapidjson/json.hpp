/******************************************************************* 
 *  summery:     提供便捷的方法操作rapidjson
 *  author:      hejl
 *  date:        2017-02-17
 *  description: 有效避免string(NULL), element=NULL, strdup(NULL)判断
 ******************************************************************/ 

#ifndef _JSON_HPP_
#define _JSON_HPP_
#include <string>
#include "document.h"
#include "rapidjson.h"
#include "prettywriter.h"

using namespace rapidjson;
using std::string;


/*****************************
关键字说明:
Object           :   json中的{key: value}形式类型
Array            :   json中的[e1, e2, e3]形式类型
Value            :   对json数据类型的抽象，包括Object,Array,int,string，是Object和Array的基类
GetStr/GetInt    :   获取json内部某一部分的字符串值，通通1st参数返回，最后一参数是父节点Value，
                     中间参数用于定位第几元素(参数类型是int)或指定key对应的值(参数类型是const char*)

usage example:
Document doc;
char jsonstr[] = "{ \"key1\": 123, \"key2\": \"string value\", \"key3\": [100, 200, \"str300\"] }"
doc.Parse<0>(jsonstr.c_str());
if (doc.HasParseError()) { ... }
// get key2 string value
std::string val2;
int result = Rjson::GetValue(val2, "key2", &doc);
// get key1 integer value
int val1;
int result = Rjson::GetValue(val1, "key1", &doc);
// get array [1] integer of key3
int val3_1;
int result = Rjson::GetValue(val3_1, "key3", 1, &doc);
// get array [2] string of key3
string val3_2;
int result = Rjson::GetValue(val3_2, "key3", 2, &doc);
*****************************/

class Rjson
{
public:
static int GetValue(const Value** value, const char* name, const Value* parent)
{
	if (parent && name && parent->IsObject())
	{
		Value::ConstMemberIterator itr = parent->FindMember(name);
		if (itr != parent->MemberEnd())
		{
			*value = &(itr->value);
			return 0;
		}
	}
	
	return -1;
}

static int GetValue(const Value** value, int idx, const Value* parent)
{
	if (parent && idx >= 0 && parent->IsArray() && idx < (int)parent->Size())
	{
		*value = &( (*parent)[idx]);
		return 0;
	}

	return -1;
}

template<typename T>
static int GetObject(const Value** value, T t, const Value* parent)
{
	if (0 == GetValue(value, t, parent) && (*value)->IsObject())
	{
		return 0;
	}
	
	*value = NULL;
	return -1;
}

template<typename T>
static int GetArray(const Value** value, T t, const Value* parent)
{
	if (0 == GetValue(value, t, parent) && (*value)->IsArray())
	{
		return 0;
	}
	
	*value = NULL;
	return -1;
}

template<typename T>
static int GetStr(string& str, T t, const Value* parent)
{
	const Value* value = NULL;
	if (0 == GetValue(&value, t, parent) && value->IsString())
	{
		str = value->GetString();
		return 0;
	}
	
	return -1;
}

template<typename T>
static int GetInt(int& n, T t, const Value* parent)
{
	const Value* value = NULL;
	if (0 == GetValue(&value, t, parent) && value->IsInt())
	{
		n = value->GetInt();
		return 0;
	}
	
	return -1;
}

///////////////////////////////////////////

template<typename T1, typename T2>
static int GetValue(const Value** value, T1 t1, T2 t2, const Value* parent)
{
	const Value* tmpv = NULL;
	int ret = GetValue(&tmpv, t1, parent);
	if (0 == ret)
	{
		return GetValue(value, t2, tmpv);
	}
	
	return -1;
}

template<typename T1, typename T2>
static int GetObject(const Value** value, T1 t1, T2 t2, const Value* parent)
{
	if (0 == GetValue(value, t1, t2, parent) && (*value)->IsObject())
	{
		return 0;
	}
	*value = NULL;
	return -1;
}

template<typename T1, typename T2>
static int GetArray(const Value** value, T1 t1, T2 t2, const Value* parent)
{
	if (0 == GetValue(value, t1, t2, parent) && (*value)->IsArray())
	{
		return 0;
	}
	*value = NULL;
	return -1;
}

template<typename T1, typename T2>
static int GetStr(string& str, T1 t1, T2 t2, const Value* parent)
{
	const Value* value = NULL;
	if (0 == GetValue(&value, t1, t2, parent) && value->IsString())
	{
		str = value->GetString();
		return 0;
	}
	
	return -1;
}

template<typename T1, typename T2>
static int GetInt(int& n, T1 t1, T2 t2, const Value* parent)
{
	const Value* value = NULL;
	if (0 == GetValue(&value, t1, t2, parent) && value->IsInt())
	{
		n = value->GetInt();
		return 0;
	}
	
	return -1;
}

/////////////////// 修改 //////////////////////
static int SetObjMember( const string& key, int val, Document* node )
{
    int ret = -1;
    if (node && node->IsObject())
    {
        if (node->HasMember(key.c_str()))
        {
            node->RemoveMember(key.c_str());
        }
        Value tmpkey(kStringType);
        tmpkey.SetString(key.c_str(), node->GetAllocator()); 
        node->AddMember(tmpkey, val, node->GetAllocator());
        ret = 0;
    }
    return ret;
}

static int SetObjMember( const string& key, const string& val, Document* node )
{
	int ret = -1;
    if (node && node->IsObject())
    {
        if (node->HasMember(key.c_str()))
        {
            node->RemoveMember(key.c_str());
        }
        Value tmpkey(kStringType);
        Value tmpstr(kStringType);
        tmpkey.SetString(key.c_str(), node->GetAllocator()); 
        tmpstr.SetString(val.c_str(), node->GetAllocator()); 
        node->AddMember(tmpkey, tmpstr, node->GetAllocator());
        ret = 0;
    }

    return ret;
}

///////////////////////////////////////////////

static int ToString(string& str, const Value* node)
{
	if (node)
	{
		StringBuffer sb;
		Writer<StringBuffer> writer(sb); // PrettyWriter
		node->Accept(writer);
		str = sb.GetString();
		return 0;
	}

	return -1;
}

static string ToString(const Value* node)
{
	if (node)
	{
		StringBuffer sb;
		Writer<StringBuffer> writer(sb); // PrettyWriter
		node->Accept(writer);
		return sb.GetString();
	}

	return "";
}
};

#endif
