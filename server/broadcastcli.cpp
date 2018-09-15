#include "broadcastcli.h"
#include "cloud/const.h"
#include "comm/strparse.h"
#include "iohand.h"
#include "switchhand.h"
#include "climanage.h"

HEPCLASS_IMPL_FUNC(BroadCastCli, OnBroadCMD)

BroadCastCli::BroadCastCli()
{

}

void BroadCastCli::init( void )
{
    m_seqid = 0;
    SwitchHand::Instance()->appendQTask(this, BROADCASTCLI_INTERVAL_SEC*1000);
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
        int localEra = CliMgr::Instance()->getLocalClisEra(); // 获取本地cli的当前版本

        ret = toWorld(s_my_svrid, localEra, "", "");
    }
    
    return ret;
}

int BroadCastCli::toWorld( int svrid, int era, const string &excludeSvrid, const string &route )
{
    // 获取所有直连的Serv
    CliMgr::AliasCursor alcr(REMOTESERV_ALIAS_PREFIX);
    CliBase *cli = NULL;
    string link_svrid_arr(excludeSvrid);
    string routepath = route + string(s_my_svrid) + "->";
    vector<CliBase *> vecCli;

    while ((cli = alcr.pop()))
    {
        WARNLOG_IF1BRK(cli->getCliType() != SERV_CLITYPE_ID && !cli->isLocal(), -23,
                       "BROADCASTRUN| msg=flow unexception| cli=%s", cli->m_idProfile.c_str());
        string svrid = cli->getProperty(CONNTERID_KEY);
        if (link_svrid_arr.find(svrid + ",") == string::npos)
        {
            link_svrid_arr += svrid + ",";
            vecCli.push_back(cli);
        }
    }

    reqbody += "{";
    StrParse::PutOneJson(reqbody, CONNTERID_KEY, svrid, true);
    StrParse::PutOneJson(reqbody, "era", era, true);
    StrParse::PutOneJson(reqbody, EXCLUDE_SVRID_LIST, link_svrid_arr, true);
    StrParse::PutOneJson(reqbody, ROUTE_PATH, routepath, false);
    reqbody += "}";

    for (int i = 0; i < vecCli.size(); ++i)
    {
        IOHand *cli = dynamic_cast<IOHand *>(vecCli[i]);
        cli->sendData(CMD_BROADCAST_REQ, ++m_seqid, reqbody.c_str(), reqbody.length(), true);
    }
}
