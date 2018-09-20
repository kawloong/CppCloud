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

// 路由消息转发异常
struct RouteExException
{
    unsigned reqcmd; // 注意reqcmd不同于上面的异常结构的cmdid,此处为避免无止传播异常
    unsigned seqid;
    unsigned from;
    unsigned to;
    string reson;
    string rpath;

    RouteExException(unsigned req_cmd, unsigned seq, unsigned fmid, unsigned toid, 
        const string& ds, const string& path): 
        reqcmd(cmd), seqid(seq), from(fmid), to(toid), reson(ds), rpath(path){}
};

#define NormalExceptionOn_IFTRUE(cond, co, cmd, seq, ds) \
    if (cond){ throw NormalExceptionOn(co, cmd, seq, ds); }

#define NormalExceptionOff_IFTRUE(cond, co, cmd, seq, ds) \
    if (cond){ throw NormalExceptionOff(co, cmd, seq, ds); }

#define RouteExException_IFTRUE(cond, reqcmd, seq, from, to, ds, path) \
    if (cond){ throw RouteExException(reqcmd, seq, from, to, ds, path); }

#endif