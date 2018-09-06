/*-------------------------------------------------------------------------
FileName     : default_hand.h
Description  : 默认消息处理类
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-01-31       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _DEFAULT_HAND_H_
#define _DEFAULT_HAND_H_
#include "comm/hep_base.h"



class DeftHand: public HEpBase
{

public:
    HEPCLASS_DECL(DeftHand, DeftHand)
    DeftHand(void);
    virtual ~DeftHand(void);


public: // interface IEPollRun
    virtual int run( int flag, long p2 );
	virtual int onEvent( int evtype, va_list ap );

protected:

protected:


};

#endif