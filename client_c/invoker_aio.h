/*-------------------------------------------------------------------------
FileName     : invoker_aio.h
Description  : 服务消费者异步IO处理
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-12-30       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _INVOKER_AIO_H_
#define _INVOKER_AIO_H_
#include <arpa/inet.h>
#include <memory>
#include "comm/lock.h"
#include "comm/hep_base.h"
#include "comm/queue.h"
#include "cloud/iobuff.h"

using namespace std;
typedef long long datasize_t;

struct InvokerException
{
	string reson;
	int code;

	InvokerException(int retcode, const string& reson): 
		reson(reson), code(retcode) { }
};

class InvokerAio: ITaskRun2
{
public:
    InvokerAio( const string& dsthost, int dstport );
    virtual ~InvokerAio( void );


public:
	int init( int epfd );
    virtual int run( int flag, long p2 );
	virtual int onEvent( int evtype, va_list ap );


	int getIOStatJson( string& rspjson ) const;


	int driveClose( const string& reason );
	void setEpThreadID( void );
	void setEpThreadID( pthread_t thid );

	int request( string& resp, int cmdid, const string& body );
	

protected:
	int onRead( int p1, long p2 );
	int onWrite( int p1, long p2 );
	virtual int onClose( int p1, long p2 );
	void clearBuf( void );

	int cmdProcess( IOBuffItem*& iBufItem );
	int sendData( unsigned int cmdid, unsigned int seqid, const char* body, 
		unsigned int bodylen, bool setOutAtonce );

protected:
	const string m_dstHost;
	const int m_dstPort;
	int m_stage; // 所处状态：0 未连接；1 连接中；2 已连接；3连接失败
    int m_cliFd;
	int m_seqid;
	int m_timeout_interval_sec;
	HEpEvFlag m_epCtrl;
	pthread_t m_epThreadID; // epoll-IO复用的线程id
	string m_cliName;
	string m_closeReason; // 关掉原因
	unsigned char m_closeFlag; // 结束标记: 0连接中, 1等待发完后关闭; 2立即要关; 3关闭

	datasize_t m_recv_bytes;
	datasize_t m_send_bytes;
	static datasize_t serv_recv_bytes;
	static datasize_t serv_send_bytes;
	int m_recvpkg_num;
	int m_sendpkg_num;
	static int serv_recvpkg_num;
	static int serv_sendpkg_num;

	IOBuffItem* m_iBufItem;
	IOBuffItem* m_oBufItem;
	Queue<IOBuffItem*, true> m_oBuffq;
	map< int, shared_ptr< Queue<string, false> > > m_reqQueue;
	RWLock m_qLock;
};

#endif
