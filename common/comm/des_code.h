#ifndef _DES_CBC_H_
#define _DES_CBC_H_

// des_cbc 模式的DES加解密,密码:key+iv
// des_ecb 模式的DES加解密,密码:key
// 注意: key和iv必须有8字节,否则解码会有错;
class Des // 当iv==null时是EBC模式,否则CBC
{
public:
    static char* Encrypt(const char* pdata,unsigned int nlen, const char* key, const char* iv, unsigned int* outlen);

    static char* Decrypt(const char* pdata,unsigned int nlen, const char* key, const char* iv, unsigned int* outlen);
};


#endif