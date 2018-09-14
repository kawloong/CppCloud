#ifndef _PUBLIC_H_
#define _PUBLIC_H_
#include <stdlib.h>
#include <stdio.h>

// #include <map>
// #include <string>
// using std::map;
// using std::string;


#ifdef LOG2STDOUT
#include <unistd.h>
#define LOGDEBUG(fmt, ...) printf("-- Debug[%d/%x][%s:%d]:"  fmt "\n", getpid(), (unsigned)pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGINFO(fmt, ...)  printf("-- Info[%d/%x][%s:%d]:"   fmt "\n", getpid(), (unsigned)pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGWARN(fmt, ...)  printf("-- Warn[%d/%x][%s:%d]:"   fmt "\n", getpid(), (unsigned)pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGERROR(fmt, ...)  printf("-- Error[%d/%x][%s:%d]:" fmt "\n", getpid(), (unsigned)pthread_self(),__FILE__, __LINE__, ##__VA_ARGS__)
#define LOGFATAL(fmt, ...)  printf("-- Fatal[%x][%s:%d]:" fmt "\n", (unsigned)pthread_self(),__FILE__, __LINE__, ##__VA_ARGS__)
//#define SVC_LOG(...) printf("miss trace\n")
#else ////////////////////////////////////////////////////////////////////////
#include "log.h"
#define LOGDEBUG(fmt, ...) log_debug(fmt , ##__VA_ARGS__)
#define LOGINFO(fmt, ...)  log_info(fmt , ##__VA_ARGS__)
#define LOGWARN(fmt, ...)  log_warn(fmt , ##__VA_ARGS__)
#define LOGERROR(fmt, ...) log_error(fmt , ##__VA_ARGS__)
#define LOGFATAL(fmt, ...) log_fatal(fmt , ##__VA_ARGS__)
#endif

#define LOGOPT_WD(exp, fmt, ...) {if(exp)LOGWARN(fmt , ##__VA_ARGS__);else LOGDEBUG(fmt , ##__VA_ARGS__);}
#define LOGOPT_EI(exp, fmt, ...) {if(exp)LOGERROR(fmt , ##__VA_ARGS__);else LOGINFO(fmt , ##__VA_ARGS__);}

#define assert_static(e) do{ enum{ assert_static_ = 1/(e) }; } while(0)
#define IFBREAK(exp) if((exp)){ break;}
#define IFRETURN(exp) if(exp){ return; }
#define IFBREAK_N(exp, n) if((exp)){ret=n; break;}
#define IFRETURN_N(exp, n) if(exp){ return n; }
#define INFOLOG_IF0(ret, fmt, ...)  if(!(ret)){ LOGINFO(fmt, ##__VA_ARGS__); }
#define ERRLOG_IF1RET_N(exp, n, format, ...)  if((exp)){ LOGERROR(format, ##__VA_ARGS__); return n; }
#define ERRLOG_IF1RET(exp, format, ...)  if((exp)){ LOGERROR(format, ##__VA_ARGS__); return; }
#define ERRLOG_IF0(ret, fmt, ...)  if(!(ret)){ LOGERROR(fmt, ##__VA_ARGS__); }
#define ERRLOG_IF0BRK(exp, n, format, ...)  if(!(exp)){ LOGERROR(format, ##__VA_ARGS__); ret=n; break; }
#define ERRLOG_IF1(ret, fmt, ...)  if(ret){ LOGERROR(fmt, ##__VA_ARGS__); }
#define ERRLOG_IF1BRK(exp, n, format, ...)  if(exp){ LOGERROR(format, ##__VA_ARGS__); ret=n; break; }
#define WARNLOG_IF1(ret, format, ...)  if(ret){ LOGWARN(format, ##__VA_ARGS__); }
#define WARNLOG_IF1BRK(exp, n, format, ...)  if(exp){ LOGWARN(format, ##__VA_ARGS__); ret=n; break; }
#define EASSERT ERRLOG_IF0
#define IFFREE(ptr) if(ptr){free(ptr); ptr=NULL;}
#define IFFREE_C(ptr) if(ptr){free((void*)ptr);}
#define IFDELETE(ptr) if(ptr){delete ptr; ptr=NULL;}
#define IFDELETE_ARR(pa) if(pa){delete [] pa; pa=NULL;}
#define INVALID_FD (-1)
#define IFCLOSEFD(fd) if(-1!=fd){close(fd); fd=-1;}
#define ISEMPTY_PCHAR(pchar) (NULL==pchar || 0==pchar[0]) // �ж�char*�����Ƿ�Ϊ��

#define PARTNER_LIST "anl_partner_list"
#define DEVICE_LIST "device_list"


// simple define singleton class
#define SINGLETON_CLASS(Cls)                                  \
public:                                                       \
static Cls* Instance(void){ static Cls s; return &s; }        \
private:                                                      \
Cls(){}                                                       \
~Cls(){}                                                      \
Cls(const Cls&);                                              \
Cls& operator=(const Cls&);    

// singleton, need define constructor&destructor
#define SINGLETON_CLASS2(Cls)                                 \
public:                                                       \
static Cls* Instance(void){ static Cls s; return &s; }        \
private:                                                      \
Cls(const Cls&);                                              \
Cls& operator=(const Cls&);    



#endif
