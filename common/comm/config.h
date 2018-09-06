/*-------------------------------------------------------------------------
FileName     : config.h
Description  : 配置文件(ini/conf)解析读取类
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2016-09-06       create     hejl 
-------------------------------------------------------------------------*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <map>
#include <list>
#include <string>

using std::string;
using std::map;
using std::list;
typedef std::pair< string, std::pair<string, string> > Pair_Config_Value;
typedef list< Pair_Config_Value > List_Config_Item;

class Config
{
public:
    Config(void);
    ~Config(void);

    int load(const char *pPath);

    int unload(void);
    
    int reload(void);

    int read(const string &strItem, const string &strLvalue,string &strValue);

private:
    Config(const Config &);
    Config &operator=(const Config &);

private:
    char *m_pPath;
    //pthread_mutex_t m_lstMutex;
    List_Config_Item m_lstConfig;
};

#endif

