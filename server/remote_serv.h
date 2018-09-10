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
#include "string"

using namespace std;

const string REMOTESERV_ALIAS_PREFIX = "serv_";
const int REMOTESERV_EXIST_CHKTIME = 5*60*1000; // ms unit

class RemoteServ: public IOHand
{
public:
    HEPCLASS_DECL(RemoteServ, RemoteServ);
    RemoteServ(void);
    virtual ~RemoteServ(void);

	static void Init( int mysvrid );
	int init( int svrid, const string& rhost, int epfd );

public: // interface HEpBase
    virtual int run( int flag, long p2 );
	virtual int onEvent( int evtype, va_list ap );

protected:
	int taskRun( int flag, long p2 );
	int onClose( int p1, long p2 );

	int prepareWhoIam( void );
	int exitRun( int flag, long p2 );

protected:
	int m_stage;
	int m_seqid;
	static int s_my_svrid;
};

#endif
