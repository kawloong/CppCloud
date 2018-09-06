/*-------------------------------------------------------------------------
FileName     : iobuff.h
Description  : tcp协议报文处理类
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-01-29       create     hejl 
-------------------------------------------------------------------------*/

#ifndef _IOBUFF_H_
#define _IOBUFF_H_
#include <string>
#include <arpa/inet.h>
#include "scomm_msg.h"

using std::string;
typedef sc_msg_head_t head_t;
enum { HEADER_LEN = sizeof(head_t) };

// 代表一个收发协议，包括读和写
struct IOBuffItem
{
	unsigned len;
	unsigned totalLen; // 包括head+body的长度
	unsigned seqId;
	string buff;
	
	
public:
	IOBuffItem(void): len(0), totalLen(0), seqId(0){}
	
	head_t* head( void ) { return (buff.length() >= HEADER_LEN)? (head_t*)buff.data(): NULL; }
	char* body( void ) { return (buff.length() >= HEADER_LEN)? (char*)buff.data()+HEADER_LEN: NULL; }
	
	bool ioFinish( void ) { return (len > 0 && !buff.empty() && len >= totalLen); }
	
	void ntoh( void ) // （接收)字节序转化
	{
		if (buff.length() >= HEADER_LEN)
		{
			head_t* phead = (head_t*)buff.data();
			phead->body_len = ntohl(phead->body_len);
			phead->cmdid = ntohs(phead->cmdid);
			phead->seqid = ntohs(phead->seqid);
			seqId = phead->seqid;
			totalLen = phead->body_len + phead->head_len;
		}
	}
	
	// 设置待发送数据
	void setData( unsigned short cmdid, unsigned short seqid, const char* body, unsigned int bodylen )
	{
		char hstr[HEADER_LEN];
		
		head_t* phead = (head_t*)hstr;
		phead->ver = g_msg_ver;
		phead->head_len = HEADER_LEN;
		phead->body_len = htonl(bodylen);
		phead->cmdid = htons(cmdid);
		phead->seqid = htons(seqid);
		buff.assign(hstr, HEADER_LEN);
		buff.append(body, bodylen);
		
		len = 0;
		totalLen = HEADER_LEN + bodylen;
	}
};


#endif
