/******************************************************************* 
 *  summery:        http的收发封装类
 *  author:         hejl
 *  date:           2016-05-12
 *  description:    
 ******************************************************************/

#ifndef SIMPLE_HTTP_H
#define SIMPLE_HTTP_H

#include <string>

/*
example:
    
{
    string req("http://www.100msh.com/api?key1=val1&key2=val2");
    string response;
    CSimpleHttp http(req);
    http.setTimeout(2000);
    http.doGet();
    response = http.getResponse();
}
*/

// socket简单实现http协议get和post通讯
class CSimpleHttp
{
public:
    enum EHttpHelperErrCode
    {
        E_HTTP_OK                   = 0,
        E_HTTP_UNKNOWN_PROTOCOL     = -1,
        E_HTTP_URL_ILLEGAL          = -2,
        E_HTTP_UNKNOWN_HOST         = -3,
        E_HTTP_CAN_NOT_CONNECT      = -4,
        E_HTTP_SEND_FAIL            = -5,
        E_HTTP_RECV_HEAD_FAIL       = -6,
        E_HTTP_RECV_BODY_FAIL       = -7,
        E_HTTP_OTHER_ERROR          = -8,
        E_HTTP_RESP_STATUS_ERROR    = -9,
        E_HTTP_RSP_TOOBIG           = -10,
        E_HTTP_FILE                 = -11,
    };

    CSimpleHttp();
    CSimpleHttp(const std::string &url);
    CSimpleHttp(const std::string &server, int port, const std::string &object);
    ~CSimpleHttp();
    void closeConnect(void);

    void reset(const std::string &url);
    void reset(const std::string &server, int port);
    void setObject(const std::string &object);

    const std::string& getResponse() const {
        return m_response;
    }

    const std::string& getErrMsg() const {
        return m_errmsg;
    }

    const std::string& getHttpStatus() const {
        return m_httpStatus;
    }

    void setTimeout(int msec) {
        m_timeout_ms = msec;
    }

    unsigned int getConnCount() const {
        return m_connect_count;
    }

    int doGet();
    int doPost(const std::string &data);
    int doPostFile(const std::string& filename);
    int testConnect(int timeout_ms); // 测试tcp是否可达

protected:
    int parseUrl(const std::string& url);
    int request(bool usePost, const std::string &data);
    int connect_timeout( int& sockfd, int timeout_ms );

protected:
    std::string m_response;
    std::string m_httpStatus;
    std::string m_errmsg;
    std::string m_server;
    int         m_port;
    std::string m_object;
    bool        m_parsed;
    int         m_timeout_ms; //毫秒
    int         m_sockfd;
    unsigned int m_connect_count;
    unsigned int m_req_count;

private:
    CSimpleHttp(const CSimpleHttp&);
    CSimpleHttp& operator=(const CSimpleHttp&);
};

#endif // SIMPLE_HTTP_H

