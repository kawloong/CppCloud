/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/

//#include <inttypes.h>
#include <unistd.h>
#include <stdarg.h>
//#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "log.h"

static Logger logger;

int log_open(FILE *fp, int level, bool is_threadsafe){
	return logger.open(fp, level, is_threadsafe);
}

int log_open(const char *filename, int level, bool is_threadsafe, uint64 rotate_size){
	return logger.open(filename, level, is_threadsafe, rotate_size);
}

int log_level(){
	return logger.level();
}

void set_log_level(int level){
	logger.set_level(level);
}

int log_write(int level, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(level, fmt, ap);
	va_end(ap);
	return ret;
}

/*****/

Logger::Logger(){
	fp = stdout;
	level_ = LEVEL_DEBUG;
    mutex = NULL;
    gzMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(gzMutex, NULL);

	filename[0] = '\0';
    fileseq = 0;
    logdate = 0;
	rotate_size = 0;
	stats.w_curr = 0;
	stats.w_total = 0;
}

Logger::~Logger(){
	if(mutex){
		pthread_mutex_destroy(mutex);
		free(mutex);
    }
    if(gzMutex){
        pthread_mutex_destroy(gzMutex);
        free(gzMutex);
    }
	this->close();
}

void Logger::threadsafe(){
	if(mutex){
		pthread_mutex_destroy(mutex);
		free(mutex);
		mutex = NULL;
	}
	mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
}

int Logger::open(FILE *fp, int level, bool is_threadsafe){
	this->fp = fp;
	this->level_ = level;
	if(is_threadsafe){
		this->threadsafe();
	}
	return 0;
}

int Logger::open(const char *filename, int level, bool is_threadsafe, uint64 rotate_size){
	if(strlen(filename) > LOGPATH_MAX - 20){
		fprintf(stderr, "log filename too long!");
		return -1;
	}
	strcpy(this->filename, filename);

	FILE *fp;
	if(strcmp(filename, "stdout") == 0){
		fp = stdout;
	}else if(strcmp(filename, "stderr") == 0){
		fp = stderr;
	}else{
		fp = fopen_ex(filename);
        if(fp == NULL){
            //fprintf(stderr, "fopen logfile %s :%s!", filename, strerror(errno));
			return -1;
		}

		struct stat st;
		int ret = fstat(fileno(fp), &st);
		if(ret == -1){
			fprintf(stderr, "fstat log file %s error!", filename);
			return -1;
		}else{
			this->rotate_size = rotate_size;
			stats.w_curr = st.st_size;
		}
	}
	return this->open(fp, level, is_threadsafe);
}

// 日志文件格式：filename.yyyymmdd.n
// 此函数是打开合适的当天的文件
FILE* Logger::fopen_ex(const char* filename)
{
    char newpath[LOGPATH_MAX];
    time_t nowt = time(NULL);
    struct tm tmnow;
    struct stat st;
    FILE* logfl = NULL;
    int ret;

    localtime_r(&nowt, &tmnow);

    do 
    {
        snprintf(newpath, sizeof(newpath), "%s.%04d%02d%02d_%d",
            filename,
            tmnow.tm_year + 1900, tmnow.tm_mon + 1, tmnow.tm_mday,
            this->fileseq);
        ret = stat(newpath, &st);
    }
    while (0 == ret && (uint64)st.st_size > rotate_size-10 && ++fileseq); // 如果文件存在且size大于最大尺度

    if ( (logfl = fopen(newpath, "a")) == NULL )
    {
        fprintf(stderr, "fopen logfile %s :%s!\n", newpath, strerror(errno));
    }
    else
    {
        const char* fname;
        tmnow.tm_hour = 0;
        tmnow.tm_min = 0;
        tmnow.tm_sec = 0;
        logdate = mktime(&tmnow); // 只看日期，忽略时分秒

        // 符号链接处理
        unlink(filename);
        fname = strrchr(newpath, '/');
        
        symlink((fname? fname+1 : newpath), filename);
    }

    return logfl;
}


void Logger::close(){
	if(fp != stdin && fp != stdout){
		fclose(fp);
	}
}

void Logger::rotate(){
	fclose(fp);
	
	fp = fopen_ex(this->filename);
	if(fp)
    {
        stats.w_curr = 0;
	}
}

