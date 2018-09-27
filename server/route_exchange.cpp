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

int RouteExchage::TransMsg( void* ptr, unsigned cmdid, void* param )
{
	CMDID2FUNCALL_BEGIN
	Document doc;

    if (doc.Parse(body).HasParseError()) 
    {
        throw NormalExceptionOn(404, cmdid|CMDID_MID, seqid, "body json invalid @");
    }

    int ret = 0;

    int to = 0;
    int bto = 0; // 所属的Servid, 当to是OuterCli时，可借助bto得知其直属serv
    int from = 0;
    string refpath;
    string actpath;

    /**
     * 路由转发功能属性说明（约定）
     * to: 目的Serv编号
     * from: 发源的Serv编号
     * refer_path：参考的传输路径 no use（可以反向）, 格式 1>2>7>9>
     * act_path: 实际走过的路径，格式同上, 转发时仅修改此值
     */

    ret |= Rjson::GetInt(to, "to", &doc);
    ret |= Rjson::GetInt(from, "from", &doc);
    ret |= Rjson::GetStr(refpath, "refer_path", &doc);
    ret |= Rjson::GetStr(actpath, ROUTE_PATH, &doc);
    Rjson::GetInt(bto, "bto", &doc); // 可选

    RouteExException_IFTRUE_EASY(ret, string("leak of param ")+Rjson::ToString(&doc));
    
    IOHand* ioh = NULL;
    for (char i=0, loop=true; i<2 && loop; ++i)
    {
        IFBREAK_N(s_my_svrid==to, 1); // continue to cmdfunc

        string searchKey = StrParse::Itoa(to) + "_";
        CliMgr::AliasCursor finder(searchKey);
        CliBase* cliptr = finder.pop();

        if (NULL == cliptr && bto > 0)
        {
            CliMgr::AliasCursor finder2(StrParse::Itoa(bto) + "_");
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
        setJsonObj(ROUTE_PATH, actpath, &doc);

        string msg = Rjson::ToString(&doc);
        ret = ioh->sendData(cmdid, seqid, msg.c_str(), msg.length(), true);
    }

    return ret;
}