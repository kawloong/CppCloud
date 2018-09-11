/*-------------------------------------------------------------------------
FileName     : remote_cli.h
Description  : 远中心端的客户数据管理
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-11       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _REMOTE_CLI_H_
#define _REMOTE_CLI_H_
#include "comm/hep_base.h"


class IOHand;

class RemoteCli: public HEpBase
{
public:
    HEPCLASS_DECL(RemoteCli, RemoteCli)
    RemoteCli(void);
    virtual ~RemoteCli(void);


protected: // interface IEPollRun
	virtual int onEvent( int evtype, va_list ap );

protected:
    IOHand* m_iohand;
};

#endif