int Logger::get_level(const char *levelname){
	if(strcmp("trace", levelname) == 0){
		return LEVEL_TRACE;
	}
	if(strcmp("debug", levelname) == 0){
		return LEVEL_DEBUG;
	}
	if(strcmp("info", levelname) == 0){
		return LEVEL_INFO;
	}
	if(strcmp("warn", levelname) == 0){
		return LEVEL_WARN;
	}
	if(strcmp("error", levelname) == 0){
		return LEVEL_ERROR;
	}
	if(strcmp("fatal", levelname) == 0){
		return LEVEL_FATAL;
	}
	if(strcmp("none", levelname) == 0){
		return LEVEL_NONE;
	}
	return LEVEL_DEBUG;
}

inline static const char* level_name(int level){
	switch(level){
		case Logger::LEVEL_FATAL:
			return "[FATAL]";
		case Logger::LEVEL_ERROR:
			return "[ERROR]";
		case Logger::LEVEL_WARN:
			return "[WARN ]";
		case Logger::LEVEL_INFO:
			return "[INFO ]";
		case Logger::LEVEL_DEBUG:
			return "[DEBUG]";
		case Logger::LEVEL_TRACE:
			return "[TRACE]";
	}
	return "";
}

#define LEVEL_NAME_LEN	8
#define LOG_BUF_LEN		4096

int Logger::logv(int level, const char *fmt, va_list ap){
	if(level < logger.level_){
		return 0;
	}

	char buf[LOG_BUF_LEN];
	size_t len;
	char *ptr = buf;
    static int pid = getpid();

	time_t time;
	struct timeval tv;
	struct tm *tm;
	gettimeofday(&tv, NULL);
	time = tv.tv_sec;
	tm = localtime(&time);
	/* %3ld 在数值位数超过3位的时候不起作用, 所以这里转成int */
	len = snprintf(ptr, LOG_BUF_LEN, "%04d-%02d-%02d %02d:%02d:%02d.%03d%s[%d/%x]",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec/1000),
        level_name(level), pid, (int)pthread_self());

    do 
    {
        ptr += len;
        if (len < LOG_BUF_LEN){
            len = vsnprintf(ptr, LOG_BUF_LEN-len, fmt, ap);
            if(len > 0) {
                ptr += len;
            }
        }

        // 消息过长

        if (ptr > buf + LOG_BUF_LEN - 6){
            memcpy(buf + LOG_BUF_LEN - 6, "...?", 4);
            ptr = buf + (LOG_BUF_LEN - 2);
        }

	    *ptr++ = '\n';
	    *ptr = '\0';

	    len = ptr - buf;
    } while (false);

	// change to write(), without locking?
	if(this->mutex){
		pthread_mutex_lock(this->mutex);
	}
    fwrite(buf, (len>LOG_BUF_LEN? LOG_BUF_LEN: len), 1, this->fp);
	fflush(this->fp);

	stats.w_curr += len;
	stats.w_total += len;
	
    if (difftime(time, logdate) >= 3600*24)
    {
        fileseq = 0;
        this->rotate();

        // 压缩旧历史文件
        if (0 == pthread_mutex_trylock(gzMutex))
        {
            char* logpath = NULL; // heap-alloc
            char* cmd = NULL; // heap-alloc
            const char* tarcmd = "find -regex \".*log.[0-9]+_[0-9]+$\" -type f -mtime +2 -exec gzip {} \\;"
                " > /dev/null &"; // const-text
            
            asprintf(&logpath, "%s", filename);
            char* sepch = strrchr(logpath, '/');
            if (sepch)
            {
                *sepch = '\0';
                asprintf(&cmd, "cd %s && %s", logpath, tarcmd);
                system(cmd);
            }
            else
            {
                system(tarcmd);
            }

            if(logpath) free(logpath);
            if(cmd) free(cmd);

            pthread_mutex_unlock(gzMutex);
        }
    }
    else if ( (rotate_size > 0 && stats.w_curr > rotate_size) )
    {
		this->rotate();
	}

	if(this->mutex)
    {
		pthread_mutex_unlock(this->mutex);
	}

	return len;
}

int Logger::trace(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_TRACE, fmt, ap);
	va_end(ap);
	return ret;
}

int Logger::debug(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_DEBUG, fmt, ap);
	va_end(ap);
	return ret;
}

int Logger::info(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_INFO, fmt, ap);
	va_end(ap);
	return ret;
}

int Logger::warn(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_WARN, fmt, ap);
	va_end(ap);
	return ret;
}

int Logger::error(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_ERROR, fmt, ap);
	va_end(ap);
	return ret;
}

int Logger::fatal(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_FATAL, fmt, ap);
	va_end(ap);
	return ret;
}
