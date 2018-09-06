/******************************************************************* 
 *  summery:        linux多进程自管理实现
 *  author:         hejl
 *  date:           2016-09-09
 *  description:    master进程依赖linux信号机制,捕获SIGCHLD
 ******************************************************************/

#ifndef _MUTIPROCESS_H_
#define _MUTIPROCESS_H_
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>

/* ----------------  usage  ---------------
 *      ret = MutiProcess::Setup(3, true);
 *      if (ret > 0) // master process exit
 *        exit(0);
 *      else if (ret < 0) // error happen
 *        exit(-1);
 *      // sub-process run here 
 *      // ... 
-------------------------------------------*/


class MutiProcess
{
	static sig_atomic_t s_process_max;
	static sig_atomic_t s_process_cur;
	static sig_atomic_t s_run_state; // 参考enum RunState

	static const int process_max_limit = 512;

    enum RunState
    {
        FORCEEND, // 即将退出状态
        WAITSUB,  // 等待子进程结束
        ONETIME,  // 子进程不可重启
        RESPAWN,  // 子进程可重启状态
    };

public:
	/* @summery: 创建多进程程序，调用者作为父进程
	 * @param: count 子进程数量
	 * @param: respawn 是否在子进程结束后自动重启 
	 * @return: 0 子进程成功返回；>0 父进程要即出时返回; <0 出错
	 */
	static int Setup(int count, bool respawn)
	{
		int ret = 0;
		s_process_max = (count>process_max_limit)? process_max_limit: count;
        s_run_state = respawn? RESPAWN: ONETIME;
		
        ret = master_run();
		return ret;
	}

    static int GetPIndex(void) { return s_process_cur; }

private:

    // 创建子进程
    // @return: 子进程返回0,父进程返回正数,出错返回负数
	static int mkprocess(void)
	{
		int ret = 1;
		for (; s_process_cur < s_process_max; )
        {
			pid_t pid = fork();
            if (pid > 0) // parent
            {
				s_process_cur++;
				ret = 1;
            }
            else if (0 == pid) // child
            {
				ret = 0;
                break;
            }
            else
            {
				perror("fork fail");
				ret = -1;
                break;
            }
        }
		
		return ret;
	}

    /* @summery: 主控进程会停留在函数内部,
        直到全部进程要退出才返回大于0值; 子进程会跳出返回0
    */
	static int master_run(void)
	{
		int ret;
        bool first = true;
	 
        sigset_t sigset;
        sigset_t sigold;
        struct sigaction sa;
        struct sigaction sa_int;
        struct sigaction sa_term;

		
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset(&sa.sa_mask);
		sa.sa_handler = sighandle;
		sa.sa_flags = 0;
		sigaction(SIGCHLD, &sa, NULL);
        sigaction(SIGINT, &sa, &sa_int);
        sigaction(SIGTERM, &sa, &sa_term);

		sigemptyset(&sigset);
		sigaddset(&sigset, SIGCHLD);
        sigaddset(&sigset, SIGINT);
        sigaddset(&sigset, SIGTERM);

		if (sigprocmask(SIG_BLOCK, &sigset, &sigold) == -1)
		{
		   perror("master set sigprocmask");
		}

		sigemptyset(&sigset);
		for ( ;; )
        {
            if (FORCEEND == s_run_state) // 父进程要退出
            {
                ret = 3;
                break;
            }

			if (RESPAWN == s_run_state || first)
			{
				ret = mkprocess();
                first = false;
				if (ret <= 0)
                {
                    // 恢复信号MASK给子进程
                    sigaction(SIGINT, &sa_int, NULL);
                    sigaction(SIGTERM, &sa_term, NULL);
                    sigprocmask(SIG_SETMASK, &sigold, NULL);
                    break;
                }
			}
            else if (s_process_cur <= 0)
            {
                ret = 2; // 子进程全部退出了，父进程也退出
                break;
            }
			
			sigsuspend(&sigset);
		}
		
		return ret;
	}

    // 信号处理回调
	static void sighandle(int signo)
	{
		switch (signo)
		{
			case SIGTERM:
			case SIGINT:
			//case SIGQUIT:
                // 第1次TERM时先关闭重启标识和等待子进程全部退出；
                // 第2次TERM时设置主进程退出标识; 
                // (如果子进程快速退出,则无需2次TERM主进程会顺利退出)
                if (RESPAWN == s_run_state || ONETIME == s_run_state)
                {
                    s_run_state = WAITSUB;
                }
                else if (WAITSUB == s_run_state)
                {
                    s_run_state = FORCEEND;
                }
                printf("s_run_state-- %d\n", s_run_state);

			break;

			case SIGCHLD:
            {
                printf("SIGCHLD-- %d\n", s_run_state);
				chld_handle();
			}
			break;
			
			default:
				break;
		}

	}
	
    // 信号(子进程结束)处理回调
	static void chld_handle(void)
	{
		int status;
		pid_t pid;
		
		for ( ;; )
		{
			pid = waitpid(-1, &status, WNOHANG);

			if (pid == 0)return;

			if (pid == -1)
			{
				if (errno == EINTR)continue;
				return;
			}

			//WEXITSTATUS(status); // exit code
			s_process_cur--;
        }

    }
	
};


sig_atomic_t MutiProcess::s_process_max = 0;
sig_atomic_t MutiProcess::s_process_cur = 0;
sig_atomic_t MutiProcess::s_run_state = 1;
 
 #endif
  