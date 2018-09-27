/******************************************************************* 
 *  summery:        字符串解析处理类
 *  author:         hejl
 *  date:           2016-05-17
 *  description:    
 ******************************************************************/

#ifndef _STRPARSE_H_
#define _STRPARSE_H_
#include <vector>
#include <map>
#include <string>

using namespace std;


class StrParse
{
public:
    // 以dv作为分隔符拆分str串返得到的结果存放到data容器
    static int SpliteInt(vector<int>& data, const string& str, char dv, int nildef = 0);
    static int SpliteStr(vector<string>& data, const string& str, char dv);
    static int SpliteStr(vector<string>& data, const char* pstr, unsigned int len, char dv);

    // 解析http的url参数
    static int SpliteQueryString(map<string, string>& outPar, const string& qstr);
    // 简单json解析, src全部转化成小写处理;
    static int PickOneJson(string& ostr, const string& src, const string& name);
    // 加入json中一项数据
    static bool PutOneJson(string& jstr, const string& jkey, const string& jvalue, bool comma_end = false);
    static bool PutOneJson(string& jstr, const string& jkey, int jval, bool comma_end = false);

    // 处理路径尾部字符
	static void AdjustPath(string& path, bool useend, char dv = '/');

    // 判断字符串是否不含特殊字符
    static bool IsCharacter(const string& str, bool inc_digit = true);
    static bool IsNumberic(const string& str);

    // 字符串格式化,类似sprintf作用
    static int AppendFormat(string& ostr, const char* fmt, ...);
    static string Format(const char* fmt, ...);
    static string Itoa(int n);
};

#define _F StrParse::Format
#define _N StrParse::Itoa

#endif