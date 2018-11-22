/*-------------------------------------------------------------------------
FileName     : client_c.hpp
Description  : cppcloud c++ sdk接口文件
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-07       create     hejl 
-------------------------------------------------------------------------*/
#include <string>

using std::string;
struct svr_item_t; // defind in "svr_item.h"
struct msg_prop_t; // define in "msgprop.h"
typedef int (*CMD_HAND_FUNC)(msg_prop_t*, const char*);
typedef void (*CONF_CHANGE_CB)(const string& confname);

namespace client_c
{
    // 初始化，连接cppcloud-serv，获得主配置文件名
    int Init( const string& appName, const string& servHostPort, int appid = 0 );

    // 加载配置指定文件(download)
    int LoadConfFile( const string& fname );
    string GetMConfName( void ); // 获取主配置文件名
    // 配置查询 
    // T=[string, int, map<string,string>, map<string,int>, vector<string>, vector<int>]
    template<class T> int Query( T& oval, const string& fullqkey, bool wideVal );
    void SetConfChangeCallBack( CONF_CHANGE_CB cb ); // 设置变化回调（如果需要）

    // TCP服务提供者
    int InitTcpProvider( int listenPort );
    int InitTcpProvider( const string& host, int listenPort, int qlen );
    
    // tcp服务提供CallBack
    int AddProvdFunction( CMD_HAND_FUNC func );
    int AddCmdFunction( unsigned cmdid, CMD_HAND_FUNC func );
    int ProvdSendMsg( const msg_prop_t* msgprop, const string& msg );
    int ProvdSendMsgAsync( const msg_prop_t* msgprop, const string& msg );
    
    // 发布服务
    int RegProvider( const string& regname, int prvdid, short protocol, int port, const string& path="" );
    int RegProvider( const string& regname, int prvdid, short protocol, const string& url );
    void setDesc( const string& regname, int prvdid, const string& desc );
    void setWeight( const string& regname, int prvdid, short weight );
    void setVersion( const string& regname, int prvdid, short ver );
    void setEnable( const string& regname, int prvdid, bool enable );
    int PostOut( const string& regname, int prvdid );
    int PostEnable( const string& regname, int prvdid, bool enable );
    void AddProviderStat( const string& regname, int prvdid, bool isOk, int dcount );


    // 服务消费者
    int InitInvoker( const string& svrList );
    void SetRefreshTimeOut( int sec ); // 不设置默认5min
    void SetReportStatTime( int sec ); // 不设置默认15sec, sec=0时关闭

    // 自定义的服务消费者方法
    int GetSvrPrvd( svr_item_t& pvd, const string& svrname ); // 获取一个服务提供者
    void AddStat( const svr_item_t& pvd, bool isOk, int dcount=1 );// 更新接口调用的统计信息

    // tcp服务消费者
    int TcpRequest( string& resp, const string& reqmsg, const string& svrname );

    // http服务消费者
    int HttpGet( string& resp, const string& reqmsg, const string& svrname );
    int HttpPost( string& resp, const string& reqmsg, const string& svrname );

    // 运行和退出通知
    int Run( bool runBackgroud );
    int NotifyExit( void* ptr );

    // 销毁和退出
    void Destroy( void );
};