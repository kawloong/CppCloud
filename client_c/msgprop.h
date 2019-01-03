/**
 * Filename: msgprop.h
*/
#ifndef _MSGPROP_H_
#define _MSGPROP_H_

struct msg_prop_t
{
    unsigned cmdid;
    unsigned seqid;
    void* iohand;

    msg_prop_t(): cmdid(0), seqid(0), iohand(0){}
    msg_prop_t(unsigned c, unsigned s, void* i): 
        cmdid(c), seqid(s), iohand(i){}
};

#endif