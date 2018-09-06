/******************************************************************* 
 *  summery:     提供便捷的方法操作tinyxml
 *  author:      hejl
 *  date:        2016-04-14
 *  description: 有效避免string(NULL), element=NULL, strdup(NULL)判断
 ******************************************************************/ 

#ifndef _BMSH_XML_H_
#define _BMSH_XML_H_
#include <string>
#include "tinyxml.h"

using std::string;
class TiXmlElement;

/*****************************
关键字说明:
text      :   xml标签的内容部分 <tag>内容部分</tag>
attr      :   xml标签的属性部分 <tag attr=属性部分></tag>
child_text:   子标签的内容部分 <tag> <item>内容</item> </tag>
string    :   返回std::string对象
strdup    :   返回由strdup堆分配得到的指针, 需调用者free释放
strptr    :   返回xmldocument当中字符串位置,调用者不能free
*****************************/

class BmshXml
{
public: 
    // 获取<element>的内容字符串
    static string text_string(TiXmlElement* element);
    static bool text_string(string& outstr, TiXmlElement* element);
    static char*  text_strdup(TiXmlElement* element, bool notNull = false);
    static const char* text_strptr(TiXmlElement* element, bool notNull = false);

    // 获取<element>的属性字符串
    static string attr_string(TiXmlElement* element, const char* attr);
    static bool attr_string(string& outstr, TiXmlElement* element, const char* attr);
    static char*  attr_strdup(TiXmlElement* element, const char* attr, bool notNull = false);
    static const char* attr_strptr(TiXmlElement* element, const char* attr, bool notNull = false);

    // 获取子标签项的内容文本字符串
    static string child_text_string(TiXmlElement* element, const char* item);
    static bool child_text_string(string& outstr, TiXmlElement* element, const char* item);
    static char*  child_text_strdup(TiXmlElement* element, const char* item, bool notNull = false);
    static const char* child_text_strptr(TiXmlElement* element, const char* item, bool notNull = false);


    // 获取<element>的内容整数值
    static int text_int(TiXmlElement* element, int def = 0);
    // 获取<element>的属性整数值
    static int attr_int(TiXmlElement* element, const char* attr, int def = 0);
    // 获取子标签项的内容文本整数值
    static int child_text_int(TiXmlElement* element, const char* item, int def = 0);

};

#define ISEMPTY_STR(pstr) (NULL==pstr||string(pstr).empty())

#endif
