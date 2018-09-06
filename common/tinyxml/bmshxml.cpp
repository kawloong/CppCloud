
#include "tinystr.h"
#include "bmshxml.h"


static const char* stringnull = "";

// param: notNull 当无内容时是否不返回NULL
// return: 内容为空或不存在时返回NULL
// remark: 如果非NULL返回, 需调用者free释放
char* BmshXml::text_strdup(TiXmlElement* element, bool notNull)
{
    if (element)
    {
        const char* text = element->GetText();
        if (text)
        {
            return strdup(text);
        }
    }

    return notNull? strdup(stringnull) : NULL;
}
string BmshXml::text_string( TiXmlElement* element )
{
    if (element)
    {
        const char* text = element->GetText();
        if (text)
        {
            return (text);
        }
    }

    return stringnull;
}
const char* BmshXml::text_strptr(TiXmlElement* element, bool notNull)
{
    if (element)
    {
        const char* text = element->GetText();
        if (text)
        {
            return (text);
        }
    }

    return notNull? (stringnull) : NULL;
}


int BmshXml::text_int(TiXmlElement* element, int def/*= 0*/)
{
    int val = def;
    if (element)
    {
        const char* text = element->GetText();
        if (text)
        {
            val = atoi(text);
        }
    }

    return val;
}

// return: 内容为空或不存在时返回NULL
// remark: 如果非NULL返回, 需调用者free释放
const char* BmshXml::attr_strptr(TiXmlElement* element, const char* attr, bool notNull/* = false*/)
{
    if (element)
    {
        const char* text = element->Attribute(attr);
        if (text)
        {
            return (text);
        }
    }

    return notNull? "": NULL;
}

string BmshXml::attr_string( TiXmlElement* element, const char* attr )
{
    if (element)
    {
        const char* text = element->Attribute(attr);
        if (text)
        {
            return (text);
        }
    }

    return stringnull;
}

char* BmshXml::attr_strdup( TiXmlElement* element, const char* attr, bool notNull /*= false*/ )
{
    if (element)
    {
        const char* text = element->Attribute(attr);
        if (text)
        {
            return strdup(text);
        }
    }

    return notNull? strdup(stringnull): NULL;
}

int BmshXml::attr_int(TiXmlElement* element, const char* attr, int def/*= 0*/)
{
    int val = def;
    if (element && attr)
    {
        const char* text = element->Attribute(attr);
        if (text)
        {
            val = atoi(text);
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
string BmshXml::child_text_string( TiXmlElement* element, const char* item )
{
    if (element && item)
    {
        return text_string(element->FirstChildElement(item));
    }

    return stringnull;
}

char* BmshXml::child_text_strdup( TiXmlElement* element, const char* item, bool notNull/*= false*/ )
{
    if (element && item)
    {
        return text_strdup(element->FirstChildElement(item), notNull);
    }

    return notNull? strdup(stringnull): NULL;
}

const char* BmshXml::child_text_strptr( TiXmlElement* element, const char* item, bool notNull /*= false*/ )
{
    if (element && item)
    {
        return text_strptr(element->FirstChildElement(item), notNull);
    }

    return notNull? (stringnull): NULL;
}

/* example:
<element>
    <item>1234</item> // return 1234;
</element>
*/
int BmshXml::child_text_int(TiXmlElement* element, const char* item, int def/*= 0*/)
{
    int val = def;
    if (element && item)
    {
        val = text_int( element->FirstChildElement(item), def );
    }

    return val;
}



