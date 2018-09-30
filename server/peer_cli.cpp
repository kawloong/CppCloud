#include "cloud/const.h"
#include "comm/strparse.h"
#include "climanage.h"
#include "exception.h"
#include "peer_cli.h"
#include "iohand.h"
#include "peer_serv.h"

HEPMUTICLASS_IMPL(PeerCli, PeerCli, HEpBase)

int PeerCli::s_my_svrid = 0;
PeerCli::PeerCli(void): m_iohand(NULL)
{

}

PeerCli::~PeerCli(void)
{
    m_iohand = NULL;
    // 删除各cli .. wait实现
}

void PeerCli::Init( int mysid )
{
    s_my_svrid = mysid;
}

int PeerCli::onEvent( int evtype, va_list ap )
{
	int ret = 0;
	if (HEPNTF_INIT_PARAM == evtype)
	{
		IOHand* ioh = va_arg(ap, IOHand*);
        m_iohand = ioh;
    }
    else if (HEPNTF_NOTIFY_CHILD == evtype)
    {
		IOBuffItem* bufitm = va_arg(ap, IOBuffItem*);
        unsigned cmdid = bufitm->head()->cmdid;
        unsigned seqid = bufitm->head()->seqid;
	    char* body = bufitm->body();
        ret = cmdHandle(cmdid, seqid, body);
    }

    return ret;
}

// return 1 上层IOHand将会继续调用IOHand::cmdProcess()处理
// 注意: 如返回1,则body应该只读,不能改写.
int PeerCli::cmdHandle( unsigned cmdid, unsigned seqid, char* body )
{
#define CMDID2FUNCALL1(CMDID)                                                              \
    if(CMDID==cmdid) { Document doc;                                                      \
        if (doc.ParseInsitu(body).HasParseError()) {                                      \
            throw NormalExceptionOn(404, cmdid|CMDID_MID, seqid, "body json invalid"); }  \
        return on_##CMDID(&doc, seqid);  }


    CMDID2FUNCALL1(CMD_IAMSERV_REQ);
    CMDID2FUNCALL1(CMD_IAMSERV_RSP);

    return 1;
}

int PeerCli::on_CMD_IAMSERV_REQ( const Value* doc, unsigned seqid )
{
    int ret;

    ret = m_iohand->Json2Map(doc);
    ERRLOG_IF1(ret, "IAMSERV_REQ| msg=json2map set prop fail %d| mi=%s", ret, m_iohand->m_idProfile.c_str());

    m_iohand->setCliType(SERV_CLITYPE_ID);

    // 添加到climanage中管理（已添加，被动方）
    string strsvrid = m_iohand->getProperty(CONNTERID_KEY) + "_C";
    if ( CliMgr::Instance()->getChildByName(strsvrid) )
    {
        throw OffConnException(string("svrid exist ")+strsvrid);
    }

    string servAlias = string(SERV_IN_ALIAS_PREFIX) + strsvrid;
    CliMgr::Instance()->addAlias2Child(strsvrid, m_iohand);
    CliMgr::Instance()->addAlias2Child(servAlias, m_iohand);
    m_iohand->m_idProfile = strsvrid + m_iohand->getCliSockName();
    m_iohand->setAuthFlag(1);

    // 响应回复
    string whoIamJson;

	whoIamJson += "{";
	StrParse::PutOneJson(whoIamJson, CONNTERID_KEY, s_my_svrid, true);
	StrParse::PutOneJson(whoIamJson, "svrname", MYSERVNAME, true);
	StrParse::PutOneJson(whoIamJson, CLISOCKET_KEY, m_iohand->getCliSockName(), true);
	StrParse::PutOneJson(whoIamJson, "begin_time", (int)time(NULL), true);
	
	StrParse::PutOneJson(whoIamJson, "pid", getpid(), true);
	StrParse::PutOneJson(whoIamJson, CLIENT_TYPE_KEY, 1, false);
	whoIamJson += "}";

    m_iohand->sendData(CMD_IAMSERV_RSP, seqid, whoIamJson.c_str(), whoIamJson.length(), true);
    LOGINFO("IAMSERV_REQ| msg=recv from Serv at %s", m_iohand->m_idProfile.c_str());

    return 0;
}

int PeerCli::on_CMD_IAMSERV_RSP( const Value* doc, unsigned seqid )
{
    int ret;
    int nsvrid = 0;

    PeerServ* rsev = dynamic_cast<PeerServ*>(m_iohand);
    NormalExceptionOff_IFTRUE(NULL==rsev, 400, CMD_IAMSERV_RSP, seqid, 
            "iohand type must PeerServ class");

    string strsvrid0 = m_iohand->getProperty(CONNTERID_KEY); // 假如此消息发生多次,svrid要一样
    ret = m_iohand->Json2Map(doc);
    string strsvrid = m_iohand->getProperty(CONNTERID_KEY);
    nsvrid = atoi(strsvrid.c_str());

    bool validsvr = (strsvrid0.empty() || strsvrid0 == strsvrid) && (nsvrid > 0);
    NormalExceptionOff_IFTRUE(!validsvr, 400, CMD_IAMSERV_RSP, seqid, 
            string("2 times svrid diff ")+strsvrid);

    rsev->setSvrid(nsvrid);

    // 添加到climanage中管理(主动方)
    strsvrid += "_S"; // 标识来自主动方和被动的serv分开
    string servAlias = string(SERV_IN_ALIAS_PREFIX) + strsvrid;
    ret = CliMgr::Instance()->addChild(m_iohand, false);
    ret |= CliMgr::Instance()->addAlias2Child(strsvrid, m_iohand);
    ret |= CliMgr::Instance()->addAlias2Child(servAlias, m_iohand);
    NormalExceptionOff_IFTRUE(ret, 400, CMD_IAMSERV_RSP, seqid, 
            string("addChild fail ")+strsvrid);
    
    m_iohand->setAuthFlag(1);
    CliMgr::Instance()->updateCliTime(m_iohand);
    m_iohand->m_idProfile = strsvrid + m_iohand->getCliSockName();
    LOGINFO("IAMSERV_RSP| msg=connect to %s success", m_iohand->m_idProfile.c_str());
    
    return ret;
}