#include "outer_serv.h"
#include "comm/strparse.h"


HEPMUTICLASS_IMPL(OuterServ, OuterServ, CliBase)

OuterServ::OuterServ()
{
    m_cliType = 1;
    m_isLocal = false;
    m_svrid = 0;
}

void OuterServ::init( int svrid )
{
    m_svrid = svrid;
}

/**
 * @summery: 设置外围Serv的路由
 * @remark: rp参数格式: 1>4>6>8>
 **/
int OuterServ::setRoutePath( const string& strpath )
{
    int ret = 0;
    RoutePath routpath;
    
    ret = StrParse::SpliteInt(routpath.path, strpath, '>');
    unsigned pathn = routpath.path.size();

    if (0 == ret && pathn > 0)
    {
        ERRLOG_IF1RET_N(m_svrid!=routpath.path[0], -37, 
            "SETROUTPATH| msg=invalid path| path=%s| servid=%d", strpath.c_str(), m_svrid);
        
        routpath.next_svrid = *routpath.path.rbegin();
        routpath.mtime = time(NULL);
        string key = StrParse::Format("P%d_%s", pathn, strpath.c_str());

        m_routpath[key] = routpath;
    }

    return ret;
}

