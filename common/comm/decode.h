#ifndef __DECODE_H__
#define __DECODE_H__

#ifdef __cplusplus
extern "C" {
#endif


/**
* @param src     :  Need to encode the string
* @param src_len :  Need to encode the length of the string
* @param dst     :  The buffer to storing the encoded string
* @param dst_len :  The size of the buffer after storing the encoded string
*/
extern int base64_encode(const char *src, int srcLen, char *dst, int dstLen);

/**
* @param dest :  String after base64 decoding
* @param src  :  Need to decode the string
* @param len  :  Need to decode the length of the src string
* @return int :  Returns the decoded length
*/
extern int base64_decode(char *dst, const char *src, int srcLen);


/**
* @param dest :  String after URL decoding
* @param src  :  Need to decode the URL string
* @param len  :  Need to decode the length of the URL
* @return int :  Returns the decoded URL length
*/
extern int url_decode(char *dst, const char *src, int len);

/**
* @param src     :  Need to encode the URL string
* @param srcLen  :  Need to encode the length of the URL
* @param dst     :  The buffer to storing the encoded URL
* @param dstLen  :  The size of the buffer after storing the encoded URL
* @return int    :  Length of the encoded URL string
*/
extern int url_encode(const char *src, int srcLen, char *dst, int dstLen);
//extern int url_encode(const char *src, int srcLen, char **dst, int *dstLen);


/**
* @param dst     :  The buffer to storing the encoded string
* @param dstLen  :  The size of the buffer after storing the encoded string
* @param src     :  Need to encode the string
* @param srcLen  :  Need to encode the length of the string
* @param key     :  secret key
*/
extern int DES_encode(char *dst, int *dstLen, const char *src, int srcLen, const char *key);

/**
* @param dest   :  String after DES decoding
* @param dstLen :  Need to decode the length of the src string
* @param src    :  Need to decode the string
* @param srcLen :  Need to decode the length of the src string
*/
extern int DES_decode(char *dst, int *dstLen, const char *src, int srcLen, const char *key);

typedef unsigned int gzsize_t;

/**
* @param dest   :  String after gzip compress
* @param dstLen :  recv buff len
* @param src    :  Need to compress the string buff
* @param srcLen :  Need to compress the length of the src string buff
*/
extern int gzCompress(const char *src, gzsize_t srcLen, char *dest, gzsize_t destLen);

/**
* @param dest   :  String after gzip uncompress
* @param dstLen :  recv buff len
* @param src    :  Need to uncompress the data buff
* @param srcLen :  Need to uncompress the length of the src buff
*/
extern int gzDecompress(const char *src, gzsize_t srcLen, char *dst, gzsize_t dstLen);
    
#ifdef __cplusplus
}
#endif

#endif /* __DECODE_H__ */
