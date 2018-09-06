/*-------------------------------------------------------------------------
fileName     : version.h
Description  : 版本号头文件
Functions    : 
Modification :
-------------------------------------------------------------------------*/
#ifndef _VERSION_H__
#define _VERSION_H__
#include <iostream>

using std::cout;
using std::endl;

//输出版本信息
void version_output(const char* name, const char* ver)
{
    //输出版本信息
    cout << "************************************************" << endl;
    cout << "    program : " << name << endl;
    cout << "    version : " << ver << endl;
	cout << "    Build   : "<< __DATE__ " " __TIME__ << endl;		
	cout << "************************************************" << endl;	
}

#endif //_COMMON_VERSION_H__

