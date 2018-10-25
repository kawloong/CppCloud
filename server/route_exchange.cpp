#include "route_exchange.h"
#include "exception.h"
#include "comm/strparse.h"
#include "climanage.h"
#include "homacro.h"
#include "cloud/const.h"
#include "outer_serv.h"
#include "outer_cli.h"


HEPCLASS_IMPL_FUNCX_BEG(RouteExchage)
HEPCLASS_IMPL_FUNCX_MORE(RouteExchage, TransMsg)
HEPCLASS_IMPL_FUNCX_END(RouteExchage)

int RouteExchage::s_my_svrid = 0;
#define RouteExException_IFTRUE_EASY(cond, resonstr) \
    RouteExException_IFTRUE(cond, cmdid, seqid, s_my_svrid, from, resonstr, "")

RouteExchage::RouteExchage( void )
{

}

void RouteExchage::Init( int my_svrid )
{
    s_my_svrid = my_svrid;
}

/**
 * 注意fromSvr参数: >=0时, 是创建最先发出源,0使用s_my_svrid; <0时不改动from键值
 */
int RouteExchage::AdjustRoutMsg( Document& doc, int fromSvr, int* toSvr, int* bto )
{
    int ret = 0;
    int ntmp = 0;

    do
    {
        if (toSvr)
        {
            if (*toSvr > 0)
            {
                Rjson::SetObjMember(ROUTE_MSG_KEY_TO, *toSvr, &doc);
            }
            else
            {
                Rjson::GetInt(ntmp, ROUTE_MSG_KEY_TO, &doc);
                IFBREAK_N(ntmp<=0, -101);
                *toSvr = ntmp;
            }
        }
        
        string actpath;
        fromSvr = (0==fromSvr?s_my_svrid: fromSvr);
        if (fromSvr > 0) // 用本地id // 最原先开始
        {
            IFBREAK_N(toSvr&&*toSvr==fromSvr, -104); // src==dst error
            Rjson::SetObjMember(ROUTE_MSG_KEY_FROM, fromSvr, &doc);
            actpath = _F("%d>", fromSvr);
            Rjson::SetObjMember(ROUTE_MSG_KEY_JUMP, 1, &doc);
        }
        else // 保持不变
        {
            Rjson::GetInt(fromSvr, ROUTE_MSG_KEY_FROM, &doc);
            IFBREAK_N(fromSvr<=0, -103);
            Rjson::GetStr(actpath, ROUTE_MSG_KEY_TRAIL, &doc);
            ntmp = 0;
            Rjson::GetInt(ntmp, ROUTE_MSG_KEY_JUMP, &doc);
            Rjson::SetObjMember(ROUTE_MSG_KEY_JUMP, ++ntmp, &doc);
        }

        // Rjson::GetStr(refpath, ROUTE_MSG_KEY_REFPATH, &doc);

        string curServInPath = _F("%d>", s_my_svrid);
        if (actpath.find(curServInPath) == string::npos)
        {
            actpath += curServInPath;
        }
        Rjson::SetObjMember(ROUTE_MSG_KEY_TRAIL, actpath, &doc);

        if (bto)
        {
            if (*bto > 0)
            {
                Rjson::SetObjMember(ROUTE_MSG_KEY_BEGORETO, *bto, &doc);
            }
            else
            {
                ntmp = 0;
                Rjson::GetInt(ntmp, ROUTE_MSG_KEY_BEGORETO, &doc); // 可选
                *bto = ntmp;
            }
        }

        ret = 0;
    }
    while(0);

    return ret;
}

int RouteExchage::TransMsg( void* ptr, unsigned cmdid, void* param )
{
	CMDID2FUNCALL_BEGIN
	Document doc;
    if (doc.Parse(body).HasParseError()) throw NormalExceptionOn(404, cmdid|CMDID_MID, seqid, "body json invalid @");

    int ret;
    try 
    {
        IFRETURN_N(!doc.HasMember(ROUTE_MSG_KEY_TO), 1); // 如果无to则直接本机处理
        int from = -1;
        if (!doc.HasMember(ROUTE_MSG_KEY_FROM))
        {
            from = iohand->getIntProperty(CONNTERID_KEY);
        }

        ret = TransMsg(doc, cmdid, seqid, from, 0, 0, iohand);
    }
    catch ( RouteExException& exp )
    {
        int from = 0;
        Rjson::GetInt(from, ROUTE_MSG_KEY_FROM, &doc);
        if (0 == exp.to && 0 == from)
        {
            LOGERROR("ROUTEEX_EXCPTION| reqcmd=0x%X reason=%s", exp.reqcmd, exp.reson.c_str());
            ret = 0;
        }
        else
        {
            exp.to = 0==exp.to? from: exp.to;
            Rjson::GetStr(exp.rpath, ROUTE_MSG_KEY_TRAIL, &doc);
            throw exp;
        }
    }

    return ret;
}

