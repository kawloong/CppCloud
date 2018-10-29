/*-------------------------------------------------------------------------
FileName     : tcp_invoker.h
Description  : 服务治理之调用者（消费者）
remark       : 与provider相对
Modification :
--------------------------------------------------------------------------
   1、Date  2018-10-29       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _TCP_INVOKER_H_
#define _TCP_INVOKER_H_



using namespace std;



class TcpInvoker
{
public:
	TcpInvoker( const string& hostport );
	TcpInvoker( const string& host, int port );
	~TcpInvoker( void );

	int init( int rcvto_sec );
	int connect( bool force );

private:
	int m_fd;
	int m_reqcount;
	unsigned m_seqid;
	int m_timeout_sec;
	time_t m_begtime;
	time_t m_atime;
	string m_rhost;
	int m_port;
	bool m_broker;

};

#endif
