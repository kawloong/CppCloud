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
    // 服务注册
    int regProvider( const string& regname, short protocol, const string& url );
    void setDesc( const string& regname, const string& desc );
    void setWeight( const string& regname, short weight );
    void setVersion( const string& regname, short ver );
    void setEnable( const string& regname, bool enable );
    void addOkCount( const string& regname, int dcount );
    void addNgCount( const string& regname, int dcount );

    int postOut( const string& regname );
    int postOut( const string& regname, bool enable );

    void uninit( void );

private:
    ProviderItem* _getProvider( const string& regname );

private:
    int m_svrid;
    int m_timeout_sec;

    // 服务提供者
    map<string, ProviderItem*> m_provider_apps;
};

#endif