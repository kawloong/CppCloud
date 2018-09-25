#include "route_exchange.h"
#include "rapidjson/json.hpp"
#include "exception.h"
#include "comm/strparse.h"
#include "climanage.h"
#include "homacro.h"
#include "cloud/const.h"
#include "outer_serv.h"


HEPCLASS_IMPL_FUNC_BEG(RouteExchage)
HEPCLASS_IMPL_FUNC_MORE(RouteExchage, TransMsg)
HEPCLASS_IMPL_FUNC_END

int RouteExchage::s_my_svrid = 0;
#define RouteExException_IFTRUE_EASY(cond, resonstr) \
    RouteExException_IFTRUE(cond, cmdid, seqid, s_my_svrid, from, resonstr, actpath)

RouteExchage::RouteExchage( void )
{

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
    actpath += StrParse::Format("%d>", s_my_svrid);

    RouteExException_IFTRUE_EASY(ret, string("leak of param ")+Rjson::ToString(&doc));
    
    do
    {
        IFBREAK_N(s_my_svrid==to, 1); // continue to cmdfunc

        string searchKey = StrParse::Itoa(to) + "_";
        CliMgr::AliasCursor finder(searchKey);
        CliBase* cliptr = finder.pop();

        RouteExException_IFTRUE_EASY(NULL==cliptr, string("maybe path broken ")+Rjson::ToString(&doc));
        
        IOHand* ioh = NULL;
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
                string("clitype=1 not OuterServ class ")+cliptr->m_idProfile);
            ioh = oserv->getNearSendServ();
            RouteExException_IFTRUE_EASY(NULL==ioh, string("no valid path to ")+cliptr->m_idProfile);
        }
        else // 发往外围App的情况
        {
            
        }

        setJsonObj("act_path", actpath, &doc);

        string msg = Rjson::ToString(&doc);
        ioh->sendData(cmdid, seqid, msg.c_str(), msg.length(), true);
        ret = 0;
    }
    while(0);

    return ret;
}