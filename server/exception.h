/******************************************************************* 
 *  summery:        业务异常类
 *  author:         hejl
 *  date:           2018-09-07
 *  description:
 ******************************************************************/
#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_
#include <string>

using std::string;

// 仅跳出当前业务流程,不关闭连接
struct NormalExceptionOn
{
    int code;
    unsigned cmdid;
    unsigned seqid;
    string desc;

    NormalExceptionOn(int co, unsigned cmd, unsigned seq, const string& ds): 
        code(co), cmdid(cmd), seqid(seq), desc(ds){}
    
};

// 等待发送完成后关闭
struct NormalExceptionOff
{
    int code;
    unsigned cmdid;
    unsigned seqid;
    string desc;

    NormalExceptionOff(int co, unsigned cmd, unsigned seq, const string& ds): 
        code(co), cmdid(cmd), seqid(seq), desc(ds){}
    
};

// 强制即时关闭连接
struct OffConnException
{
    string reson;

    OffConnException(const string reson_param): reson(reson_param){}
};

#define NormalExceptionOn_IFTRUE(cond, co, cmd, seq, ds) \
    if (cond){ throw NormalExceptionOn(co, cmd, seq, ds); }

#define NormalExceptionOff_IFTRUE(cond, co, cmd, seq, ds) \
    if (cond){ throw NormalExceptionOff(co, cmd, seq, ds); }


#endif