/*-------------------------------------------------------------------------
FileName     : remote_mgr.h
Description  : 管理RemoteServ对象
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-12       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _REMOTE_MGR_H_
#define _REMOTE_MGR_H_
#include "comm/hep_base.h"
#include "comm/public.h"
#include <map>

using namespace std;
class RemoteServ;


class RemoteMgr: public HEpBase
{
public:
	HEPCLASS_DECL(RemoteMgr, RemoteMgr);
	SINGLETON_CLASS2(RemoteMgr)
	RemoteMgr();

public:
	int init( int epfd );

	// interface HEpBase
	virtual int onEvent( int evtype, va_list ap );

private:
	int m_mysvrid;
	int m_epfd;
	map<RemoteServ*, int> m_rSvrs; // 此为主, CliMgr次
};

#endif
