#include "remote_cli.h"
#include "iohand.h"

HEPMUTICLASS_IMPL(RemoteCli, RemoteCli, HEpBase)


RemoteCli::RemoteCli(void): m_iohand(NULL)
{

}

RemoteCli::~RemoteCli(void)
{
    m_iohand = NULL;
    // 删除各cli .. wait实现
}

int RemoteCli::onEvent( int evtype, va_list ap )
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

    return 0;
}

// return 1 上层IOHand将会继续调用IOHand::cmdProcess()处理
// 注意: 如返回1,则body应该只读,不能改写.
int RemoteCli::cmdHandle( unsigned cmdid, unsigned seqid, char* body )
{
    switch (cmdid)
    {
        case:
        break;
    }
}

int RemoteCli::on_CMD_IAMSERV_REQ( const Value* doc, unsigned seqid )
{
    int ret;

    ret = m_iohand->Json2Map(doc);
    ERRLOG_IF1(ret, "IAMSERV_REQ| msg=json2map set prop fail %d| mi=%d", ret, m_iohand->m_idProfile.c_str());

    // 添加到climanage中管理（已添加，被动方）

    // 响应回复
    m_iohand->setCliType(1);
    string whoIamJson;

	whoIamJson += "{";
	StrParse::PutOneJson(whoIamJson, "svrid", s_my_svrid, true);
	StrParse::PutOneJson(whoIamJson, "svrname", REMOTESERV_SVRNAME, true);
	StrParse::PutOneJson(whoIamJson, "localsock", m_iohand->getCliSockName(), true);
	StrParse::PutOneJson(whoIamJson, "begin_time", (int)time(NULL), true);
	
	StrParse::PutOneJson(whoIamJson, "pid", getpid(), true);
	StrParse::PutOneJson(whoIamJson, "clitype", 1, false);
	whoIamJson += "}";

    m_iohand->sendData(CMD_IAMSERV_RSP, seqid, whoIamJson.c_str(), whoIamJson.length(), true);
    return 0;
}

int RemoteCli::on_CMD_IAMSERV_RSP( const Value* doc, unsigned seqid )
{
    int ret;
    int svrid = 0;

    ret = m_iohand->Json2Map(doc);


    // 添加到climanage中管理(主动方)

    return 0;
}