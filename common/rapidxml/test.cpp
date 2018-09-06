// 测试使用rapidxml解析xml-file, 并用BmshXml辅助读取信息功能。
#include <cstdio>
#include "bmshxml.hpp"

char g_xmlstr[] = "<root> <nd1 attr=\"123\">node1_txt</nd1> <nd2> node2_txt </nd2> </root>";

int main(int argc, char* argv[])
{
    try
    {
        file<> fl;
        xml_document<> doc;
        doc.parse<0>(g_xmlstr);

        printf("doc.name=%s\n", doc.name());
        printf("doc.value=%s\n", doc.value());
        
        xml_node<char>* root = doc.first_node();
        printf("root.name=%s\n", root->name());
        printf("root.value=%s\n", root->value());

        xml_node<>* nd1 = root->first_node("nd1");
        printf("nd1.name=%s\n", nd1->name());
        printf("nd1.value=%s\n", nd1->value());

        printf("attr=%s\n", BmshXml::attr_strptr(nd1, "attr", true));
        printf("txt=%s\n", BmshXml::child_text_strptr(root, "nd2", true));
    }
    catch(rapidxml::parse_error& er)
    {
        printf("err: %s (%s)\n", er.what(), er.where<char>());
    }

    return 0;
}