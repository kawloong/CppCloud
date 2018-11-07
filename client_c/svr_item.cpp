#include "svr_item.h"
#include "comm/strparse.h"

// 把url里的主机和端口解析出来
bool svr_item_t::parseUrl( void )
{
    // url example: http://192.168.1.12:8000/path 
    string tmp;
    int ret = StrParse::SplitURL(tmp, host, port, tmp, tmp, url);
    return 0 == ret;
}
