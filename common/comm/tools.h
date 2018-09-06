#ifndef _TOOLS_H_
#define _TOOLS_H_
#include <pthread.h>


using namespace std;
class Tools{

public:
	unsigned long compressBound( unsigned long textlen);/* 压缩后的长度是不会超过blen的 需要把字符串的结束符'\0'也一并处理 */  
	bool compress(char desbuf[],  unsigned long* desbuflen, const char srcbuf[], unsigned long srcbuflen);
	bool uncompress(char desbuf[], unsigned long* desbuflen, const char srcbuf[], unsigned long srcbuflen);
	bool compressFile(const char* desFileName, const char* srcFileName);
	bool uncompressFile(const char* desFileName, const char* srcFileName);

	unsigned long get_file_size(const char *path);

	static Tools* getInstance();	
	~Tools();

private:
	Tools();
	Tools(const Tools& other);
	Tools& operator=(const Tools& other);

	static Tools* tools;
	static pthread_mutex_t lock; //线程
};


#endif //_TOOLS_H_
