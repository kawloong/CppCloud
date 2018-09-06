
#include <stdio.h>
#include <stdlib.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>
#include <unistd.h>
//#include <openssl/des.h>
#include <openssl/evp.h>
#include "des_code.h"

typedef const unsigned char* ucharptr_t;
#define DES_BREAK_IF0(n, rt) if(!(n)) { ret=rt; break; } // assert(n)

char* Des::Encrypt( const char* pdata,unsigned int nlen, const char* key, const char* iv, unsigned int* outlen )
{
	int ret = 0;
	unsigned char* result = NULL;

	do 
	{
		DES_BREAK_IF0(pdata && nlen > 0 && key, -1);
		EVP_CIPHER_CTX ctx;
		EVP_CIPHER_CTX_init(&ctx);
        ret = EVP_EncryptInit_ex(&ctx, (NULL==iv? EVP_des_ecb() : EVP_des_cbc()),
            NULL, (ucharptr_t)key, (ucharptr_t)iv);
		DES_BREAK_IF0(1 == ret, -2);

		result = (unsigned char*)malloc(nlen + 64); // 足够大的空间
		int len1 = 0;
		ret = EVP_EncryptUpdate(&ctx, result, &len1, (ucharptr_t)pdata, nlen);
		DES_BREAK_IF0(1 == ret, -3);

		int len2 = 0;
		ret = EVP_EncryptFinal_ex(&ctx, result+len1, &len2); 
		DES_BREAK_IF0(1 == ret, -4);

		ret = EVP_CIPHER_CTX_cleanup(&ctx);
		DES_BREAK_IF0(1 == ret, -5);

		ret = 0;
		if (outlen)
		{
			*outlen = len1+len2;
		}
	}
	while(0);

	if (ret < 0)
	{
		free(result);
		result = NULL;
	}

	return (char*)result;
}

char* Des::Decrypt( const char* pdata,unsigned int nlen, const char* key, const char* iv, unsigned int* outlen )
{
	int ret = 0;
	unsigned char* result = NULL;

	do 
	{
		DES_BREAK_IF0(pdata && nlen > 0 && key, -1);

		EVP_CIPHER_CTX ctx;
		EVP_CIPHER_CTX_init(&ctx);
		ret = EVP_DecryptInit_ex(&ctx, (NULL==iv? EVP_des_ecb() : EVP_des_cbc()),
            NULL, (ucharptr_t)key, (ucharptr_t)iv);
		DES_BREAK_IF0(1 == ret, -2);

		result = (unsigned char*)malloc(nlen + 64); // 足够大的空间
		int len1 = 0;
		ret = EVP_DecryptUpdate(&ctx, result, &len1, (ucharptr_t)pdata, nlen);
		DES_BREAK_IF0(1 == ret, -3);

		int len2 = 0;
		ret = EVP_DecryptFinal_ex(&ctx, result+len1, &len2); 
		DES_BREAK_IF0(1 == ret, -4);

		ret = EVP_CIPHER_CTX_cleanup(&ctx);
		DES_BREAK_IF0(1 == ret, -5);

		ret = 0;
		if (outlen)
		{
			*outlen = len1+len2;
		}
	}
	while (0);

	if (ret < 0)
	{
		free(result);
		result = NULL;
	}

	return (char*)result;
}
