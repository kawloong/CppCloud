#include <cstring>
#include <cstdlib>
#include "base64.h"

static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// 二进制 --> 字符串
int Base64::Encode( const void* inbuf, int size, char** str )
{
    char *s, *p;
    int i,c;
    unsigned char *q;

    p = s = (char*)calloc(1, size*4/3+4);

    if(p == NULL) return -1;

    q = (unsigned char*)inbuf;

    for(i = 0; i < size;)
    {
        c=q[i++];
        c*=256;
        if(i < size)
            c+=q[i];
        i++;
        c*=256;
        if(i < size)
            c+=q[i];
        i++;
        p[0]=base64[(c&0x00fc0000) >> 18];
        p[1]=base64[(c&0x0003f000) >> 12];
        p[2]=base64[(c&0x00000fc0) >> 6];
        p[3]=base64[(c&0x0000003f) >> 0];
        if(i > size)
            p[3]='=';
        if(i > size+1)
            p[2]='=';
        p+=4;
    }

    *p=0;
    *str = s;
    return strlen(s);
}

static char find_pos(char ch)   
{ 
    char* ptr = (char*)strrchr(base64, ch);//the last position (the only) in base[] 
    return (ptr - base64); 
}

// 字符串 --> 二进制
int Base64::Decode( const char* str, int size, void** outbuf )
{
    int outbuf_len = (size / 4) * 3;
    int outLen = 0;
	int equal_count = 0; 
	//char *outbuf = NULL; 
    const char *inbuf = str;
	char *f = NULL; 
	int tmp = 0; 
	int temp = 0; 
	char need[3]; 
	int prepare = 0; 
	int i = 0; 

	if (*(inbuf + size - 1) == '=') 
	{ 
		equal_count += 1; 
	} 
	if (*(inbuf + size - 2) == '=') 
	{ 
		equal_count += 1; 
	} 
	if (*(inbuf + size - 3) == '=') 
	{//seems impossible 
		equal_count += 1; 
	} 

	outbuf_len = outbuf_len - equal_count + 1 ;//+1Ϊ\0
	//outbuf = (char *)malloc(outbuf_len);
    *outbuf = calloc(1, outbuf_len);
    f = (char*)(*outbuf);

    if (NULL == f)
    {
        return -1;
    }

	while (tmp < (size - equal_count)) 
	{ 
		temp = 0; 
		prepare = 0; 
		memset(need, 0, 3); 
		while (temp < 4) 
		{ 
			if (tmp >= (size - equal_count)) 
			{ 
				break; 
			} 
			prepare = (prepare << 6) | (find_pos(inbuf[tmp])); 
			temp++; 
			tmp++; 
		} 
		prepare = prepare << ((4-temp) * 6); 
		for (i=0; i<3 ;i++ ) 
		{ 
			if (i == temp) 
			{ 
				break; 
			} 
			*f = (char)((prepare>>((2-i)*8)) & 0xFF); 
			f++; 
		} 
	} 

	//*f = '\0'; 
	outLen = outbuf_len - 1;

	return outLen; 
}


// 二进制 --> 字符串
int Base64::Encode(std::string& iobuff)
{
    int ret;
    char* outstr = NULL;
    int insize = iobuff.size();
    const void* inbuffptr = iobuff.data();

    ret = Encode(inbuffptr, insize, &outstr);
    if (ret > 0)
    {
        iobuff.assign(outstr, ret);
    }

    if (outstr)
    {
        free(outstr);
    }

    return ret;
}

// 字符串 --> 二进制
int Base64::Decode(std::string& iobuff)
{
    int ret;
    void* outbuff = NULL;
    int insize = iobuff.size();
    const char* inbuffstr = iobuff.data();

    ret = Decode(inbuffstr, insize, &outbuff);
    if (ret > 0)
    {
        iobuff.assign((const char*)outbuff, ret);
    }

    if (outbuff)
    {
        free(outbuff);
    }

    return ret;
}


