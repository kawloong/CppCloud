#ifndef _BASE_64_H_
#define _BASE_64_H_
#include <string>

class Base64
{
public:
    /* @summey: bas64编码函数
    ** @param: inbuf [in] 输入数据起始地址
    ** @param: size [in] 输入数据size,字节单位
    ** @param: str [out] 接收编码输出数据返回堆指针,需调用者free()释放
    ** @return: 成功返回输出编码后数据的字节数, 失败返回小于0
    **/
    static int Encode(const void* inbuf, int size, char** str);

    /* @summey: bas64解码函数
    ** @param: str [in] 输入数据起始地址
    ** @param: size [in] 输入数据size,字节单位
    ** @param: outbuf [out] 接收解码输出数据返回堆指针,需调用者free()释放
    ** @return: 成功返回输出解码后数据的字节数, 失败返回小于0
    **/
    static int Decode(const char* str, int size, void** outbuf);


    /* @summey: 面向string的bas64编码函数
    ** @param: iobuff [in/out] 输入二进制数据,输出base64编码后字符串数据
    ** @return: 成功返回输出解码后数据的字节数, 失败返回小于0
    **/
    static int Encode(std::string& iobuff);

    /* @summey: 面向string的bas64解码函数
    ** @param: iobuff [in/out] 输入加密的字符串数据,输出base64解码后的二进制数据
    ** @return: 成功返回输出解码后数据的字节数, 失败返回小于0
    **/
    static int Decode(std::string& iobuff);
};

#endif