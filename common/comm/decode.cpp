#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <memory.h>
#include <zlib.h>

#include "decode.h"

#ifdef __cplusplus
extern "C" {
#endif


/*=========================== Base64 decode or encode ===========================*/

static unsigned char base64_index[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 
    64, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64, 
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

/**
* @param dest :  String after base64 decoding
* @param src  :  Need to decode the string
* @param len  :  Need to decode the length of the src string
* @return int :  Returns the decoded length
*/
int base64_decode(char *dst, const char *src, int srcLen)
{
    unsigned char ch1, ch2;
    unsigned char ch3, ch4;
    char *ptr = dst;
    int i;
    
    for (i = 0; i < srcLen; ++i)
    {
        ch1 = base64_index[(int)src[i]];
        if(ch1 == 64) {
            continue;
        }
        
        ch2 = base64_index[(int)src[++i]];
        if(ch2 == 64) {
            continue;
        }
        
        *(ptr++) = ch1<<2 | ch2>>4;
        ch3 = base64_index[(int)src[++i]];
        if(src[i] == '=' || ch3 == 64) {
            continue;
        }
        
        *(ptr++) = ch2<<4 | ch3>>2;
        ch4 = base64_index[(int)src[++i]];
        if(src[i] == '=' || ch4 == 64) {
            continue;
        }
        
        *(ptr++) = ch3<<6 | ch4;
    }
    
    return (ptr - dst);
}

const char * base64_char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
* @param src     :  Need to encode the string
* @param src_len :  Need to encode the length of the string
* @param dst     :  The buffer to storing the encoded string
* @param dst_len :  The size of the buffer after storing the encoded string
*/
int base64_encode(const char *src, int srcLen, char *dst, int dstLen)
{
    int i, j;
    int max_len;
    unsigned char ch, ch2;

    /* Check parameters */
    if (!src || (srcLen <= 0)) {
        return -1;
    }

    /* Check parameters */
    if (!dst || (dstLen <= 0)) {
        return -1;
    }

    /* dst buffer min size is 5bytes */
    max_len = dstLen - 1;
    if (max_len < 4) {
        return -1;
    }

    for (i=j=0; i<srcLen; i+=3) {
        /* Get first 6 bits */
        ch  = (unsigned char)(src[i] >> 2);
        ch &= (unsigned char)0x3F;
        dst[j++] = base64_char[(int)ch];
        
        /* Get second 6 bits */
        ch  = (unsigned char)(src[i] << 4);
        ch &= (unsigned char)0x30;
        if ((i + 1) >= srcLen) {
            dst[j++] = base64_char[(int)ch];
            dst[j++] = '=';
            dst[j++] = '=';
            break;
        }
        ch2  = (unsigned char)(src[i+1] >> 4);
        ch2 &= (unsigned char)0x0F;
        ch |= ch2;
        dst[j++] = base64_char[(int)ch];
        
        /* Get third 6 bits */
        ch  = (unsigned char)(src[i+1] << 2);
        ch &= (unsigned char)0x3C;
        if ((i + 2) >= srcLen) {
            dst[j++] = base64_char[(int)ch];
            dst[j++] = '=';
            break;
        }
        ch2  = (unsigned char)(src[i+2] >> 6);
        ch2 &= (unsigned char)0x03;
        ch |= ch2;
        dst[j++] = base64_char[(int)ch];
        
        /* Get fourth 6 bits */
        ch = (unsigned char)src[i+2];
        ch &= (unsigned char)0x3F;
        dst[j++] = base64_char[(int)ch];

        if (((j + 4) > max_len) && ((i + 3) < srcLen)) {
            return -1;
        }
    }

    dst[j] = '\0';
    return j;
}


/*=========================== URL decode or encode ===========================*/

/**
* @param dest :  String after URL decoding
* @param src  :  Need to decode the URL string
* @param len  :  Need to decode the length of the URL
* @return int :  Returns the decoded URL length
*/
int url_decode(char *dst, const char *src, int len)
{
    char *pbuf = dst;
    char *data = (char *)src;
    
    int value;
    int c;
    
    while (len--) {
        if (*data == '+') {
            *pbuf = ' ';
        }
        else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1))
               && isxdigit((int) *(data + 2)))
        {
            c = ((unsigned char *)(data+1))[0];
            if (isupper(c))
               c = tolower(c);
            value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
            c = ((unsigned char *)(data+1))[1];
            if (isupper(c))
               c = tolower(c);
            value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;
            
            *pbuf = (char)value ;
            data += 2;
            len -= 2;
        } else {
            *pbuf = *data;
        }
        data++;
        pbuf++;
    }
    *pbuf = '\0';
    return (pbuf - src);
}

