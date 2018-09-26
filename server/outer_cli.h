/*-------------------------------------------------------------------------
FileName     : outer_cli.h
Description  : 间隔过来的客户对象
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-13       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _OUTER_CLI_H_
#define _OUTER_CLI_H_
#include "clibase.h"


class OuterCli: public CliBase
{
public:
    HEPCLASS_DECL(OuterCli, OuterCli);

    OuterCli(void);
    void init( int inServ );

    int getBelongServ(void) { return m_inServ; }

protected: // interface IEPollRun
	//virtual int onEvent( int evtype, va_list ap );
    //virtual int qrun( int flag, long p2 );

protected:
    int m_inServ; // 直接连接到哪个Serv
};

#endif
