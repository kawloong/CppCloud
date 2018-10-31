#include "config_mgr.h"

// @summery: 启动时同步下载所需的配置文件到本地
// @param: confName 以:分隔的配置文件名列表
// @return: 0成功；其他失败应退出进程
int ConfigMgr::initLoad( const string& confName )
{

}

int ConfigMgr::OnCMD_EVNOTIFY_REQ( void* ptr, unsigned cmdid, void* param )
{
    return This->onCMD_EVNOTIFY_REQ(ptr, cmdid, param);
}
int ConfigMgr::onCMD_EVNOTIFY_REQ( void* ptr, unsigned cmdid, void* param )
{

}
int ConfigMgr::OnCMD_GETCONFIG_RSP( void* ptr, unsigned cmdid, void* param )
{
    return This->onCMD_GETCONFIG_RSP(ptr, cmdid, param);
}
int ConfigMgr::onCMD_GETCONFIG_RSP( void* ptr, unsigned cmdid, void* param )
{
    
}