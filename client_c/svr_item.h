/*-------------------------------------------------------------------------
FileName     : svr_item.h
Description  : 一项服务提供者信息
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-07       create     hejl 
-------------------------------------------------------------------------*/

#ifndef _SVR_ITEM_H_
#define _SVR_ITEM_H_
#include <string>

using std::string;

struct svr_item_t
{
    string url;
    string version;
    string host;
    
    int port;
    int svrid;
    short protocol; // tcp=1 udp=2 http=3 https=4
    short weight;

    svr_item_t(): port(0), svrid(0), protocol(0), weight(0) {}
    bool parseUrl( void );
};

#endif