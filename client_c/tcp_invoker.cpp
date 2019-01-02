#include "tcp_invoker.h"
#include "comm/public.h"
#include "comm/sock.h"
#include "cloud/iobuff.h"
#include "comm/strparse.h"
#include <sys/types.h>
#include <sys/socket.h>

TcpInvoker::TcpInvoker( const string& hostport ): m_fd(INVALID_FD), 
    m_reqcount(0), m_seqid(0), m_timeout_sec(3), m_begtime(0), m_atime(0), 
    m_broker(true), m_waitRsp(false)
{
    size_t pos = hostport.find(":");
    if (pos > 0)
    {
        m_rhost = hostport.substr(0, pos);
        m_port = atoi(hostport.c_str() + pos + 1);
    }
}

TcpInvoker::TcpInvoker( const string& host, int port ): m_fd(INVALID_FD), 
    m_reqcount(0), m_seqid(0), m_timeout_sec(3), m_begtime(0), m_atime(0), 
    m_rhost(host), m_port(port), m_broker(true), m_waitRsp(false)
{
}

TcpInvoker::~TcpInvoker( void )
{
    release();
}

int TcpInvoker::init( int rcvto_sec )
{
    m_timeout_sec = rcvto_sec;
    return connect(false);
}

int TcpInvoker::connect( bool force )
{
    if (check())
    {
        IFRETURN_N(!force, 0);
    }

    IFCLOSEFD(m_fd);

    int ret = Sock::connect(m_fd, m_rhost.c_str(), m_port, m_timeout_sec, false, false);
    m_broker = !(0==ret && INVALID_FD != m_fd);
    if (!m_broker)
    {
        m_begtime = time(NULL);
        m_atime = m_begtime;
        m_waitRsp = false;
        Sock::setRcvTimeOut(m_fd, m_timeout_sec);
        Sock::setSndTimeOut(m_fd, m_timeout_sec);
        LOGDEBUG("INVOKERCONN| msg=connct to %s:%d ok", m_rhost.c_str(), m_port);
    }

    return ret;
}

// 返回是否连接正常
// param: -1 忽略请求/响应状态检查；0 检查是否属等待发送状态； 1 检查是否是等待响应状态
bool TcpInvoker::check( int flowFlag /*=-1*/ ) const
{
    bool flowCheck = (flowFlag >= 0 ? (0==flowFlag? !m_waitRsp : m_waitRsp) : true);
    return INVALID_FD != m_fd && !m_broker && 0 == Sock::geterrno(m_fd) && flowCheck;
}

void TcpInvoker::release( void )
{
    IFCLOSEFD(m_fd);
    m_broker = true;
    m_waitRsp = false;
    m_begtime = 0;
}

int TcpInvoker::send( int cmdid, const string& msg )
{
    IOBuffItem obf;
    int msglen = msg.length();
	obf.setData(cmdid, ++m_seqid, msg.c_str(), msglen);

    int ret = 0;
    int sndbytes = ::send(m_fd, obf.head(), obf.totalLen, 0);
    if (sndbytes != (int)obf.totalLen )
    {
        LOGERROR("INVOKERSEND| host=%s:%d| cmdid=0x%x| ret=%d/%d| sockerrno=%d", 
            m_rhost.c_str(), m_port, cmdid, sndbytes, msglen, Sock::geterrno(m_fd));
        ret = -91;
    }
    else
    {
        m_reqcount++;
        m_waitRsp = true;
        m_atime = time(NULL);
    }

    return ret;
}

// 同步接收tcp服务provider返回
int TcpInvoker::recv( unsigned& rcmdid, string& msg )
{
    static const int close_retcode = -2;
    int ret;
    char* body = NULL;
    // 接收包头
    do
    {
        char buff[HEADER_LEN];
        ret = Sock::setRcvTimeOut(m_fd, m_timeout_sec);
        ret = ::recv(m_fd, buff, HEADER_LEN, 0);
        IFBREAK_N(0 == ret, close_retcode);
        ERRLOG_IF1BRK(ret != HEADER_LEN, -92, "INVOKERRECV| host=%s:%d| ret=%d| sockerrno=%d| dt=%ds",
            m_rhost.c_str(), m_port, ret, Sock::geterrno(m_fd), int(time(NULL)-m_atime));
        
        IOBuffItem inbf;
        inbf.buff.append(buff, HEADER_LEN);
        inbf.ntoh();
        head_t* hdr = inbf.head();

        ERRLOG_IF1BRK(inbf.totalLen > g_maxpkg_len, -93, "INVOKERRECV| msg=head invalid| "
            "host=%s:%d| cmd=0x%x| seq=%u| len=%u",  m_rhost.c_str(), m_port, hdr->cmdid, inbf.seqId, inbf.totalLen);
        
        rcmdid = hdr->cmdid;
        body = new char[hdr->body_len];
        ret = ::recv(m_fd, body, hdr->body_len, 0);
        IFBREAK_N(0 == ret, close_retcode);

        ERRLOG_IF1BRK(ret != (int)hdr->body_len, -94, "INVOKERRECV| msg=body recv fail host=%s:%d| ret=%d| sockerrno=%d| dt=%ds",
            m_rhost.c_str(), m_port, ret, Sock::geterrno(m_fd), int(time(NULL)-m_atime));
        WARNLOG_IF1(hdr->seqid != m_seqid, "INVOKERRECV| msg=resp seqid not match| host=%s:%d| %u!=%u", 
            m_rhost.c_str(), m_port, hdr->seqid, m_seqid);
        msg.assign(body, ret);
        m_waitRsp = false;
        ret = 0;
    }while(0);

    IFDELETE_ARR(body);
    if (ret)
    {
        int dt = time(NULL) - m_atime;
        LOGOPT_EI(close_retcode!=ret, "INVOKERRECV| msg=invoke end| host=%s:%d| dt=%ds", m_rhost.c_str(), m_port, dt);
        release();
    }

    return ret;
}

string TcpInvoker::getKey( void ) const
{
    return m_rhost + ":" + _N(m_port);
}

time_t TcpInvoker::getAtime( void ) const
{
    return m_atime > m_begtime? m_atime : m_begtime;
}