#include <stdio.h>
#include <stdlib.h>
#include "cppcloud_config.h"

namespace BmshConf
{
    Config s_config;


int Init( const char* conffile )
{
    int ret = 0;

    if (NULL == conffile || 0 == conffile[0])
    {
        ret = s_config.load(DEF_CONFILENAME);
    }
    else
    {
        ret = s_config.load(conffile);
    }

    if ( 0 != ret )
    {
        fprintf(stderr, "CONFINIT| msg=conf(%s) init fail(%d)\n", conffile, ret);
    }

    return ret;
}

void UnInit( void )
{
    s_config.unload();
}

#define FUNC_STR_CONF_IMP(funname, szSec, szKey, defval)             \
std::string funname(bool commIfnil)                            \
{                                                                    \
    std::string outval;                                              \
    int result = s_config.read(szSec, szKey, outval);                \
    if (result&&commIfnil)                                           \
        result = s_config.read("common", szKey, outval);             \
    if (result) return defval;                                       \
    return outval;                                                   \
}

#define FUNC_INT_CONF_IMP(funname, szSec, szKey, defval)             \
int funname(bool commIfnil)                                    \
{                                                                    \
    int outi;                                                        \
    std::string outval;                                              \
    int result = s_config.read(szSec, szKey, outval);                \
    if (result&&commIfnil)                                           \
        result = s_config.read("common", szKey, outval);             \
    if (result) return defval;                                       \
    if (outval.length() > 1){                                        \
    const char* p = outval.c_str();                                  \
    sscanf(p, ('0'==p[0]?('x'==p[1]? "%x": "%o") : "%d"), &outi);}   \
    else { outi = atoi(outval.c_str()); }                            \
    return outi;                                                     \
}

#define FUNC_ARRSTR_CONF_IMP(funname, szSec, szKey, defval)          \
static std::string funname(int idx, bool warnlog)              \
{                                                                    \
    int result;                                                      \
    char outval;                                                     \
    char keystr[128];                                                \
    snprintf(keystr, sizeof(keystr), "%s%d", szKey, idx);            \
    result = s_config.read(szSec, keystr, outval);                   \
    if (result) return defval;                                       \
    return outval;                                                   \
}

// 快捷配置读取定义
// <todo>: append reader function implement here
FUNC_STR_CONF_IMP(CppCloudLogPath, "scomm_serv", "logpath", ".");
FUNC_INT_CONF_IMP(CppCloudLogLevel, "scomm_serv", "loglevel", 4);
FUNC_INT_CONF_IMP(CppCloudLogFSize, "scomm_serv", "logfsize", 2);
FUNC_INT_CONF_IMP(CppCloudListenPort, "scomm_serv", "port", 4800);
FUNC_INT_CONF_IMP(CppCloudTaskQNum, "scomm_serv", "taskqnum", 1);
FUNC_STR_CONF_IMP(CppCloudListenClass, "scomm_serv", "listen_class", "Lisn");

}


