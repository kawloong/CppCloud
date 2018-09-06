/******************************************************************* 
 *  summery:     提供便捷的方法操作tinyxml
 *  author:      hejl
 *  date:        2017-02-17
 *  description: 有效避免string(NULL), element=NULL, strdup(NULL)判断
 ******************************************************************/ 

#ifndef _BMSH_XML_H_
#define _BMSH_XML_H_
#include <string>
#include "rapidxml.hpp"
//#include "rapidxml_utils.hpp" // 解析文件用

using std::string;
typedef rapidxml::xml_node<char> XmlNode;

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
    static string text_string(XmlNode* element);
    static bool text_string(string& outstr, XmlNode* element);
    static char*  text_strdup(XmlNode* element, bool notNull = false);
    static const char* text_strptr(XmlNode* element, bool notNull = false);

    // 获取<element>的属性字符串
    static string attr_string(XmlNode* element, const char* attr);
    static bool attr_string(string& outstr, XmlNode* element, const char* attr);
    static char*  attr_strdup(XmlNode* element, const char* attr, bool notNull = false);
    static const char* attr_strptr(XmlNode* element, const char* attr, bool notNull = false);

    // 获取子标签项的内容文本字符串
    static string child_text_string(XmlNode* element, const char* item);
    static bool child_text_string(string& outstr, XmlNode* element, const char* item);
    static char*  child_text_strdup(XmlNode* element, const char* item, bool notNull = false);
    static const char* child_text_strptr(XmlNode* element, const char* item, bool notNull = false);


    // 获取<element>的内容整数值
    static int text_int(XmlNode* element, int def = 0);
    // 获取<element>的属性整数值
    static int attr_int(XmlNode* element, const char* attr, int def = 0);
    // 获取子标签项的内容文本整数值
    static int child_text_int(XmlNode* element, const char* item, int def = 0);

};

using namespace rapidxml;

#define ISEMPTY_STR(pstr) (NULL==pstr||string(pstr).empty())

#endif
