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

class IOHand: public HEpBase
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

protected:
	int onRead( int p1, long p2 );
	int onWrite( int p1, long p2 );
	int onClose( void );

	int cmdProcess( IOBuffItem*& iBufItem );

protected:
    int m_cliFd;
	HEpEvFlag m_epCtrl;
	string m_cliName;
	string m_closeReason; // 关掉原因
	bool m_bClose;
	bool m_ntfEnd; // 被通知结束
	int m_cliType; // 何种类型的客户端: 1 对接进程; 10 监控进程; 20 web serv; 30 观察进程
	//map<HEpBase*, int> m_children;

	IOBuffItem* m_iBufItem;
	IOBuffItem* m_oBufItem;
	Queue<IOBuffItem*, true> m_oBuffq;

public:
	map<string, string> m_cliProp; // 客户属性
};

#endif
