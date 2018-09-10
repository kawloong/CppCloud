/*-------------------------------------------------------------------------
FileName     : iohand.h
Description  : 收发tcp协议消息
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-01-23       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _IOHAND_H_
#define _IOHAND_H_
#include <arpa/inet.h>
#include "comm/hep_base.h"
#include "comm/queue.h"
#include "iobuff.h"
#include "clibase.h"

class IOHand: public CliBase
{
public:
    HEPCLASS_DECL(IOHand, IOHand);
    IOHand(void);
    virtual ~IOHand(void);

	static int Init( void );

public: // interface HEpBase
    virtual int run( int flag, long p2 );
	virtual int onEvent( int evtype, va_list ap );

	// 自定义属性的操作
    void setProperty( const string& key, const string& val );
    string getProperty( const string& key );
	int getCliType(void) {return m_cliType; }

	int sendData( unsigned int cmdid, unsigned int seqid, const char* body, 
		unsigned int bodylen, bool setOutAtonce );

protected:
	int onRead( int p1, long p2 );
	int onWrite( int p1, long p2 );
	virtual int onClose( int p1, long p2 );

	int cmdProcess( IOBuffItem*& iBufItem );

protected:
    int m_cliFd;
	HEpEvFlag m_epCtrl;
	string m_closeReason; // 关掉原因
	unsigned char m_closeFlag; // 结束标记: 0连接中, 1等待发完后关闭; 2立即要关; 3关闭

	IOBuffItem* m_iBufItem;
	IOBuffItem* m_oBufItem;
	Queue<IOBuffItem*, true> m_oBuffq;

};

#endif
