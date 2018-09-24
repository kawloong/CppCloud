#include "broadcastcli.h"
#include "cloud/const.h"
#include "comm/strparse.h"
#include "homacro.h"
#include "iohand.h"
#include "switchhand.h"
#include "climanage.h"
#include "outer_serv.h"
#include "exception.h"

HEPCLASS_IMPL_FUNC(BroadCastCli, OnBroadCMD)

int BroadCastCli::s_my_svrid = 0;

BroadCastCli::BroadCastCli()
{

}

void BroadCastCli::init( int my_svrid )
{
    m_seqid = 0;
    s_my_svrid = my_svrid;
    SwitchHand::Instance()->appendQTask(this, my_svrid*1000);
}

int BroadCastCli::run(int p1, long p2)
{
    return -1;
}

int BroadCastCli::qrun( int flag, long p2 )
{
    int ret = 0;
    if (HEFG_PEXIT != flag)
    {
        string reqbody;
        string localEra = CliMgr::Instance()->getLocalClisEraJson(); // 获取本地cli的当前版本

        if (!localEra.empty())
        {
            ret = toWorld(s_my_svrid, localEra, 1, "", "");
        }

        SwitchHand::Instance()->appendQTask(this, BROADCASTCLI_INTERVAL_SEC*1000);
    }
    
    return ret;
}

int BroadCastCli::toWorld( int svrid, int ttl, const string& era, const string &excludeSvrid, const string &route )
{
    // 获取所有直连的Serv
    int ret = 0;
    CliMgr::AliasCursor alcr(REMOTESERV_ALIAS_PREFIX);
    CliBase *cli = NULL;
    string link_svrid_arr(excludeSvrid);
    string routepath = route + StrParse::Itoa(s_my_svrid) + ">";
    vector<CliBase *> vecCli;

    while ((cli = alcr.pop()))
    {
        WARNLOG_IF1BRK(cli->getCliType() != SERV_CLITYPE_ID && !cli->isLocal(), -23,
                       "BROADCASTRUN| msg=flow unexception| cli=%s", cli->m_idProfile.c_str());
        string svridstr = cli->getProperty(CONNTERID_KEY);
        if (link_svrid_arr.find(svridstr + ",") == string::npos)
        {
            link_svrid_arr += svridstr + ",";
            vecCli.push_back(cli);
        }
    }

    string reqbody = "{";
    StrParse::PutOneJson(reqbody, CONNTERID_KEY, svrid, true);
    StrParse::PutOneJson(reqbody, "era", era, true);
    StrParse::PutOneJson(reqbody, "ttl", ttl, true);
    StrParse::PutOneJson(reqbody, EXCLUDE_SVRID_LIST, link_svrid_arr, true);
    StrParse::PutOneJson(reqbody, ROUTE_PATH, routepath, false);
    reqbody += "}";

    for (unsigned i = 0; i < vecCli.size(); ++i)
    {
        IOHand *cli = dynamic_cast<IOHand *>(vecCli[i]);
        cli->sendData(CMD_BROADCAST_REQ, ++m_seqid, reqbody.c_str(), reqbody.length(), true);
    }

    return ret;
}

int BroadCastCli::OnBroadCMD( void* ptr, unsigned cmdid, void* param )
{
    CMDID2FUNCALL_BEGIN
    LOGDEBUG("BROADCMD| seqid=%u| cmd=%u| body=%s", seqid, cmdid, body);
    CMDID2FUNCALL_CALL(CMD_BROADCAST_REQ)
    //CMDID2FUNCALL_CALL(CMD_BROADCAST_RSP) 

    return -5;
}

