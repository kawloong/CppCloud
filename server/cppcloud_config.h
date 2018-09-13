/*-------------------------------------------------------------------------
FileName     : cppcloud_config.h
Description  : 灵活便捷地读取conf/ini配置
dependency   : config.h
Modification :
--------------------------------------------------------------------------
   1、Date  2016-09-06       create     hejl 
-------------------------------------------------------------------------*/

/*
usage:

    程序需要添加对[comm]:key1=val1项的读取， 使用者仅需加入3行代码即实现

    1 在cppcloud_config.h的<todo>后添加获取函数宏声明，名字自定义，之后在需要
      读取处使用该函数名读取: FUNC_STR_CONF(Getkey1)

    2 在bmsh_config.cpp后面的<todo>下方定义函数实现，定义时使用文件前面部分
      实现的宏：FUNC_STR_CONF_IMP(Getkey1, "comm", "key1", "default_str")

    3 根据程序需要在要读取配置的地方调用第1步的函数名，即可获得配置值
      std::string val1=Getkey1(); // 这样调用即可获得"val1"值

*/

#ifndef __CPPCLOUD_CONFIG_H__
#define __CPPCLOUD_CONFIG_H__
#include <string>
//#include "public.h"
#include "comm/config.h"

#define FUNC_STR_CONF(funname)                      \
void funname##_SET(const std::string&);             \
void funname##_DEFAULT(const std::string&);         \
std::string funname(bool b=false)

#define FUNC_INT_CONF(funname)                      \
void funname##_SET(int);                            \
void funname##_DEFAULT(int);                        \
int funname(bool b=false)

#define FUNC_ARRSTR_CONF(funname) static std::string funname(int idx)
#define DEF_CONFILENAME "cppcloud.conf"

namespace CloudConf
{
    int Init( const char* confpath );
    void UnInit( void );

    // <todo>: append reader function declare here
    FUNC_STR_CONF(CppCloudLogPath);
    FUNC_INT_CONF(CppCloudLogLevel);
    FUNC_INT_CONF(CppCloudLogFSize);
    FUNC_INT_CONF(CppCloudListenPort);
    FUNC_INT_CONF(CppCloudTaskQNum);
    FUNC_STR_CONF(CppCloudListenClass);
    FUNC_INT_CONF(CppCloudServID); // 控制中心自身的svrid
    FUNC_INT_CONF(CppCloudPort); // 监听端口
    FUNC_STR_CONF(CppCloudPeerNode); // 分布式其他serv: 192.168.10.44:3303|192.168.10.55:3409
};

#endif

