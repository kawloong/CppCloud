/*-------------------------------------------------------------------------
FileName     : outer_cli.h
Description  : 客户对象保活机制
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
    HEPCLASS_DECL(OuterCli, OuterCli);
    void init( void );

protected: // interface IEPollRun
	virtual int onEvent( int evtype, va_list ap );
    virtual int qrun( int flag, long p2 );

protected:

};

#endif
