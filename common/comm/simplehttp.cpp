#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <cerrno>
#include <sstream>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include "simplehttp.h"

using namespace std;
const int INVALID_SOCKFD = -1;
/*#define SIMPLEHTTP_INIT_MEMBER {m_port = 0; m_parsed = false; m_jsonMime=true, m_timeout_ms = 0;\
     m_sockfd = INVALID_SOCKFD; m_connect_count = 0;m_req_count = 0;}
*/
#define SIMPLEHTTP_INIT_MEMBER m_port(0), m_parsed(false), m_jsonMime(true), \
     m_timeout_ms(0), m_sockfd(INVALID_SOCKFD), m_connect_count(0),m_req_count(0)

CSimpleHttp::CSimpleHttp():SIMPLEHTTP_INIT_MEMBER {

}

CSimpleHttp::CSimpleHttp(const std::string &url):SIMPLEHTTP_INIT_MEMBER {
    parseUrl(url);
}

CSimpleHttp::CSimpleHttp(const std::string &server, int port, const std::string &object):
    SIMPLEHTTP_INIT_MEMBER {
    m_server = server;
    m_port = port;
    m_object = object;
}

CSimpleHttp::~CSimpleHttp()
{
    this->closeConnect();
}

void CSimpleHttp::closeConnect(void)
{
    if (INVALID_SOCKFD != m_sockfd)
    {
        close(m_sockfd);
        m_sockfd = INVALID_SOCKFD;
    }
}


void CSimpleHttp::reset(const std::string &url) {
    m_response.clear();
    m_httpStatus.clear();
    m_errmsg.clear();
    
    parseUrl(url);
}

void CSimpleHttp::reset(const std::string &server, int port)
{
    m_response.clear();
    m_httpStatus.clear();
    m_errmsg.clear();

    // 长连接时要前后server一样
    if (INVALID_SOCKFD != m_sockfd)
    {
        if (server.compare(m_server) || port != m_port)
        {
            closeConnect();
        }
    }

    m_server = server;
    m_port = port;
    m_parsed = true;
}

void CSimpleHttp::setObject( const std::string &object )
{
    m_object = object;
    m_errmsg.clear();
}

int CSimpleHttp::parseUrl( const string& url )
{
    if (url.find("http://") != 0)
    {
        return E_HTTP_UNKNOWN_PROTOCOL;
    }

    string::size_type pos = url.find('/', 7);
    string host;
    if (pos == string::npos)
    {
        host = url.substr(7);
        m_object = "/";
    }
    else
    {
        host = url.substr(7, pos - 7);
        m_object = url.substr(pos);
    }

    if (host.empty())
    {
        return E_HTTP_URL_ILLEGAL;
    }

    int port = 80;
    string::size_type colon = host.find(':');
    if (colon != string::npos)
    {
        port = atoi(host.substr(colon + 1).c_str());
        host = host.substr(0, colon);
    }

    if (INVALID_SOCKFD != m_sockfd)
    {
        if (m_server.compare(host) || port != m_port)
        {
            closeConnect();
        }
    }
    
    m_server = host;
    m_port = port;
    m_parsed = true;

    return E_HTTP_OK;
}

int CSimpleHttp::connect_timeout( int& sockfd, int timeout_ms )
{
    int ret;
    bool connect_ok = false;
    struct addrinfo *answer = NULL;

    do 
    {
        if (!m_parsed)
        {
            sockfd = INVALID_SOCKFD;
            ret = E_HTTP_UNKNOWN_HOST;
            break;
        }
        
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            m_errmsg.append("create sockfd fail,");
            m_errmsg.append(strerror(errno));
            ret = E_HTTP_OTHER_ERROR;
            break;
        }

        if (timeout_ms > 0)
        {
            struct timeval timeout;
            timeout.tv_sec = timeout_ms/1000;
            timeout.tv_usec = (timeout_ms%1000)*1000;    
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        }

        char szPort[16] = {0};
        snprintf(szPort, sizeof(szPort), "%d", m_port);

        struct addrinfo hint, *curr = NULL;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_INET;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;

        ret = getaddrinfo(m_server.c_str(), szPort, &hint, &answer);
        if (0 != ret || NULL == answer)
        {
            m_errmsg.append("getaddrinfo fail,");
            m_errmsg.append(strerror(errno));
            ret = E_HTTP_UNKNOWN_HOST;
            break;
        }

        for (curr = answer; curr != NULL; curr = curr->ai_next)
        {
            ret = connect(sockfd, curr->ai_addr, curr->ai_addrlen);
#if 0
            if (ret == 0)
            {
                connect_ok = true;
                break;
            }
#else
            // 只尝试第一个地址
            connect_ok = (0==ret);
            break;
#endif
        }

        if (-1 == ret)
        {
            m_errmsg.append("connect fail,");
            m_errmsg.append(strerror(errno));
        }

        ret = connect_ok? 0: E_HTTP_CAN_NOT_CONNECT;
    }
    while (0);

    if (!connect_ok && -1 != sockfd)
    {
        close(sockfd);
        sockfd = -1;
    }
    
    if (answer)
    {
        freeaddrinfo(answer);
    }

    return ret;
}

