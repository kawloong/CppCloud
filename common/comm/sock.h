/*-------------------------------------------------------------------------
FileName     : sock.h
Description  : 异步socket相关操作
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2015-11-05       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _SOCKET_H_
#define _SOCKET_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>

#define sock_nonblock(s)  fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
#define sock_block(s)     fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK)
#define sock4_ntop(str, addr) char str[16]={0}; \
    inet_ntop(AF_INET, (void *)&addr.sin_addr, str, 16);
#define sock6_ntop(str, addr) char str[40]={0}; \
    inet_ntop(AF_INET6, (void *)&addr.sin6_addr, str, 40);
#define sock4_port(addr) ntohs(addr.sin_port)
#define sock6_port(addr) ntohs(addr.sin6_port)

#define LISTEN_BACKLOG 100 // listen()等待队列大小

enum ErrSock
{
    ERRSOCK_UNKNOW = -4,
    ERRSOCK_PARAM = -3, // 参数错误
    ERRSOCK_FAIL = -2,  // socket检测到出错
    ERRSOCK_AGAIN = -1, // noblock socket暂时缓冲区中无数据
    ERRSOCK_CLOSE = 0,  // 收到EOF

    ERRSOCK_CONNECT = -5, // connect fail
};

class Sock
{
public:
    // 创建监听套接字
    static int create_fd(const char* ip, int port, bool udp = false, bool v6 = false); 
    static int connect(int& fd, const char* ip, int port, int timout_sec, bool noblock, bool v6);
    static int connect_noblock(int& fd, const char* ip, int port);

    static std::string sock_name(int fd, bool hasport = false, bool v6 = false);
    static std::string peer_name(int fd, bool hasport = false, bool v6 = false);
    static int geterrno(int fd);

    static int read(int fd, char* buff, unsigned& begpos, unsigned bufflen);
    static int recv(int fd, char* buff, unsigned& begpos, unsigned bufflen);
    static int send(int fd, char* buff, unsigned& begpos, unsigned bufflen);

    static int setRcvTimeOut(int fd, int sec);
    static int setSndTimeOut(int fd, int sec);

private:
    template<bool TPeer> static std::string _getName_(int fd, bool hasport, bool v6);
};

#endif