/**
 * 路由转发功能属性说明（约定）
 * to: 目的Serv编号
 * from: 发源的Serv编号
 * refer_path：参考的传输路径 no use（可以反向）, 格式 1>2>7>9>
 * act_path: 实际走过的路径，格式同上, 转发时仅修改此值
 */
int RouteExchage::TransMsg( Document& doc, unsigned cmdid, unsigned seqid, int fromSvr, int toSvr, int bto, IOHand* iohand )
{
    int ret;
    int& from = fromSvr;
    int to = toSvr;
    int belongTo = bto;

    RouteExException_IFTRUE_EASY(!doc.IsObject(), string("doc isnot jsonobject ")+Rjson::ToString(&doc));
    ret = AdjustRoutMsg(doc, fromSvr, &to, &belongTo);
    RouteExException_IFTRUE_EASY(ret, 
        _F("msg invalid %d-%d-%d ret%d ", from, toSvr, bto, ret)+Rjson::ToString(&doc));
    toSvr = to;
    
    string actpath;
    Rjson::GetStr(actpath, ROUTE_MSG_KEY_TRAIL, &doc);
    
    IOHand* ioh = NULL;
    for (char i=0, loop=true; i<2 && loop; ++i)
    {
        IFBREAK_N(s_my_svrid==to, 1); // continue to cmdfunc

        string searchKey = StrParse::Itoa(to) + "_";
        CliMgr::AliasCursor finder(searchKey);
        CliBase* cliptr = finder.pop();

        if (NULL == cliptr && belongTo > 0)
        {
            CliMgr::AliasCursor finder2(StrParse::Itoa(belongTo) + "_");
            cliptr = finder2.pop();
        }

        RouteExException_IFTRUE_EASY(NULL==cliptr, string("maybe path broken ")+Rjson::ToString(&doc));
        
        loop = false;
        // 查看是否属于直连的cli(isLocal=1)
        if (cliptr->isLocal())
        {
            ioh = dynamic_cast<IOHand*>(cliptr);
            RouteExException_IFTRUE_EASY(iohand==ioh, string("dead loop ")+Rjson::ToString(&doc));
        }
        // 发往外围的Serv的情况
        else if (1 == cliptr->getCliType())
        {
            OuterServ* oserv = dynamic_cast<OuterServ*>(cliptr);
            RouteExException_IFTRUE_EASY(NULL==oserv, 
                string("clitype=1 but not OuterServ class ")+cliptr->m_idProfile);
            ioh = oserv->getNearSendServ();
            RouteExException_IFTRUE_EASY(NULL==ioh, string("no valid path to ")+cliptr->m_idProfile);
            RouteExException_IFTRUE_EASY( // 走过的路径不能再走，免得死循环
                    actpath.find(string(">")+ioh->getProperty(CONNTERID_KEY)+">") != string::npos, 
                    string("dead loop ")+cliptr->m_idProfile);
        }
        else // 发往外围App的情况
        {
            OuterCli* ocli = dynamic_cast<OuterCli*>(cliptr);
            RouteExException_IFTRUE_EASY(NULL==ocli, 
                string("clitype!=1 but not OuterCli class ")+cliptr->m_idProfile);
            to = ocli->getBelongServ();
            RouteExException_IFTRUE_EASY(to==s_my_svrid || 1==i, 
                StrParse::Format("logic err%d OuterCli obj-%d catnot belongto self %s", 
                i, to, cliptr->m_idProfile.c_str()));
            loop = true; // 配合上面的for(),使得只可最多循环2次；
        }
    }

    if (ioh)
    {
        string msg = Rjson::ToString(&doc);
        ret = ioh->sendData(cmdid, seqid, msg.c_str(), msg.length(), true);
        LOGDEBUG("ROUTEXCHMSG| msg=exch [0x%X] -> %d -> %d| %s",
            cmdid, ioh->getIntProperty(CONNTERID_KEY), toSvr, msg.c_str());
    }

    return ret;
}

int RouteExchage::PostToCli( const string& jobj, unsigned cmdid, unsigned seqid, 
        int toSvr /*=0*/, int fromSvr /*=0*/, int bto /*=0*/ )
{
    Document doc;
    if (doc.Parse(jobj.c_str()).HasParseError()) return -111;
    
    return TransMsg(doc, cmdid, seqid, fromSvr, toSvr, bto, NULL);
}