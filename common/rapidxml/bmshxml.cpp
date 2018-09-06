#include <cstring>
#include "bmshxml.hpp"


static const char* stringnull = "";

// param: notNull 当无内容时是否不返回NULL
// return: 内容为空或不存在时返回NULL
// remark: 如果非NULL返回, 需调用者free释放
char* BmshXml::text_strdup(XmlNode* element, bool notNull)
{
    if (element)
    {
        const char* text = element->value(); //->GetText();
        if (text)
        {
            return strdup(text);
        }
    }

    return notNull? strdup(stringnull) : NULL;
}
string BmshXml::text_string( XmlNode* element )
{
    if (element)
    {
        const char* text = element->value();
        if (text)
        {
            return (text);
        }
    }

    return stringnull;
}

bool BmshXml::text_string( string& outstr, XmlNode* element )
{
    bool ret = false;
    if (element)
    {
        const char* text = element->value();
        if (text)
        {
            outstr = (text);
            ret = true;
        }
    }

    return ret;
}

const char* BmshXml::text_strptr(XmlNode* element, bool notNull)
{
    if (element)
    {
        const char* text = element->value();
        if (text)
        {
            return (text);
        }
    }

    return notNull? (stringnull) : NULL;
}


int BmshXml::text_int(XmlNode* element, int def/*= 0*/)
{
    int val = def;
    if (element)
    {
        const char* text = element->value();
        if (text)
        {
            val = atoi(text);
        }
    }

    return val;
}

// return: 内容为空或不存在时返回NULL
// remark: 如果非NULL返回, 需调用者free释放
const char* BmshXml::attr_strptr(XmlNode* element, const char* attr, bool notNull/* = false*/)
{
    if (element)
    {
        xml_attribute<>* xmlattr = element->first_attribute(attr);
        if (xmlattr)
        {
            return (xmlattr->value());
        }
    }

    return notNull? "": NULL;
}

string BmshXml::attr_string( XmlNode* element, const char* attr )
{
    if (element)
    {
        xml_attribute<>* xmlattr = element->first_attribute(attr);
        if (xmlattr)
        {
            return (xmlattr->value());
        }
    }

    return stringnull;
}

bool BmshXml::attr_string( string& outstr, XmlNode* element, const char* attr )
{
    bool ret = false;
    if (element)
    {
        xml_attribute<>* xmlattr = element->first_attribute(attr);
        if (xmlattr)
        {
            outstr = (xmlattr->value());
            ret = true;
        }
    }

    return ret;
}

char* BmshXml::attr_strdup( XmlNode* element, const char* attr, bool notNull /*= false*/ )
{
    if (element)
    {
        xml_attribute<>* xmlattr = element->first_attribute(attr);
        if (xmlattr)
        {
            return strdup(xmlattr->value());
        }
    }

    return notNull? strdup(stringnull): NULL;
}

int BmshXml::attr_int(XmlNode* element, const char* attr, int def/*= 0*/)
{
    int val = def;
    if (element && attr)
    {
        xml_attribute<>* xmlattr = element->first_attribute(attr);
        if (xmlattr && xmlattr->value())
        {
            val = atoi(xmlattr->value());
        }
    }

    return val;
}

/*
return: 内容为空或不存在时返回NULL
remark: 如果非NULL返回, 需调用者free释放
example:
<element>
    <item>return_value</item>
</element>
*/
string BmshXml::child_text_string( XmlNode* element, const char* item )
{
    if (element && item)
    {
        return text_string(element->first_node(item));
    }

    return stringnull;
}

bool BmshXml::child_text_string( string& outstr, XmlNode* element, const char* item )
{
    if (element && item)
    {
        return text_string(outstr, element->first_node(item));
    }

    return false;
}

char* BmshXml::child_text_strdup( XmlNode* element, const char* item, bool notNull/*= false*/ )
{
    if (element && item)
    {
        return text_strdup(element->first_node(item), notNull);
    }

    return notNull? strdup(stringnull): NULL;
}

const char* BmshXml::child_text_strptr( XmlNode* element, const char* item, bool notNull /*= false*/ )
{
    if (element && item)
    {
        return text_strptr(element->first_node(item), notNull);
    }

    return notNull? (stringnull): NULL;
}

/* example:
<element>
    <item>1234</item> // return 1234;
</element>
*/
int BmshXml::child_text_int(XmlNode* element, const char* item, int def/*= 0*/)
{
    int val = def;
    if (element && item)
    {
        val = text_int( element->first_node(item), def );
    }

    return val;
}



