/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef __LOG_H__
#define __LOG_H__
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef unsigned long long uint64;
class Logger
{
	public:
		static const int LEVEL_NONE		= (-1);
        static const int LEVEL_MIN		= 0;
        static const int LEVEL_TRACE	= 0;
        static const int LEVEL_DEBUG	= 1;
        static const int LEVEL_INFO		= 2;
        static const int LEVEL_WARN		= 3;
        static const int LEVEL_ERROR	= 4;
		static const int LEVEL_FATAL	= 5;
        static const int LEVEL_MAX		= 5;

        static const int LOGPATH_MAX = 1024;

		static int get_level(const char *levelname);
	private:
		FILE *fp;
        char filename[LOGPATH_MAX];
        short fileseq; // 每日日志文件计数
        time_t logdate; // 当前日志所属日期（用于进入新的一日创建新文件）
		int level_;
        pthread_mutex_t *mutex;
        pthread_mutex_t *gzMutex;

		uint64 rotate_size;
		struct{
			uint64 w_curr;
			uint64 w_total;
		}stats;

		void rotate();
		void threadsafe();
        FILE* fopen_ex(const char* filename);
	public:
		Logger();
		~Logger();

		int level(){
			return level_;
		}

		void set_level(int level){
			this->level_ = level;
		}

		int open(FILE *fp, int level=LEVEL_DEBUG, bool is_threadsafe=false);
		int open(const char *filename, int level=LEVEL_DEBUG,
			bool is_threadsafe=false, uint64 rotate_size=0);
		void close();

		int logv(int level, const char *fmt, va_list ap);

		int trace(const char *fmt, ...);
		int debug(const char *fmt, ...);
		int info(const char *fmt, ...);
		int warn(const char *fmt, ...);
		int error(const char *fmt, ...);
		int fatal(const char *fmt, ...);
};


int log_open(FILE *fp, int level=Logger::LEVEL_DEBUG, bool is_threadsafe=false);
int log_open(const char *filename, int level=Logger::LEVEL_DEBUG, bool is_threadsafe=false, uint64 rotate_size=0);
int log_level();
void set_log_level(int level);
int log_write(int level, const char *fmt, ...);


#ifdef NDEBUG
	#define log_trace(fmt, args...) do{}while(0)
#else
	#define log_trace(fmt, args...)	\
		log_write(Logger::LEVEL_TRACE, " " fmt, ##args)
#endif

#define log_debug(fmt, args...)	\
    log_write(Logger::LEVEL_DEBUG, "[%s:%s#%d] " fmt, __FILE__, __FUNCTION__, __LINE__, ##args)
#define log_info(fmt, args...)	\
	log_write(Logger::LEVEL_INFO,  "[%s] " fmt, __FILE__, ##args)
#define log_warn(fmt, args...)	\
	log_write(Logger::LEVEL_WARN,  "[%s:%s#%d] " fmt, __FILE__, __FUNCTION__, __LINE__, ##args)
#define log_error(fmt, args...)	\
	log_write(Logger::LEVEL_ERROR, "[%s:%s#%d] " fmt, __FILE__, __FUNCTION__, __LINE__, ##args)
#define log_fatal(fmt, args...)	\
	log_write(Logger::LEVEL_FATAL, "[%s:%s#%d] " fmt, __FILE__, __FUNCTION__, __LINE__, ##args)


#endif