int CSimpleHttp::request(bool usePost, const std::string &data)
{
    int ret;
    static const size_t maxConLen = 1024*1000;

    if (INVALID_SOCKFD == m_sockfd)
    {
        ret = connect_timeout(m_sockfd, m_timeout_ms);
        if (ret) return ret;
        ++m_connect_count;
    }

    if (m_object.empty())
    {
        m_object = "/";
    }

    ostringstream oss;
    oss << (usePost ? "POST " : "GET ") << m_object << " HTTP/1.1\r\n";
    // add head: user-agent
    oss << "User-Agent: Mozilla/4.0\r\n";
    oss << "Connection: Keep-Alive\r\n";
    if (m_jsonMime)
    {
        oss << "Content-Type: application/json\r\n";
        oss << "Accept: application/json, */*;\r\n";
    }

    // add head: host
    oss << "Host: " << m_server;
    if (m_port != 80)
    {
        oss << ":" << m_port;
    }

    oss << "\r\nContent-Length: " << (unsigned int)data.size();

#define ERR_RETURN(_true, errmsg, retn) \
    if(_true){m_errmsg += errmsg; close(m_sockfd); m_sockfd=INVALID_SOCKFD; return retn; }

    oss << "\r\nCache-Control: no-cache\r\n\r\n";
    string header = oss.str();
    const char *pData = header.data();
    size_t size = header.size();

    size_t sendSize = 0;
    while (sendSize < size)
    {
        ret = send(m_sockfd, pData + sendSize, size - sendSize, 0);
        ERR_RETURN(ret<=0, strerror(errno), E_HTTP_SEND_FAIL);

        sendSize += ret;
    }

#ifdef DEBUG
    printf("senddata:\n%s", pData);
#endif

    if (!data.empty())
    {
        pData = data.data();
        size = data.size();
        sendSize = 0;
        while (sendSize < size)
        {
            ret = send(m_sockfd, pData + sendSize, size - sendSize, 0);
            ERR_RETURN(ret<=0, strerror(errno), E_HTTP_SEND_FAIL);
            sendSize += ret;
        }

#ifdef DEBUG
        printf("%s\n", pData);
#endif
    }

    char recvBuf[512];
    string::size_type headEnd;
    size_t recv_len = 0;
    string respHeader;
    while ((headEnd = respHeader.find("\r\n\r\n")) == string::npos)
    {
        ret = recv(m_sockfd, recvBuf, sizeof(recvBuf), 0);
        ERR_RETURN(ret<=0, strerror(errno), E_HTTP_RECV_HEAD_FAIL);

        ERR_RETURN(recv_len + ret > maxConLen, "http head too big", E_HTTP_RSP_TOOBIG);
        
        respHeader.append(recvBuf, ret);
        recv_len += ret;
    }
    headEnd += 4;

    int contentLength = -1;
    string::size_type clBegin = respHeader.find("Content-Length:");
    if (clBegin != string::npos && clBegin < headEnd)
    {
        clBegin += 15;
        string::size_type clEnd = respHeader.find("\r\n", clBegin);
        if (clEnd != string::npos)
        {
            contentLength = atoi(respHeader.substr(clBegin, clEnd - clBegin).c_str());
            if (contentLength > (int)maxConLen)
            {
                contentLength = maxConLen; // 有最大接收的限制,不能无限长接收
            }
        }
    }

    bool close_connect = false;
    clBegin = respHeader.find("Connection:");
    if (clBegin != string::npos && clBegin < headEnd)
    {
        close_connect = ( string::npos != respHeader.find("close", clBegin+10) ||
                          string::npos != respHeader.find("Close", clBegin+10) );
    }

    m_response.clear();
    if (contentLength > 0)
    {
        if (respHeader.size() < headEnd + contentLength)
        {
            if (respHeader.size() > headEnd)
            {
                m_response = respHeader.substr(headEnd);
            }

            int left = contentLength - m_response.size();
            while (left > 0)
            {
                int readBytes = ((unsigned)left > sizeof(recvBuf) ? sizeof(recvBuf) : left);
                ret = recv(m_sockfd, recvBuf, readBytes, 0);
                ERR_RETURN(ret<=0, strerror(errno), E_HTTP_RECV_BODY_FAIL);

                left -= ret;
                recv_len += ret;
                m_response.append(recvBuf, ret);
            }
        }
        else
        {
            m_response = respHeader.substr(headEnd, contentLength);
        }
    }
    else if (contentLength < 0) // 如果响应中没有Content-Length的情况
    {
        static const char chunkedcoding[] = "Transfer-Encoding";
        string::size_type tranfer;

        if (respHeader.size() > headEnd) // 把头中接收多余部分移动m_response
        {
            m_response = respHeader.substr(headEnd);
        }

        tranfer = respHeader.find(chunkedcoding);
        if (string::npos != tranfer &&
            string::npos != respHeader.find("chunked", sizeof(chunkedcoding)))
        {
            // 可变分块包体
            unsigned long chunksize = 0;
            const size_t crlf_len = 2;
            string::size_type chunkbeg = 0;
            string::size_type chunksep;
            string strsize;

            do
            {
                chunksep = m_response.find("\r\n", chunkbeg);
                while ( chunksep == string::npos )
                {
                    ERR_RETURN(m_response.size()-chunkbeg > 10, "chunksize invalid", E_HTTP_OTHER_ERROR);

                    ret = recv(m_sockfd, recvBuf, sizeof(recvBuf), 0);
                    ERR_RETURN(ret<=0, string("recv not finish:")+strerror(errno), E_HTTP_RECV_BODY_FAIL);

                    m_response.append(recvBuf, ret);
                    recv_len += ret;

                    ERR_RETURN(recv_len > maxConLen, "http body too big", E_HTTP_RSP_TOOBIG);
                    chunksep = m_response.find("\r\n", chunkbeg);
                }

                string strsize(m_response, chunkbeg, chunksep-chunkbeg);
                chunksize = strtoul(strsize.c_str(), NULL, 16);
                ERR_RETURN(chunksize>(long)maxConLen, "chunksize too big:"+strsize, E_HTTP_RSP_TOOBIG);

                m_response.erase(chunkbeg, chunksep-chunkbeg + crlf_len);
                if (0 == chunksize)
                {
                    m_response.erase(chunkbeg);
                    break;
                }

                while (m_response.size() - chunkbeg < chunksize) // 实际数据部分
                {
                    ret = recv(m_sockfd, recvBuf, sizeof(recvBuf), 0);
                    ERR_RETURN(ret<=0, string("recv chunk not finish:")+strerror(errno), E_HTTP_RECV_BODY_FAIL);

                    m_response.append(recvBuf, ret);
                    recv_len += ret;

                    ERR_RETURN(recv_len > maxConLen, "http body too big", E_HTTP_RSP_TOOBIG);
                }

                chunkbeg += chunksize;
                while (m_response.size() - chunkbeg < crlf_len) // 尾部的\r\n 
                {
                    ret = recv(m_sockfd, recvBuf, sizeof(recvBuf), 0);
                    ERR_RETURN(ret<=0, string("recv chunk not finish:")+strerror(errno), E_HTTP_RECV_BODY_FAIL);

                    m_response.append(recvBuf, ret);
                    recv_len += ret;

                    ERR_RETURN(recv_len > maxConLen, "http body too big", E_HTTP_RSP_TOOBIG);
                }

                m_response.erase(chunkbeg, crlf_len);
            }
            while (true);
        }
        else // Connection: close
        {
            while ( (ret = recv(m_sockfd, recvBuf, sizeof(recvBuf), 0)) > 0 )
            {
                m_response.append(recvBuf, ret);
                recv_len += ret;
                ERR_RETURN(recv_len > maxConLen, "http resp too big", E_HTTP_RSP_TOOBIG);
            }

            if (ret < 0)
            {
                m_errmsg.append(strerror(errno));
            }

            close_connect = true;
        }
    }

    if (close_connect) // 可能服务端不支持长连接
    {
        closeConnect();
    }

    respHeader.erase(headEnd);
    string::size_type httpCodeBegin = respHeader.find(' ');
    if (httpCodeBegin != string::npos)
    {
        ++httpCodeBegin;
        string::size_type httpCodeEnd = respHeader.find(' ', httpCodeBegin);
        if (httpCodeEnd != string::npos)
        {
            m_httpStatus = respHeader.substr(httpCodeBegin, httpCodeEnd - httpCodeBegin);
        }
    }

    ++m_req_count;
    ret = atoi(m_httpStatus.c_str());
    if (ret != 200)
    {
        if (ret <= 0)
        {
            m_errmsg.append(strerror(errno));
            return E_HTTP_RESP_STATUS_ERROR;
        }
        return ret;
    }

    return E_HTTP_OK;
}

int CSimpleHttp::doGet(){
    return request(false, "");
}

int CSimpleHttp::doPost(const std::string &data) {
    return request(true, data);
}

int CSimpleHttp::doPostFile( const std::string& filename )
{
    int ret = -1;
    string data;
    FILE* file = fopen(filename.c_str(), "rb");
    if (file)
    {
        char buff[2048];

        do 
        {
            ret = fread(buff, 1, sizeof(buff), file);
            if (ret > 0)
            {
                data.append(buff, ret);
            }
        }
        while (sizeof(buff) == ret); 
        fclose(file);   
    }
    
    if (-1 == ret)
    {
        m_errmsg.append(strerror(errno));
        ret = E_HTTP_FILE;
    }
    else
    {
        ret = request(true, data);
    }

    return ret;
}


int CSimpleHttp::testConnect( int timeout_ms )
{
    int ret;
    int sockfd = -1;

    ret = connect_timeout(sockfd, timeout_ms);
    if (-1 != sockfd)
    {
        close(sockfd);
    }

    return ret;
}



