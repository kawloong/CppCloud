/*-------------------------------------------------------------------------
FileName     : hepoll.h
Description  : 封闭高性能epoll的操作
remark       : ET-mode
Modification :
--------------------------------------------------------------------------
   1、Date  2018-01-23       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _HEPOLL_H_
#define _HEPOLL_H_
#include <cstddef>

enum _err_no_ {
    EP_INIT_FAIL = 500,
    EP_CTRL_FAIL,
    EP_WAIT_FAIL,
};

enum he_flag{
	
};

class HEpoll
{
public:
    HEpoll( void );
    ~HEpoll( void );
    int init( void ); // int port, const char* svrhost = NULL, int lqueue=100 
    int run( bool& bexit );
    void unInit( void );
    
    int getEPfd(void) { return m_epfd; }

private:
    int m_epfd; // epoll socket
    int m_lifd; // listen socket
};


#endif