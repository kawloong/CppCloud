#include "route_exchange.h"
#include "rapidjson/json.hpp"
#include "exception.h"
#include "comm/strparse.h"
#include "climanage.h"
#include "homacro.h"
#include "cloud/const.h"
#include "outer_serv.h"
#include "outer_cli.h"


HEPCLASS_IMPL_FUNC_BEG(RouteExchage)
HEPCLASS_IMPL_FUNC_MORE(RouteExchage, TransMsg)
HEPCLASS_IMPL_FUNC_END

int RouteExchage::s_my_svrid = 0;
#define RouteExException_IFTRUE_EASY(cond, resonstr) \
    RouteExException_IFTRUE(cond, cmdid, seqid, s_my_svrid, from, resonstr, actpath)

RouteExchage::RouteExchage( void )
{

}

void RouteExchage::Init( int my_svrid )
{
    s_my_svrid = my_svrid;
}

int setJsonObj( const string& key, const string& val, Document* node )
{
    node->RemoveMember(key.c_str());
    Value tmpkey(kStringType);
    Value tmpstr(kStringType);
    tmpkey.SetString(key.c_str(), node->GetAllocator()); 
    tmpstr.SetString(val.c_str(), node->GetAllocator()); 
    node->AddMember(tmpkey, tmpstr, node->GetAllocator());
    return 0;
}

int setJsonObj( const char* key, int val, Document* node )
{
    node->RemoveMember(key);
    node->AddMember(key, val, node->GetAllocator());
    return 0;
}

int RouteExchage::AjustRoutMsg( Document& doc, int fromSvr, int* toSvr, int* bto )
{
    int ret = 0;
    int ntmp = 0;

    do
    {
        if (Rjson::GetInt(ntmp, ROUTE_MSG_KEY_TO, &doc))
        { // 无
            IFBREAK_N(NULL==toSvr, -101);
            setJsonObj(ROUTE_MSG_KEY_TO, *toSvr, &doc);
        }
        else
        {
            if (toSvr) *toSvr = ntmp;
        }

        if (Rjson::GetInt(ntmp, ROUTE_MSG_KEY_FROM, &doc))
        {
            if (0 == fromSvr) fromSvr = s_my_svrid;
            setJsonObj(ROUTE_MSG_KEY_FROM, fromSvr, &doc);
        }
        else
        {
            fromSvr = ntmp;
        }

        // Rjson::GetStr(refpath, ROUTE_MSG_KEY_REFPATH, &doc);
        Rjson::GetStr(actpath, ROUTE_MSG_KEY_TRAIL, &doc);
        string fromInPath = _F("%d>", fromSvr);
        string curServInPath = _F("%d>", s_my_svrid);
        if (actpath.find(fromInPath) == string::npos) // 可能会来自app作源的包
        {
            actpath = fromInPath + actpath;
        }
        if (actpath.find(curServInPath) == string::npos)
        {
            actpath += curServInPath;
        }
        setJsonObj(ROUTE_MSG_KEY_TRAIL, actpath, &doc);

        if (bto)
        {
            if (*bto > 0)
            {
                setJsonObj(ROUTE_MSG_KEY_BEGORETO, *bto, &doc);
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

    RouteExException_IFTRUE_EASY(!doc.IsObject(), string("doc isnot jsonobject ")+Rjson::ToString(&doc));
    return TransMsg(doc, cmdid, seqid, 0, 0, 0);
}

/**
 * 路由转发功能属性说明（约定）
 * to: 目的Serv编号
 * from: 发源的Serv编号
 * refer_path：参考的传输路径 no use（可以反向）, 格式 1>2>7>9>
 * act_path: 实际走过的路径，格式同上, 转发时仅修改此值
 */
int RouteExchage::TransMsg( Document& doc, unsigned cmdid, unsigned seqid, int fromSvr, int toSvr, int bto )
{
    int ret;
    int to = toSvr;
    int belongTo = bto;

    ret = AjustRoutMsg(doc, fromSvr, &to, &belongTo);
    RouteExException_IFTRUE_EASY(ret, 
        _F("msg invalid %d-%d-%d ", from, toSvr, bto)+Rjson::ToString(&doc));
    
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
        actpath += StrParse::Format("%d>", s_my_svrid);
        setJsonObj(ROUTE_MSG_KEY_TRAIL, actpath, &doc);

        string msg = Rjson::ToString(&doc);
        ret = ioh->sendData(cmdid, seqid, msg.c_str(), msg.length(), true);
    }

    return ret;
}

int RouteExchage::postToCli( const string& jobj, unsigned cmdid, unsigned seqid, int toSvr, int fromSvr, int bto )
{
    Document doc;
    if (doc.Parse(jobj.c_str()).HasParseError()) return -111;
    
    return TransMsg(doc, cmdid, seqid, fromSvr, toSvr, bto);
}