/*-------------------------------------------------------------------------
FileName     : outer_serv.h
Description  : 客户对象保活机制
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-13       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _OUTER_SERV_H_
#define _OUTER_SERV_H_
#include <map>
#include <vector>
#include <string>
#include "clibase.h"

using namespace std;
class IOHand;

class OuterServ: public CliBase
{
public:
    struct RoutePath
    {
        int next_svrid;
        int mtime;
        vector<int> path;

        RoutePath(void):next_svrid(0),mtime(0){}
    };

    HEPCLASS_DECL(OuterServ, OuterServ);
    OuterServ();
    
    void init( int svrid );
    int setRoutePath( const string& rp );
    IOHand* getNearSendServ( void );

protected: // interface IEPollRun
	//virtual int onEvent( int evtype, va_list ap );
    //virtual int qrun( int flag, long p2 );

protected:
    map<string, RoutePath> m_routpath;
    int m_svrid;
};

#endif
