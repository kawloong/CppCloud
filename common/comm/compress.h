#ifndef _COMPRESS_H_
#define _COMPRESS_H_


class ComPress
{
public:
	unsigned long compressBound( unsigned long textlen);/* 压缩后的长度是不会超过blen的 需要把字符串的结束符'\0'也一并处理 */  
	bool compress(char desbuf[],  unsigned long* desbuflen, const char srcbuf[], unsigned long srcbuflen);
	bool uncompress(char desbuf[], unsigned long* desbuflen, const char srcbuf[], unsigned long srcbuflen);
	bool compressFile(const char* desFileName, const char* srcFileName);
	bool uncompressFile(const char* desFileName, const char* srcFileName);

	unsigned long get_file_size(const char *path);

};


#endif //_TOOLS_H_
