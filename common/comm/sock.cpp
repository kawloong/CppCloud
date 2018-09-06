/*-------------------------------------------------------------------------
FileName     : socket.h
Description  : 异步socket相关操作实现
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2015-11-05       create     hejl 
-------------------------------------------------------------------------*/
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "sock.h"

#ifdef SOCKET_LOG
#include "public.h" // deps ERRLOG_IF1BRK macro
#else
#define ERRLOG_IF1BRK(exp, n, format, ...) if(exp){ ret=n; break; }
#define IFBREAK_N(exp, n) if((exp)){ret=n; break;}
#define LOGERROR(...) do{}while(0)
#endif

// 默认返回noblock sockfd
int Sock::create_fd(const char* ip, int port, bool udp, bool v6)
{
    const char* ipaddr = ip;
    int s = -1;
    int ret = 0;

    do 
    {
        int reuseaddr = 1;
        if (v6)
        {
            struct sockaddr_in6 ip6addr;

            bzero(&ip6addr, sizeof(ip6addr));
            ip6addr.sin6_family = AF_INET6;
            ip6addr.sin6_port = htons(port); 

            if (NULL==ip || 0==*ip) // 任意本地接口
            {
                ip6addr.sin6_addr = in6addr_any;
            }
            else
            {
                ret = inet_pton(AF_INET6, ipaddr, &ip6addr.sin6_addr); 
                ERRLOG_IF1BRK(ret<=0, ret, "INET_PTON| ret=%d| err=%s| ipaddr=%s",
                    ret, strerror(errno), ip);
            }

            s = socket(AF_INET6, (udp? SOCK_DGRAM: SOCK_STREAM), 0);
            ERRLOG_IF1BRK(s<0, s, "SOCKET| s=%d| err=%s| ipaddr=%s",
                s, strerror(errno), ip);
            if ( -1 == setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                (const void *) &reuseaddr, sizeof(int)) )
            {
                ret = errno;
                perror("LISTEN| msg=reuse addr fail");
                //LOGERROR("LISTEN| msg=reuse addr fail| err(%d)=%s", ret, strerror(ret));
                break;
            }

            ret = bind(s, (struct sockaddr*)&ip6addr, sizeof(ip6addr));
            ERRLOG_IF1BRK(ret, s, "SOCKBIND| err(%d)=%s", errno, strerror(errno));
        }
        else
        {
            struct sockaddr_in ip4addr;

            bzero(&ip4addr, sizeof(ip4addr));
            ip4addr.sin_family = AF_INET;
            ip4addr.sin_port = htons(port); 

            if (NULL==ip || 0==*ip) // 任意本地接口
            {
                ip4addr.sin_addr.s_addr = INADDR_ANY;
            }
            else
            {
                ret = inet_pton(PF_INET, ipaddr, (void*)&ip4addr.sin_addr.s_addr); 
                ERRLOG_IF1BRK(ret<=0, ret, "INET_PTON| ret=%d| err=%s| ipaddr=%s",
                    ret, strerror(errno), ip);
            }

            s = socket(AF_INET, (udp? SOCK_DGRAM: SOCK_STREAM), 0);
            ERRLOG_IF1BRK(s<0, s, "SOCKET| s=%d| err=%s| ipaddr=%s",
                s, strerror(errno), ip);

            if ( -1 == setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                (const void *) &reuseaddr, sizeof(int)) )
            {
                ret = errno;
                perror("LISTEN| msg=reuse addr fail");
                //LOGERROR("LISTEN| msg=reuse addr fail| err(%d)=%s", ret, strerror(ret));
                break;
            }

            ret = bind(s, (struct sockaddr*)&ip4addr, sizeof(struct sockaddr));
            ERRLOG_IF1BRK(ret, s, "SOCKBIND| err(%d)=%s", errno, strerror(errno));
        }

        if (!udp)
        {
            ret = listen(s, LISTEN_BACKLOG);
            ERRLOG_IF1BRK(ret, s, "LISTEN| s=%d| err=%s| ipaddr=%s",
                s, strerror(errno), ip);

            sock_nonblock(s);
        }        
    }
    while (false);

    if (ret && -1 != s)
    {
        close(s);
        s = -1;
    }
    return s;
}

template<bool TPeer>
std::string Sock::_getName_(int fd, bool hasport /*= false*/, bool v6 /*= false*/)
{
   int result, ret;
    socklen_t addr_len;
    std::string name;

    do
    {
        if (v6)
        {
            struct sockaddr_in6 addr6;
            addr_len = sizeof(addr6);
            result = TPeer? getpeername(fd, (sockaddr*)&addr6, &addr_len) : getsockname(fd, (sockaddr*)&addr6, &addr_len);
            ERRLOG_IF1BRK(result, result, "SOCKNAME| err(%d)=%s", errno, strerror(errno));
            sock6_ntop(straddr, addr6);
            name = straddr;
            if (hasport)
            {
                char strport[16] = {0};
                snprintf(strport, sizeof(strport), ":%d", (int)sock6_port(addr6));
                name += strport;
            }
        }
        else
        {
            struct sockaddr_in addr4;
            addr_len = sizeof(addr4);
            result = TPeer? getpeername(fd, (sockaddr*)&addr4, &addr_len) : getsockname(fd, (sockaddr*)&addr4, &addr_len);
            ERRLOG_IF1BRK(result, result, "SOCKNAME| err(%d)=%s", errno, strerror(errno));
            sock4_ntop(straddr, addr4);
            name = straddr;
            if (hasport)
            {
                char strport[16] = {0};
                snprintf(strport, sizeof(strport), ":%d", (int)sock4_port(addr4));
                name += strport;
            }
        }
    }
    while(false);
    return name;
}

