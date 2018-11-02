/*-------------------------------------------------------------------------
FileName     : svrprop.h
Description  : 服务提供者自身属性描述类
remark       : 
Modification :
--------------------------------------------------------------------------
   1、Date  2018-11-02       create     hejl 
-------------------------------------------------------------------------*/
#ifndef _SVRPROP_H_
#define _SVRPROP_H_
#include <string>

using std::string;

struct SvrProp
{
	string regname;
	string url;
	string desc;
	int svrid;
	int okcount; // 成功调用次数
	int ngcount; // 失败调用次数
	int tmpnum;
	short protocol; // tcp=1 udp=2 http=3 https=4
	short version;
	short weight; // 服务权重
	short idc;
	short rack;
	bool islocal;
	bool enable;

	SvrProp( void );
	bool valid( void ) const;

	string jsonStr( void ) const;
};


#endif