#if 0
/**
* @param src     :  Need to encode the URL string
* @param srcLen  :  Need to encode the length of the URL
* @param dst     :  The buffer to storing the encoded URL
* @param dstLen  :  The size of the buffer after storing the encoded URL
* @return int    :  Length of the encoded URL string
**/
int url_encode(const char *src, int srcLen, char **dst, int *dstLen)
{
    char c;
    unsigned char *start, *to;
    unsigned char const *from, *end;
    unsigned char hexchars[] = "0123456789ABCDEF";

    start = to = NULL;
    from = end = NULL;

    /* Check parameters */
    if (!src || (srcLen <= 0)) {
        return -1;
    }

    /* Check parameters */
    if ((dst == NULL) || (srcLen == NULL)) {
        return -1;
    }
    
    from = (unsigned char const *)src;
    end = (unsigned char const *)(src + srcLen);
    start = to = (unsigned char *)malloc(3 * srcLen + 1);

    while (from < end) {
        c = *from++;
        
        if (c == ' ') {
            *to++ = '+';
        } else if ((c < '0' && c != '-' && c != '.')
                 ||(c < 'A' && c > '9')
                 ||(c > 'Z' && c < 'a' && c != '_')
                 ||(c > 'z')) {
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 15];
            to += 3;
        } else {
            *to++ = c;
        }
    }
    *to = 0;

    *dst = (char *)start;
    dstLen = to - start;

    return 0;
}

#endif

/**
* @param src     :  Need to encode the URL string
* @param srcLen  :  Need to encode the length of the URL
* @param dst     :  The buffer to storing the encoded URL
* @param dstLen  :  The size of the buffer after storing the encoded URL
* @return int    :  Length of the encoded URL string
*/
int url_encode(const char *src, int srcLen, char *dst, int dstLen)
{
    char c;
    unsigned int max_len, k = 0;
    unsigned char const *from, *end;
    unsigned char hexchars[] = "0123456789ABCDEF";

    from = end = NULL;

    /* Check parameters */
    if (!src || (srcLen <= 0)) {
        return -1;
    }

    /* Check parameters */
    if ((dst == NULL) || (srcLen <= 0)) {
        return -1;
    }
    
    from = (unsigned char const *)src;
    end = (unsigned char const *)(src + srcLen);
    max_len = dstLen - 1;
    
    while (from < end) {
        c = *from++;
        if (c == ' ') {
            dst[k++] = '+';
        } else if ((c < '0' && c != '-' && c != '.')
                 ||(c < 'A' && c > '9')
                 ||(c > 'Z' && c < 'a' && c != '_')
                 ||(c > 'z')) {
            dst[k++] = '%';
            dst[k++] = hexchars[c >> 4];
            dst[k++] = hexchars[c & 15];
        } else {
            dst[k++] = c;
        }

        if (k >= max_len) {
            break;
        }
    }
    dst[k] = '\0';
    
    return k;
}


/*=========================== DES decode or encode ===========================*/

typedef char ElemType;

/* Initial permutation table IP*/
int IP_Table[64] = {
    57, 49, 41, 33, 25, 17, 9,  1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7,
    56, 48, 40, 32, 24, 16, 8,  0,
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6
}; 

/* Inverse initial permutation table IP^-1 */
int IP_1_Table[64] = {
    39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30,
    37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28,
    35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26,
    33, 1, 41, 9,  49, 17, 57, 25,
    32, 0, 40, 8,  48, 16, 56, 24
};

/* Extended permutation table E */
int E_Table[48] = {
    31, 0,  1,  2,  3,  4,
    3,  4,  5,  6,  7,  8,
    7,  8,  9,  10, 11, 12,
    11, 12, 13, 14, 15, 16,
    15, 16, 17, 18, 19, 20,
    19, 20, 21, 22, 23, 24,
    23, 24, 25, 26, 27, 28,
    27, 28, 29, 30, 31, 0
};

