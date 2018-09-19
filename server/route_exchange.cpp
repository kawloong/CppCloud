#include "route_exchange.h"


HEPCLASS_IMPL_FUNC_BEG(RouteExchage)
HEPCLASS_IMPL_FUNC_MORE(RouteExchage, on_CMD_ERAALL_REQ)
HEPCLASS_IMPL_FUNC_END

int RouteExchage::s_my_svrid = 0;

RouteExchage::RouteExchage( void )
{

}

int RouteExchage::on_CMD_ERAALL_REQ( void* ptr, unsigned cmdid, void* param )
{
	CMDID2FUNCALL_BEGIN
	Document doc;

    if (doc.Parse(body).HasParseError()) 
    {
        throw NormalExceptionOn(404, cmdid|CMDID_MID, seqid, "body json invalid");
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
     * refer_path：参考的传输路径（可以反向）, 格式 1>2>7>9>
     * act_path: 实际走过的路径，格式同上
     */

    ret |= Rjson::GetInt(to, "to", &doc);
    ret |= Rjson::GetInt(from, "from", &doc);
    ret |= Rjson::GetStr(refpath, "refer_path", &doc);
    ret |= Rjson::GetStr(actpath, "act_path", &doc);
    NormalExceptionOn_IFTRUE(ret, 406, cmdid|CMDID_MID, seqid, 
        string("leak of param ")+Rjson::ToString(&doc));
    
    do
    {
        IFBREAK_N(s_my_svrid==to, 1); // continue to cmdfunc

        // 查看是否属于直连的cli(isLocal=1)
        string searchKey = StrParse::Itoa(to) + "_";
        CliMgr::AliasCursor finder(searchKey);
        CliBase* cliptr = finder.pop();

        NormalExceptionOn_IFTRUE(NULL == cliptr, 407, cmdid|CMDID_MID, seqid, 
            string("maybe path broken ")+Rjson::ToString(&doc));
        if (cliptr->isLocal())
        {
            IOHand* ioh = dynamic_cast<IOHand*>(cliptr);
            actpath += StrParse::Format("%d>", s_my_svrid);
            doc.setStr("act_path", actpath + "");

            string msg = Rjson::ToString(&doc);
            ioh->sendData(cmdid, seqid, msg.c_str(), msg.length(), true);
            ret = 0;
            break;
        }
    }
    while(0);

    return ret;
}