/*-------------------------------------------------------------------------
FileName     : provd_mgr.h
Description  : 客户端服务治理管理类
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-02       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _PROVD_MGR_H_
#define _PROVD_MGR_H_
#include "comm/public.h"
#include "cloud/svrprop.h"
#include <string>
#include <map>

using namespace std;

struct ProviderItem : public SvrProp
{

};

class ProvdMgr
{
    SINGLETON_CLASS2(ProvdMgr)
    ProvdMgr( void );

public:
    static int ReconnectNotifyCB( void* param ); // 重连后回调

public:

    // 获取，提供对外判断是否存在此服务
    ProviderItem* getProvider( const string& regname, int prvdid );

    // 服务注册
    int regProvider( const string& regname, int prvdid, short protocol, const string& url );
    void setUrl( const string& regname, int prvdid, const string& url );
    void setDesc( const string& regname, int prvdid, const string& desc );
    void setWeight( const string& regname, int prvdid, short weight );
    void setVersion( const string& regname, int prvdid, short ver );
    void setEnable( const string& regname, int prvdid, bool enable );
    void addOkCount( const string& regname, int prvdid, int dcount );
    void addNgCount( const string& regname, int prvdid, int dcount );

    int postOut( const string& regname, int prvdid );
    int postOut( const string& regname, int prvdid, bool enable );

    void uninit( void );

private:
    int reconnectNotifyCB( void* param );
    int registRequest( ProviderItem* pvd ) const;

private:
    int m_svrid;
    int m_timeout_sec;

    // 服务提供者
    map<string, ProviderItem*> m_provider_apps;
};

#endif