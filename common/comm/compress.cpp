#include "compress.h"
#include <zlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


bool ComPress::compressFile(const char* desFileName, const char* srcFileName){

	if(desFileName == (const char*)NULL || srcFileName == (const char*)NULL){
		return false;
	}

	if(access(srcFileName, F_OK|R_OK) != 0) //文件不存在或没有读权限
	{ 
		return false;
	}

	int fsrcFileName = open(srcFileName, O_RDONLY);	
	if(fsrcFileName == -1){
		return false;
	}

	bool result = true;
	int fdesFileName = open(desFileName, O_CREAT|O_APPEND|O_TRUNC|O_RDWR, 0666);
	unsigned long fileSize = ComPress::getInstance()->get_file_size(srcFileName);
	char* psrcbuffer = (char*)malloc(fileSize);
	memset(psrcbuffer, 0, fileSize);
	ssize_t readsize = 0;
	unsigned long desbuflen = 0;
	char* desbuf = (char*)NULL;

	while(1){

		readsize = read(fsrcFileName, psrcbuffer, fileSize);

		if (readsize > 0)
		{
			desbuflen = ComPress::getInstance()->compressBound(fileSize);
			desbuf = (char*) malloc(desbuflen);
			if (ComPress::getInstance()->compress(desbuf, &desbuflen, psrcbuffer, readsize)){

			}else
			{
				fprintf(stderr, "compressFile read error errno=%d|errstr=%s\n", 
					errno, strerror(errno));
				free(desbuf);
				result = false;
				break;
			}

			if(write(fdesFileName, desbuf, desbuflen) == -1){
				fprintf(stderr, "compressFile write error errno=%d|errstr=%s|fdesFileName=%d\n", 
					errno, strerror(errno), fdesFileName);
				free(desbuf);
				result = false;
				break;
			}

			free(desbuf);

		}else if(readsize == 0){ //读到文件尾
			result = true;
			break;
		}else if(readsize == -1 && errno ==  EINTR){//信号中断，需要继续读
			fprintf(stderr, "compressFile read error errno=%d|errstr=%s\n", 
				errno, strerror(errno));
			continue;
		}else{ //出错
			fprintf(stderr, "compressFile read error errno=%d|errstr=%s\n", 
				errno, strerror(errno));
			result = false;
			break;
		}
	}
	close(fsrcFileName);
	close(fdesFileName);
	free(psrcbuffer);
	return result;
}

bool ComPress::uncompressFile(const char* desFileName, const char* srcFileName){

	if(desFileName == (const char*)NULL || srcFileName == (const char*)NULL){
		return false;
	}

	if(access(srcFileName, F_OK|R_OK) != 0) //文件不存在或没有读权限
	{ 
		return false;
	}

	int fsrcFileName = open(srcFileName, O_RDONLY);	
	if(fsrcFileName == -1){
		return false;
	}

	bool result = true;
	int fdesFileName = open(desFileName, O_CREAT|O_APPEND|O_TRUNC|O_RDWR, 0666);
	unsigned long fileSize = ComPress::getInstance()->get_file_size(srcFileName);
	char* psrcbuffer = (char*)malloc(fileSize);
	memset(psrcbuffer, 0, fileSize);
	ssize_t readsize = 0;
	char* desbuf = (char*)NULL;
	unsigned long desbuflen = fileSize * 5; //网上说设为4倍一般就够了，这里我设置为5倍，文本文件一般会4倍多点; 
	while(1){

		readsize = read(fsrcFileName, psrcbuffer, fileSize);

		if (readsize > 0)
		{	
			desbuf = (char*) malloc(desbuflen);
			if (ComPress::getInstance()->uncompress(desbuf, &desbuflen, psrcbuffer, readsize)){

			}else
			{
				fprintf(stderr, "uncompressFile read error errno=%d|errstr=%s\n", 
					errno, strerror(errno));
				free(desbuf);
				result = false;
				break;
			}

			if(write(fdesFileName, desbuf, desbuflen) == -1){
				fprintf(stderr, "uncompressFile write error errno=%d|errstr=%s|fdesFileName=%d\n", 
					errno, strerror(errno), fdesFileName);
				free(desbuf);
				result = false;
				break;
			}

			free(desbuf);

		}else if(readsize == 0){ //读到文件尾
			result = true;
			break;
		}else if(readsize == -1 && errno ==  EINTR){//信号中断，需要继续读
			fprintf(stderr, "uncompressFile read error errno=%d|errstr=%s\n", 
				errno, strerror(errno));
			continue;
		}else{ //出错
			fprintf(stderr, "uncompressFile read error errno=%d|errstr=%s\n", 
				errno, strerror(errno));
			result = false;
			break;
		}
	}
	close(fsrcFileName);
	close(fdesFileName);
	free(psrcbuffer);
	return result;
}

/* 压缩后的长度是不会超过blen的 需要把字符串的结束符'\0'也一并处理 */  
unsigned long ComPress::compressBound(unsigned long textlen){
	return ::compressBound(textlen); 
}

bool ComPress::compress(char desbuf[],  unsigned long* desbuflen, const char srcbuf[],  unsigned long srcbuflen){

	if(::compress((Bytef*)desbuf, desbuflen, (Bytef*)srcbuf, srcbuflen) == Z_OK)  
	{  
		return true;
	}else{
		fprintf(stderr, "compress failed!\n");  
		return false;
	}

}

bool ComPress::uncompress(char desbuf[], unsigned long* desbuflen, const char srcbuf[],  unsigned long srcbuflen){
	
	if(::uncompress((Bytef*)desbuf, desbuflen, (Bytef*)srcbuf, srcbuflen) == Z_OK)  
	{  
		return true;
	}else{
		fprintf(stderr, "uncompress failed!\n");  
		return false;
	}

}

unsigned long ComPress::get_file_size(const char *path)  
{  
	unsigned long filesize = -1;      
	struct stat statbuff;  
	if(stat(path, &statbuff) < 0){  
		return filesize;  
	}else{  
		return statbuff.st_size;
	}  
}