/* Permutation function P */
int P_Table[32] = {
    15, 6,  19, 20, 28, 11, 27, 16,
    0,  14, 22, 25, 4,  17, 30, 9,
    1,  7,  23, 13, 31, 26, 2,  8,
    18, 12, 29, 5,  21, 10, 3,  24
};

/* S box */
int S[8][4][16] ={
    {   /* S1 */
        {14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7},
        {0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8},
        {4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0},
        {15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13}
    },
    {   /* S2 */
        {15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10},
        {3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5},
        {0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15},
        {13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9}
    },
    {   /* S3 */
        {10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8},  
        {13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1},  
        {13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7},  
        {1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12}
    },
    {   /* S4 */
        {7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15},
        {13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9},
        {10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4},
        {3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14}
    },
    {   /* S5 */
        {2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9},
        {14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6},
        {4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14},
        {11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3}
    },
    {   /* S6 */
        {12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11},
        {10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8},
        {9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6},
        {4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13}
    },
    {   /* S7 */
        {4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1},  
        {13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6},  
        {1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2},  
        {6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12}
    },
    {   /* S8 */
        {13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7},
        {1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2},
        {7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8},
        {2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11}
    }
};

/* Replacement selection 1 */
int PC_1[56] = {
    56, 48, 40, 32, 24, 16, 8,
    0,  57, 49, 41, 33, 25, 17,
    9,  1,  58, 50, 42, 34, 26,
    18, 10, 2,  59, 51, 43, 35,
    62, 54, 46, 38, 30, 22, 14,
    6,  61, 53, 45, 37, 29, 21,
    13, 5,  60, 52, 44, 36, 28,
    20, 12, 4,  27, 19, 11, 3
};

/* Replacement selection 2 */
int PC_2[48] = {
    13, 16, 10, 23, 0,  4,  2,  27,
    14, 5,  20, 9,  22, 18, 11, 3,
    25, 7,  15, 6,  26, 19, 12, 1,
    40, 51, 30, 36, 46, 54, 29, 39,
    50, 44, 32, 46, 43, 48, 38, 55,
    33, 52, 45, 41, 49, 35, 28, 31
};

