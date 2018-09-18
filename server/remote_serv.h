/*-------------------------------------------------------------------------
FileName     : remote_serv.h
Description  : 分布式的除自身外的服务端
remark       : 远端serv的别名引用是serv_nnn
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-10       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _REMOTE_SERV_H_
#define _REMOTE_SERV_H_
#include "iohand.h"
#include <string>

using namespace std;


const int REMOTESERV_EXIST_CHKTIME = 20*1000; // ms unit

class RemoteServ: public IOHand
{
public:
    HEPCLASS_DECL(RemoteServ, RemoteServ);
    RemoteServ(void);
    virtual ~RemoteServ(void);

	static void Init( int mysvrid );
	int init( const string& rhost, int port, int epfd );
	void setSvrid( int svrid );
	int appendTimerq( void );

public: // interface HEpBase
    virtual int qrun( int flag, long p2 );
	virtual int onEvent( int evtype, va_list ap );

protected:
	int taskRun( int flag, long p2 );
	int onClose( int p1, long p2 );

	int prepareWhoIam( void );
	int exitRun( int flag, long p2 );

public:
	static int s_my_svrid;

protected:
	int m_stage;
	int m_seqid;
	int m_svrid;
	int m_epfd;
	int m_port;
	bool m_inqueue;
	string m_rhost;
};

#endif