std::string Sock::sock_name(int fd, bool hasport /*= false*/, bool v6 /*= false*/)
{
    return _getName_<false>(fd, hasport, v6);
}

std::string Sock::peer_name(int fd, bool hasport/* = false*/, bool v6/* = false*/)
{
    return _getName_<true>(fd, hasport, v6);
}


int Sock::recv(int fd, char* buff, unsigned& begpos, unsigned endpos)
{
    int ret = 0;
    int nread, nleft;
    unsigned length_byte;
    char* pdata;

    do 
    {
        ERRLOG_IF1BRK(fd<0 || 0==buff || begpos >= endpos, ERRSOCK_PARAM,
            "SOCKRECV| fd=%d| buff=%p| beg=%u| end=%u", fd, buff, begpos, endpos);

        length_byte = endpos - begpos;
        pdata = buff + begpos;
        nread = ::recv(fd, pdata, length_byte, MSG_PEEK);
        if (nread < 0)
        {
            int eno = errno;
            IFBREAK_N(eno==EAGAIN || eno==EINTR, ERRSOCK_AGAIN);
            LOGERROR("SOCKRECV| fd=%d| err(%d)=%s", fd, eno, strerror(eno));
            eno = 0;
            ret = ERRSOCK_FAIL;
            break;
        }
        
        IFBREAK_N(0==nread, ERRSOCK_CLOSE); // normal close
        ERRLOG_IF1BRK((unsigned)nread>length_byte, ERRSOCK_UNKNOW, // overwrite ?
            "SOCKRECV| fd=%d| nread=%d| lengthbyte=%u", fd, nread, length_byte);

        nleft = nread;
        while (nleft > 0) 
        {
            if ((nread = read(fd, pdata, nleft)) < 0)
            {
                int eno = errno;
                if (eno == EINTR || eno == EAGAIN)
                {
                    nread = 0;
                }
                else
                {
                    LOGERROR("SOCKRECV| fd=%d| err(%d)=%s", fd, eno, strerror(eno));
                    ret = ERRSOCK_FAIL;
                    break;
                }
            } 
            else if (nread == 0)
            {
                break;
            }

            nleft -= nread;
            pdata += nread;
            begpos += nread;
            ret += nread;
        }
    } while (false);

    return ret;
}

int Sock::send(int fd, char* buff, unsigned& begpos, unsigned endpos)
{
    int ret = 0;
    int nwrite;
    int trytime = 2;
    unsigned length_byte;
    char* pdata;

    while (--trytime > 0) 
    {
        ERRLOG_IF1BRK(fd<0 || 0==buff || begpos >= endpos, ERRSOCK_PARAM,
            "SOCKSEND| fd=%d| buff=%p| beg=%u| end=%u", fd, buff, begpos, endpos);
        length_byte = endpos - begpos;
        pdata = buff + begpos;

        nwrite = ::send(fd, pdata, length_byte, MSG_DONTWAIT);
        if (nwrite < 0)
        {
            int eno = errno;
            IFBREAK_N(EAGAIN==eno||EWOULDBLOCK==eno, ERRSOCK_AGAIN);
            if (EINTR==eno)
            {
                ++trytime;
                continue;
            }
            else
            {
                LOGERROR("SOCKSEND| fd=%d| err(%d)=%s", fd, eno, strerror(eno));
                ret = ERRSOCK_FAIL;
                break;
            }
        }
        
        begpos += nwrite;
        ret += nwrite;
    }

    return ret;
}

int Sock::connect(int& fd, const char* ip, int port, bool v6 /*= false*/)
{
    int ret;
    int svrfd = -1;

    do
    {
        if (v6)
        {
            struct sockaddr_in6	addr6;
            bzero(&addr6, sizeof(addr6));
            addr6.sin6_family = AF_INET6;
            addr6.sin6_port = htons(port); 
            ret = inet_pton(AF_INET6, ip, &addr6.sin6_addr);
            ERRLOG_IF1BRK(ret<=0, ERRSOCK_PARAM, "SOCKCONNECT| err(%d)=%s| ip=%s:%d",
                errno, strerror(errno), ip, port);

            svrfd = socket(AF_INET6, SOCK_STREAM, 0);
            sock_nonblock(svrfd);
            ret = ::connect(svrfd, (struct sockaddr*)&addr6, sizeof(addr6));
        }
        else
        {
            struct sockaddr_in	addr4;
            bzero(&addr4, sizeof(addr4));
            addr4.sin_family = AF_INET;
            addr4.sin_port = htons(port);
            ret = inet_pton(AF_INET, ip, &addr4.sin_addr);
            ERRLOG_IF1BRK(ret<=0, ERRSOCK_PARAM, "SOCKCONNECT| ret=%d| err(%d)=%s| ip=%s:%d",
                ret, errno, strerror(errno), ip, port);

            svrfd = socket(AF_INET, SOCK_STREAM, 0);
            sock_nonblock(svrfd);
            ret = ::connect(svrfd, (struct sockaddr*)&addr4, sizeof(addr4));
        }

        if (ret)
        {
            int eno = errno;

            if (EINPROGRESS == eno)
            {
                // non-block socket connecting
                ret = ERRSOCK_AGAIN;
                fd = svrfd;
                break;
            }
            if (svrfd > 0)
            {
                close(svrfd);
            }
            
            LOGERROR("SOCKCONNECT| err(%d)=%s| ip=%s:%d",
                eno, strerror(eno), ip, port);
            ret = ERRSOCK_CONNECT;
            break;
        }
        ret = 0;
        fd = svrfd;
    }
    while(0);

    return ret;
}

int Sock::geterrno(int fd)
{
    int err;
    socklen_t len;
    int status = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    return status;
}