/* Provisions on the number of left shift */
int MOVE_TIMES[16] = { 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

/* Byte to binary */
static int ByteToBit(ElemType ch, ElemType bit[8])
{
    int cnt;
    
    for (cnt = 0; cnt < 8; cnt++) {
        *(bit + cnt) = (ch >> cnt) & 1;
    }
    
    return 0;
}

/* Binary to byte */
static int BitToByte(ElemType bit[8], ElemType *ch)
{
    int cnt;
    
    for (cnt = 0; cnt < 8; cnt++) {
        *ch |= *(bit + cnt) << cnt;
    }
    
    return 0;
}

/* A string of length 8 is converted to a binary string */
static int Char8ToBit64(ElemType ch[8], ElemType bit[64])
{
    int cnt;
    
    for (cnt = 0; cnt < 8; cnt++) {
        ByteToBit(*(ch+cnt), bit+(cnt<<3));
    }
    
    return 0;  
}

/* A binary string  is converted to string of length 8 */
static int Bit64ToChar8(ElemType bit[64], ElemType ch[8])
{
    int cnt;
    
    memset(ch, 0, 8);
    
    for (cnt = 0; cnt < 8; cnt++) {
        BitToByte(bit+(cnt<<3), ch+cnt);
    }
    
    return 0;
}

/* Key exchange 1 */
static int DES_PC1_Transform(ElemType key[64], ElemType tempbts[56])
{
    int cnt;
    
    for(cnt = 0; cnt < 56; cnt++){
        tempbts[cnt] = key[PC_1[cnt]];
    }
    
    return 0;
}

/* Key exchange 2 */
static int DES_PC2_Transform(ElemType key[56], ElemType tempbts[48])
{
    int cnt;
    
    for (cnt = 0; cnt < 48; cnt++) {
        tempbts[cnt] = key[PC_2[cnt]];
    }
    
    return 0;
}

/* Cyclic left shift */
static int DES_ROL(ElemType data[56], int time)
{
    ElemType temp[56];

    /* Save the bit to move to the right */
    memcpy(temp, data, time);
    memcpy(temp+time, data+28, time);
    
    /* The first 28 bit move */
    memcpy(data, data+time, 28-time);
    memcpy(data+28-time, temp, time);

    /* Back 28 bit move */
    memcpy(data+28, data+28+time, 28-time);
    memcpy(data+56-time, temp+time, time); 

    return 0;
}

/* Generating sub key */
static int DES_MakeSubKeys(ElemType key[64], ElemType subKeys[16][48])
{
    int cnt;
    ElemType temp[56];
    
    DES_PC1_Transform(key, temp);
    
    for (cnt = 0; cnt < 16; cnt++) {
        DES_ROL(temp, MOVE_TIMES[cnt]);
        DES_PC2_Transform(temp, subKeys[cnt]);
    }
    
    return 0;
}

/* IP replacement */
static int DES_IP_Transform(ElemType data[64])
{
    int cnt;
    ElemType temp[64];
    
    for (cnt = 0; cnt < 64; cnt++) {
        temp[cnt] = data[IP_Table[cnt]];
    }
    memcpy(data, temp, 64);
    
    return 0;
}

/* IP inverse replacement */
static int DES_IP_1_Transform(ElemType data[64])
{
    int cnt;
    ElemType temp[64];
    
    for (cnt = 0; cnt < 64; cnt++) {
        temp[cnt] = data[IP_1_Table[cnt]];
    }
    memcpy(data, temp, 64);
    
    return 0;
}

/* Extended permutation */
static int DES_E_Transform(ElemType data[48])
{
    int cnt;
    ElemType temp[48];
    
    for (cnt = 0; cnt < 48; cnt++) {
        temp[cnt] = data[E_Table[cnt]];
    }
    memcpy(data, temp, 48);
    
    return 0;
}

/* P replacement */
static int DES_P_Transform(ElemType data[32])
{
    int cnt;
    ElemType temp[32];
    
    for (cnt = 0; cnt < 32; cnt++) {
        temp[cnt] = data[P_Table[cnt]];
    }
    memcpy(data, temp, 32);
    
    return 0;
}

/* XOR */
static int DES_XOR(ElemType R[48], ElemType L[48], int count)
{
    int cnt;
    
    for (cnt = 0; cnt < count; cnt++) {
        R[cnt] ^= L[cnt];
    }
    
    return 0;  
}

/* S box replacement */
static int DES_SBOX(ElemType data[48])
{
    int cnt;
    int cur1, cur2;
    int line, row, output;
    
    for (cnt = 0; cnt < 8; cnt++) {
        cur1 = cnt * 6;
        cur2 = cnt << 2;
        
        /* Calculation rows and columns in a S box */
        line = (data[cur1] << 1) + data[cur1+5];
        row = (data[cur1+1] << 3) + (data[cur1+2] << 2)
            + (data[cur1+3] << 1) + data[cur1+4];
        output = S[cnt][line][row];

        /* to binary */
        data[cur2] = (output & 0X08) >> 3;
        data[cur2+1] = (output & 0X04) >> 2;
        data[cur2+2] = (output & 0X02) >> 1;
        data[cur2+3] = output & 0x01;
    }
    
    return 0;
}

/* Exchange */
static int DES_Swap(ElemType left[32], ElemType right[32])
{
    ElemType temp[32];
    
    memcpy(temp, left, 32);
    memcpy(left, right, 32);
    memcpy(right, temp, 32);
    
    return 0;
}

/* Encrypt a single group */
static int DES_EncryptBlock(ElemType plainBlock[8], ElemType subKeys[16][48], ElemType cipherBlock[8])
{
    int cnt;
    ElemType plainBits[64];
    ElemType copyRight[48];

    Char8ToBit64(plainBlock,plainBits);
    /* Initial replacement (IP replacement) */
    DES_IP_Transform(plainBits);
  
    /* 16 round iteration */
    for (cnt = 0; cnt < 16; cnt++) {
        memcpy(copyRight, plainBits+32, 32);
        /* The right part is extended replacement, from 32 bits extended to 48 bits */
        DES_E_Transform(copyRight);
        /* The right part and key XOR operation */
        DES_XOR(copyRight, subKeys[cnt], 48);
        /* XOR result into the S box, output 32 bit results */
        DES_SBOX(copyRight);
        /* P replacement */
        DES_P_Transform(copyRight);
        /* The left part and the right part of the plaintext to XOR operation */
        DES_XOR(plainBits, copyRight, 32);
        if (cnt != 15) {
            /* At the end of the exchange of the left and right */
            DES_Swap(plainBits, plainBits+32);
        }
    }
    
    /* Inverse initial permutation (IP^1) */
    DES_IP_1_Transform(plainBits);
    Bit64ToChar8(plainBits, cipherBlock);
    
    return 0;
}

/* Decrypt a single group */
static int DES_DecryptBlock(ElemType cipherBlock[8], ElemType subKeys[16][48],ElemType plainBlock[8])
{
    int cnt;
    ElemType cipherBits[64];
    ElemType copyRight[48];

    Char8ToBit64(cipherBlock, cipherBits);
    /* Initial replacement (IP replacement) */
    DES_IP_Transform(cipherBits);
    
    /* 16 round iteration */
    for(cnt = 15; cnt >= 0; cnt--){
        memcpy(copyRight, cipherBits+32, 32);
        /* The right part is extended replacement, from 32 bits extended to 48 bits */
        DES_E_Transform(copyRight);
        /* The right part and key XOR operation */
        DES_XOR(copyRight, subKeys[cnt], 48);
        /* XOR result into the S box, output 32 bit results */
        DES_SBOX(copyRight);
        /* P replacement */
        DES_P_Transform(copyRight);
        /* The left part and the right part of the plaintext to XOR operation */
        DES_XOR(cipherBits, copyRight, 32);
        if (cnt != 0) {
            /* At the end of the exchange of the left and right */
            DES_Swap(cipherBits, cipherBits+32);
        }
    }

    /* Inverse initial permutation (IP^1) */
    DES_IP_1_Transform(cipherBits);
    Bit64ToChar8(cipherBits, plainBlock);
    
    return 0;
}

/**
* @param dst     :  The buffer to storing the encoded string
* @param dstLen  :  The size of the buffer after storing the encoded string
* @param src     :  Need to encode the string
* @param srcLen  :  Need to encode the length of the string
* @param key     :  secret key
*/
int DES_encode(char *dst, int *dstLen, const char *src, int srcLen, const char *key)
{
    int index, lastLen;
    ElemType plainBlock[8], cipherBlock[8], keyBlock[8];
    ElemType bKey[64], subKeys[16][48];

    /* invalid parameter */
    if (!key || !src || (srcLen <= 0)) {
        return -1;
    }

    /* invalid parameter */
    if (!dst || (*dstLen <= 0)) {
        return -1;
    }
    
    /* Setting key */
    memcpy(keyBlock, key, 8);
    /* Converting a key to a binary stream */
    Char8ToBit64(keyBlock, bKey);
    /* Generating sub key */
    DES_MakeSubKeys(bKey, subKeys);

    for (index = 0; index < srcLen; index += 8) {
        if ((*dstLen - index) < 8) {
            /* Buffer too small */
            return 1;
        }

        //memset(srcBlock, 0, 8);
        lastLen = srcLen - index;
        if (lastLen < 8) {
            memcpy(plainBlock, src+index, lastLen);
            memset(plainBlock+lastLen, '\0', 7-lastLen);
            plainBlock[7] = 8 - lastLen;
        } else {
            memcpy(plainBlock, src+index, 8);
        }
        
        memset(cipherBlock, 0, 8);
        DES_EncryptBlock(plainBlock, subKeys, cipherBlock);
        memcpy(dst+index, cipherBlock, 8);
    }

    *dstLen = index;
    return 0;  
}

/**
* @param dest   :  String after DES decoding
* @param dstLen :  Need to decode the length of the src string
* @param src    :  Need to decode the string
* @param srcLen :  Need to decode the length of the src string
*/
int DES_decode(char *dst, int *dstLen, const char *src, int srcLen, const char *key)
{
    int k = 0;
    int index, lastLen;
    ElemType cipherBlock[8], plainBlock[8], keyBlock[8];
    ElemType bKey[64], subKeys[16][48];

    /* invalid parameter */
    if (!key || !src || (srcLen <= 0)) {
        return -1;
    }

    /* invalid parameter */
    if (!dst || (*dstLen <= 0)) {
        return -1;
    }

    /* The number of bytes in the cipher text must be an integer multiple of 8 */
    if ((srcLen % 8) != 0) {
        return -2;
    }
    
    /* Setting key */
    memcpy(keyBlock, key, 8);
    /* Converting a key to a binary stream */
    Char8ToBit64(keyBlock, bKey);
    /* Generating sub key */
    DES_MakeSubKeys(bKey, subKeys);

    for (index = 0; index < srcLen; index += 8) {
        if ((*dstLen - index) < 8) {
            /* Buffer too small */
            return 1;
        }
        
        //memset(cipherBlock, 0, sizeof(cipherBlock));
        memcpy(cipherBlock, src+index, 8);
        memset(plainBlock, 0, sizeof(plainBlock));
        DES_DecryptBlock(cipherBlock, subKeys, plainBlock);

        if ((index + 8) < srcLen) {
            memcpy(dst+index, plainBlock, 8);
        } else {
            break;
        }
            
    }

    /* Judge whether the end is filled */
    if (plainBlock[7] < 8) {
        lastLen = 8 - plainBlock[7];
        for (k = lastLen; k < 7; k++) {
            if(plainBlock[k] != '\0') {
                break;
            }
        }
    }

    if (k == 7) {  /* Had Fill */
        memcpy(dst+index, plainBlock, lastLen);
        index += lastLen;
    } else {  /* No fill */
        memcpy(dst+index, plainBlock, 8);
        index += 8;
    }

    *dstLen = index;
    return 0;  
}

// gzCompress: do the compressing  
int gzCompress(const char *src, gzsize_t srcLen, char *dest, gzsize_t destLen)  
{  
    z_stream c_stream;  
    int err = 0;  
    int windowBits = 15;  
    int GZIP_ENCODING = 16;  
  
    if(src && srcLen > 0)  
    {  
        c_stream.zalloc = (alloc_func)0;  
        c_stream.zfree = (free_func)0;  
        c_stream.opaque = (voidpf)0;  
        if(deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,   
                    windowBits | GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY) != Z_OK) return -1;  
        c_stream.next_in  = (Bytef *)src;  
        c_stream.avail_in  = srcLen;  
        c_stream.next_out = (Bytef *)dest;  
        c_stream.avail_out  = destLen;  
        while (c_stream.avail_in != 0 && c_stream.total_out < destLen)   
        {  
            if(deflate(&c_stream, Z_NO_FLUSH) != Z_OK) return -1;  
        }  
            if(c_stream.avail_in != 0) return c_stream.avail_in;  
        for (;;) {  
            if((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END) break;  
            if(err != Z_OK) return -1;  
        }  
        if(deflateEnd(&c_stream) != Z_OK) return -1;  
        return c_stream.total_out;  
    }  
    return -1;  
}  
  
// gzDecompress: do the decompressing  
int gzDecompress(const char *src, gzsize_t srcLen, char *dst, gzsize_t dstLen)
{  
    z_stream strm;  
    strm.zalloc=NULL;  
    strm.zfree=NULL;  
    strm.opaque=NULL;  
       
    strm.avail_in = srcLen;  
    strm.avail_out = dstLen;  
    strm.next_in = (Bytef *)src;  
    strm.next_out = (Bytef *)dst;  
       
    int err=-1, ret=-1;  
    err = inflateInit2(&strm, MAX_WBITS+16);  
    if (err == Z_OK){  
        err = inflate(&strm, Z_FINISH);  
        if (err == Z_STREAM_END){  
            ret = strm.total_out;  
        }  
        else{  
            inflateEnd(&strm);  
            return err;  
        }  
    }  
    else{  
        inflateEnd(&strm);  
        return err;  
    }  
    inflateEnd(&strm);  
    return err;  
}


#ifdef __cplusplus
}
#endif
