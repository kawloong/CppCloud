/******************************************************************* 
 *  summery:     CppCloud核心服务入口
 *  author:      hejl
 *  date:        2018-01-30
 *  description: 
 ******************************************************************/
#include <csignal>
#include <cerrno>
#include <cstring>

#include "comm/version.h"
#include "comm/daemon.h"
#include "comm/file.h"
#include "comm/log.h"
#include "comm/strparse.h"
#include "comm/public.h"
#include "cppcloud_config.h"
#include "flowctrl.h"
 

bool g_bExit = false;
string g_str_pwdpath;
string g_str_exename;

struct ParamConf
{
    string inifile; // 配置文件
    string logfile; // 日志文件全路径
    string workpath; // 进程工作目录
    int loglevel;
    int logfsize;
    int msg_id;

    bool debug;
    bool daemon;
}g_param; // 保存命令行相关参数

enum _ret_main_
{
    RET_NORMAL   = 0,
    RET_SIGNAL   = 1,  // 信号注册
    RET_CONFIG   = 2,  // conf初始化失败
    RET_LOG      = 3,  // 日志服务初始化失败
    RET_COMMOND  = 4,  // 进程管理命令返回(start,stop,status)
    RET_SETTING  = 5,

    RET_SERVICE  = 10,  // 业务初始化失败
};

// 程序配置文件，日志文件，名称，版本号
#define DEF_CONFILE "monitor.conf"
#define LOGFILE_NAME "cppcloud_serv.log"
#define PROGRAM_NAME "cppcloud_serv"
#define PROGRAM_VER "1.0.0"
#define PROGRAM_DESC "Build: " __DATE__ " " __TIME__

static void sigdeal(int signo)
{
    printf("signal happen %d, thread is %x\n", signo, (int)pthread_self());
    g_bExit = true;
    FlowCtrl::Instance()->notifyExit(); /// #PROG_EXITFLOW(1)
}

static int sigregister(void)
{
    int ret = 0;
    struct sigaction sa;

#define Sigaction(signo) if (sigaction(signo, &sa, NULL) == -1){ perror("signal req fail:"); ret=errno; }

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = sigdeal;
//    Sigaction(SIGUSR1);
    Sigaction(SIGINT);
    Sigaction(SIGTERM);

    sa.sa_handler = SIG_IGN;
    Sigaction(SIGALRM);
    Sigaction(SIGPIPE);
    Sigaction(SIGHUP);
    Sigaction(SIGURG);

    return ret;
}

static int loginit( void )
{
    int result;
    const int mb_size_byte = 1024*1024;

    g_param.logfile = BmshConf::CppCloudLogPath(true);
    g_param.loglevel = BmshConf::CppCloudLogLevel(true);
    g_param.logfsize = BmshConf::CppCloudLogFSize(true);
    StrParse::AdjustPath(g_param.logfile, true);

    if (!File::CreatDir_r(g_param.logfile.c_str()))
    {
        perror("creatdir_r for log path");
        fprintf(stderr, "mkdir %s fail\n", g_param.logfile.c_str());
        return -1;
    }

    g_param.logfile += LOGFILE_NAME;
    result = log_open(g_param.logfile.c_str(), g_param.loglevel, true, g_param.logfsize*mb_size_byte);
    LOGINFO("LOGINIT| result=%d| file=%s", result, g_param.logfile.c_str());
    return result;
}

// 获取命令行参数
static int parse_cmdline(int argc, char** argv)
{
    int ret = 0;
    int c;
    int debug = 0;
    int daemon = 0;
    string conffile(DEF_CONFILE);

    while ((c = getopt(argc, argv, "c:w:W:q:vdD")) != -1)
    {
        switch (c)
        {
        case 'c' :
            conffile = optarg;
            break;
        case 'd' :
            daemon = 1;
            break;
        case 'D':
            debug = 1;
            break;
            
        case 'v':
            version_output(PROGRAM_NAME, PROGRAM_VER);
            exit(0);
            break;

        case 'h':
        default :
            printf("command parameter fail! \n"
                "\t-c <f.conf> configure file name\n"
                //"\t-p <port> listen port\n"
                "\t-v show program version\n"
                "\t-d  run as a deamon\n");
            ret = RET_COMMOND;
            break;
        }
    }

    g_param.inifile = conffile;
    g_param.debug = debug;
    g_param.daemon = daemon;

    return ret;
}

int main(int argc, char* argv[])
{
    int mret;
#define PROGRAM_EXIT(retcode) mret = retcode; goto mainend;
#define CHK_EXIT(r) if(r){goto mainend;}

    // 解析命令行参数
    if ( (mret = parse_cmdline(argc, argv)) )
    {
        PROGRAM_EXIT(mret);
    }

    // 日志例程初始化
    if (BmshConf::Init(g_param.inifile.c_str()))
    {
        PROGRAM_EXIT(RET_CONFIG);
    }

    if (loginit())
    {
        PROGRAM_EXIT(RET_LOG);
    }

    // 信号处理(多线程下,全部线程共用相同处理函数,并且只有1个线程获得信号处理)
    if (sigregister())
    {
        PROGRAM_EXIT(RET_SIGNAL);
    }

    /************* 主体服务执行 ***************/
    // 1. 数据服务相关初始化
    // skip


    // 2. 工作业务启动
    {
        string lisnCls = BmshConf::CppCloudListenClass();
        int port = BmshConf::CppCloudListenPort();
        int taskQnum = BmshConf::CppCloudTaskQNum();
        if (FlowCtrl::Instance()->init(taskQnum))
        {
            PROGRAM_EXIT(RET_SERVICE);
        }
        if (FlowCtrl::Instance()->addListen(lisnCls.c_str(), port))
        {
            PROGRAM_EXIT(RET_SERVICE);
        }

        mret = FlowCtrl::Instance()->run(g_bExit);

        FlowCtrl::Instance()->uninit();
    }


mainend:
    LOGINFO("PROGRAM_END| msg=%s exit(%d)", PROGRAM_NAME, mret);
    if (mret)
    {
        fprintf(stderr, "%s exit(%d)\n", PROGRAM_NAME, mret);
    }
    
    return mret;
}