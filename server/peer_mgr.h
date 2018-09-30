/*-------------------------------------------------------------------------
FileName     : peer_mgr.h
Description  : 管理PeerServ对象
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-09-12       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _PEER_MGR_H_
#define _PEER_MGR_H_
#include "comm/hep_base.h"
#include "comm/public.h"
#include <map>

using namespace std;
class PeerServ;


class PeerMgr: public HEpBase
{
public:
	HEPCLASS_DECL(PeerMgr, PeerMgr);
	SINGLETON_CLASS2(PeerMgr)
	PeerMgr();

public:
	int init( int epfd );
	void uninit( void );

	// interface HEpBase
	virtual int onEvent( int evtype, va_list ap );

private:
	int m_mysvrid;
	int m_epfd;
	map<PeerServ*, int> m_rSvrs; // 此为主, CliMgr次
};

#endif
