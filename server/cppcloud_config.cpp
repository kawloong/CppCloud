#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "cloud/const.h"
#include "cppcloud_config.h"

namespace CloudConf
{
    Config s_config;

static bool isFileExists(const char* name)
{
    struct stat st;
    return stat(name?name:"", &st) == 0;
}

int Init( const char* conffile )
{
    int ret = 0;
    const char* filename = (NULL == conffile || 0 == conffile[0])? DEF_CONFILENAME: conffile;

    if (isFileExists(filename))
    {
        ret = s_config.load(filename);
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
static std::string s_##funname##_default = defval;                   \
static std::string s_##funname##_val;                                \
static bool s_##funname##_reset = false;                             \
std::string funname(bool commIfnil)                                  \
{                                                                    \
    if (s_##funname##_reset) return s_##funname##_val;               \
    std::string outval;                                              \
    int result = s_config.read(szSec, szKey, outval);                \
    if (result&&commIfnil)                                           \
        result = s_config.read("common", szKey, outval);             \
    if (result) return s_##funname##_default;                        \
    return outval;                                                   \
}                                                                    \
void funname##_DEFAULT(const std::string& newdef ) {                 \
    s_##funname##_default = newdef;                                  \
}                                                                    \
void funname##_SET(const std::string& newdef ) {                     \
    s_##funname##_val = newdef;   s_##funname##_reset=true;          \
}


#define FUNC_INT_CONF_IMP(funname, szSec, szKey, defval)             \
static int s_##funname##_default = defval;                           \
static int s_##funname##_val = 0;                                    \
static bool s_##funname##_reset = false;                             \
int funname(bool commIfnil)                                          \
{                                                                    \
    if (s_##funname##_reset) return s_##funname##_val;               \
    int outi;                                                        \
    std::string outval;                                              \
    int result = s_config.read(szSec, szKey, outval);                \
    if (result&&commIfnil)                                           \
        result = s_config.read("common", szKey, outval);             \
    if (result) return s_##funname##_default;                        \
    if (outval.length() > 1){                                        \
    const char* p = outval.c_str();                                  \
    sscanf(p, ('0'==p[0]?('x'==p[1]? "%x": "%o") : "%d"), &outi);}   \
    else { outi = atoi(outval.c_str()); }                            \
    return outi;                                                     \
}                                                                    \
void funname##_DEFAULT( int newdef ) {                               \
    s_##funname##_default = newdef;                                  \
}                                                                    \
void funname##_SET( int newdef ) {                                   \
    s_##funname##_val = newdef;                                      \
    s_##funname##_reset=true;                                        \
}

#define FUNC_ARRSTR_CONF_IMP(funname, szSec, szKey, defval)          \
static std::string s_##funname##_default = defval;                   \
static std::string funname(int idx, bool warnlog)                    \
{                                                                    \
    int result;                                                      \
    char outval;                                                     \
    char keystr[128];                                                \
    snprintf(keystr, sizeof(keystr), "%s%d", szKey, idx);            \
    result = s_config.read(szSec, keystr, outval);                   \
    if (result) return s_##funname##_default;                        \
    return outval;                                                   \
}



// 快捷配置读取定义
// <todo>: append reader function implement here
FUNC_STR_CONF_IMP(CppCloudLogPath, "cloud_serv", "logpath", ".");
FUNC_INT_CONF_IMP(CppCloudLogLevel, "cloud_serv", "loglevel", 4);
FUNC_INT_CONF_IMP(CppCloudLogFSize, "cloud_serv", "logfsize", 2);
FUNC_INT_CONF_IMP(CppCloudListenPort, "cloud_serv", "port", 4800);
FUNC_INT_CONF_IMP(CppCloudTaskQNum, "cloud_serv", "taskqnum", 1);
FUNC_STR_CONF_IMP(CppCloudListenClass, "cloud_serv", "listen_class", "Lisn");
FUNC_INT_CONF_IMP(CppCloudServID, "cloud_serv", "servid", 1);
FUNC_STR_CONF_IMP(CppCloudPeerNode, "cloud_serv", "peernode", ""); 
FUNC_STR_CONF_IMP(CppCloudConfPath, "cloud_serv", "conf_path", "conf");

}