int BroadCastCli::on_CMD_BROADCAST_REQ( IOHand* iohand, const Value* doc, unsigned seqid )
{
    int ret;
    int osvrid = 0;
    string era = 0;
    int ttl = 1;
    string str_osvrid;
    string excludeSvrid;
    string routepath;
    
    ret = Rjson::GetInt(osvrid, CONNTERID_KEY, doc);
    ret |= Rjson::GetStr(era, "era", doc);
    ret |= Rjson::GetInt(ttl, "ttl", doc);
    ret |= Rjson::GetStr(excludeSvrid, EXCLUDE_SVRID_LIST, doc);
    ret |= Rjson::GetStr(routepath, ROUTE_PATH, doc);
    str_osvrid = StrParse::Itoa(osvrid);

    NormalExceptionOn_IFTRUE(ret||osvrid<=0||era.empty(), 409, CMD_BROADCAST_RSP, seqid, 
        string("invalid CMD_BROADCAST_REQ body ")+Rjson::ToString(doc));
    
    ret = BroadCastCli::Instance()->toWorld(osvrid, ++ttl, era, excludeSvrid, routepath); // 将消息广播出去

    // 自身处理
    // 1. 本Serv是否存在编号为osvrid的对象
    string near_servid = iohand->getProperty(CONNTERID_KEY); // 当前直接传的消息的serv编号
    const string aliasPrefix(REMOTESERV_ALIAS_PREFIX);
    CliMgr::AliasCursor citr(aliasPrefix + str_osvrid);
    CliBase* servptr = citr.pop();
    string old_era;
    bool needall = false;

    if (NULL == servptr) // 来自一个新的Serv上报
    {
        NormalExceptionOn_IFTRUE(near_servid==str_osvrid, 410, CMD_BROADCAST_RSP,
            seqid, StrParse::Format("near serv-%s must not outer serv-%d", 
            near_servid.c_str(), osvrid)); // assert

        OuterServ* outsvr = new OuterServ;
        outsvr->init(osvrid);
        ret = outsvr->setRoutePath(routepath);
        ret |= CliMgr::Instance()->addChild(outsvr);
        ret |= CliMgr::Instance()->addAlias2Child(aliasPrefix + str_osvrid + "s", outsvr);
        ret |= CliMgr::Instance()->addAlias2Child(str_osvrid + "_s", outsvr);
        needall = !era.empty();

        if (ret)
        {
            CliMgr::Instance()->removeAliasChild(outsvr, true);
            throw NormalExceptionOn(411, CMD_BROADCAST_RSP, seqid,
                StrParse::Format("%s:%d addChild ret %d", __FILE__, __LINE__, ret));
        }

        servptr = outsvr;
    }
    else
    // 2. 编号为osvrid的对象的era是否最新，否则请求获取
    {

        if (1 == ttl || servptr->isLocal()) // 直连的Serv发出的广播
        {
            NormalExceptionOn_IFTRUE(near_servid!=str_osvrid, 412, CMD_BROADCAST_RSP,
                seqid, StrParse::Format("first broadcast serv-%d should be near serv-%s ", 
                osvrid, near_servid.c_str())); // assert

            old_era = iohand->getProperty("era");
        }
        else
        {
            OuterServ* outsvr = dynamic_cast<OuterServ*>(servptr);
            NormalExceptionOn_IFTRUE(NULL==outsvr, 413, CMD_BROADCAST_RSP,
                seqid, StrParse::Format("servptr-%s isnot OuterServ instance ", 
                servptr->m_idProfile.c_str())); // assert

            old_era = outsvr->getProperty("era");

            // 更新路由信息
            ret = outsvr->setRoutePath(routepath);
            ERRLOG_IF1(ret, "SETROUTPATH| msg=route path fail| path=%s", routepath.c_str());
        }
    }

    int last_reqera_time = servptr->getIntProperty(LAST_REQ_SERVMTIME);
    int now = time(NULL);
    const int reqall_interval_sec = 10;

    // era新旧判断
    string differa = needall? "all": CliMgr::Instance()->diffOuterCliEra(era);
    
    if (!differa.empty() && now > last_reqera_time + reqall_interval_sec)
    {
        // 请求获取某Serv下的所有cli (CMD_ERAALL_REQ)
        string msgbody;

        msgbody += "{";
        StrParse::PutOneJson(msgbody, "to", osvrid, true);
        StrParse::PutOneJson(msgbody, "from", s_my_svrid, true);
        StrParse::PutOneJson(msgbody, "differa", differa, true);
        //StrParse::PutOneJson(msgbody, "newera", era, true);
        StrParse::PutOneJson(msgbody, "refer_path", routepath, true); // 参考路线
        StrParse::PutOneJson(msgbody, ROUTE_PATH, StrParse::Itoa(s_my_svrid)+">", false); // 已经过的路线
        msgbody += "}";

        ret = iohand->sendData(CMD_ERAALL_REQ, seqid, msgbody.c_str(), msgbody.size(), true);
        servptr->setIntProperty(LAST_REQ_SERVMTIME, now);
        LOGDEBUG("REQERAALL| msg=Serv-%d need %d eraall data| era=%d->%d| differa=%s| refpath=%s| retsend=%d", 
            s_my_svrid, osvrid, old_era, era, differa.c_str(), routepath.c_str(), ret);
    }

    return ret;
}
