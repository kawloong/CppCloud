/*-------------------------------------------------------------------------
FileName     : cloudapp.h
Description  : app对象
remark       : 作为CppCloudServ的客户端而存在
Modification :
--------------------------------------------------------------------------
   1、Date  2018-10-12       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _CLOUDAPP_H_
#define _CLOUDAPP_H_
#include "comm/public.h"
#include "iohand.h"
#include "synchand.h"

using namespace std;

typedef int (*NotifyCBFunc)( void* param );

class CloudApp: public IOHand
{
public:
	HEPCLASS_DECL(CloudApp, CloudApp);
	SINGLETON_CLASS2(CloudApp)
	CloudApp();

	// ****** message handle begin *******
	static int OnCMD_WHOAMI_RSP( void* ptr, unsigned cmdid, void* param );
	static int OnSyncMsg( void* ptr, unsigned cmdid, void* param );
	static int OnCMD_EVNOTIFY_REQ( void* ptr, unsigned cmdid, void* param );
	static int OnCMD_KEEPALIVE_REQ( void* ptr, unsigned cmdid, void* param );
	static int OnShowMsg( void* ptr, unsigned cmdid, void* param );
	static int OnCMD_WEBCTRL_REQ( void* ptr, unsigned cmdid, void* param );
	
	int onCMD_WHOAMI_RSP( string& whoamiResp );
	int onSyncMsg( void* ptr, unsigned cmdid, void* param );
	int onCMD_EVNOTIFY_REQ( void* ptr, unsigned cmdid, void* param );
	int onCMD_WEBCTRL_REQ( void* ptr, unsigned cmdid, void* param );
	// ****** message handle end *******

public:
	int init( int epfd, const string& svrhost_port, const string& appname );
	void uninit( void );

	bool isInitOk( void ) const;
	bool isInEpRun( void ) const;
	string getMConf( void ) const;
	string getLocalIP( void ) const;
	string whoamiMsg( void ) const;
	int setToEpollEv( void );

	void setSvrid( int svrid );
	void setNotifyCB( const string& notify, NotifyCBFunc func ); // 订阅Serv的推送消息

	int syncRequest( string& resp, unsigned cmdid, const string& reqmsg, int tosec ); // 同步等待请求+响应全过程完成
	int postRequest( unsigned cmdid, const string& reqmsg ); // 发送消息后不等待响应就返回
	int postRequest( unsigned cmdid, const string& reqmsg, bool noEpFlag ); // 发送消息后不等待响应就返回
	int begnRequest( string& resp, unsigned cmdid, const string& reqmsg, bool noRcv=false ); // 当加入IO复用时用此方法代替syncRequest()

	virtual int qrun( int flag, long p2 );
	virtual int onClose( int p1, long p2 );

protected:
	virtual int notifyParent(int evtype, ...);
	int invokeNotifyCB( const string& notifyName, void* param ) const;

private:
	void reset( void );
	int taskRun( int flag, long p2 );
	int appendTimerq( void );


private:
	int m_appid;
	int m_epfd;

	int m_stage;
	int m_seqid;
	int m_port;
	int m_cliType;
	bool m_inqueue;
	bool m_existLink; // 是否已连接中
	bool m_inEpRun;
	string m_rhost;
	string m_svrname;
	string m_mconf;

	SyncHand m_syncHand;
	map<string, NotifyCBFunc> m_ntfCB; // 未做多个相同事件消费

	static CloudApp* This;
};

#endif
