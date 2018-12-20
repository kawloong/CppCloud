#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
与scomm_serv通信模块,封tcp报文收发
'''

import os
import sys
import time
import socket
import struct
import json
from const import *
#from cliconfig import config as cfg

scomm_sevr_addr = ("192.168.1.68", 4800)
g_version = 1
g_headlen = 10


class ScommCli(object):
    def __init__(self, svraddr, clitype):
        self.svraddr = svraddr
        self.cli = None
        self.svrid = 0 # 这个是服务端为本连接分配置的ID, 由whoami请求获得
        self.clitype = clitype
        self.step = 0
        self.progName = ""
        self.progDesc = ""
        self.tag = ""
    
    
    # return 大于0正常; 0失败
    def checkConn(self):
        if self.step <= 0:
            try:
                self.cli = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                ret = self.cli.connect(self.svraddr)
                print('connect tcp to %s, result %s'% (self.svraddr,ret) )
            except socket.error, e:
                print('connect %s fail: %s'%(str(self.svraddr),e))
                return 0
            
            self.step = 1
            self.tell_whoami()
        return self.step

    # return 大于0成功，0失败
    def sndMsg(self, cmdid, seqid, body):
        if isinstance(body, dict) or isinstance(body, list):
            body = json.dumps(body)
        bodylen = len(body)
        head = struct.pack("!bbIHH", g_version, 
            g_headlen, bodylen, cmdid, seqid)
        head += body
        
        if self.checkConn() <= 0:
            return 0

        try:
            # print('sndMsg bodylen=',bodylen)
            return self.cli.send(head)
        except socket.error, e:
            print('except happen ', e)
            self.cli.close()
            self.cli = None
            self.step = 0
            return 0


    # 失败第1个参数是0;
    def rcvMsg(self, tomap=True):
        if self.step <= 0:
            self.checkConn()
            return 0, 0, ''

        try:
            headstr = '' # self.cli.recv(g_headlen)
            while len(headstr) < g_headlen: # 在慢网环境
                recvmsg = self.cli.recv(g_headlen - len(headstr))
                headstr += recvmsg
                # 当收到空时,可能tcp连接已断开
                if len(recvmsg) <= 0:
                    self.close();
                    return 0,0,0                    
            
            ver, headlen, bodylen, cmdid, seqid = struct.unpack("!bbIHH", headstr)
            if ver != g_version or headlen != g_headlen or bodylen > 1024*1024*5:
                print("Recv Large Package| ver=%d headlen=%d bodylen=%d cmdid=0x%X"%(ver, headlen, bodylen, cmdid))

            body = ''
            while len(body) < bodylen:
                body += self.cli.recv(bodylen)

        except socket.error, e:
            print('except happen ', e)
            self.close()
            return 0, 0, ('sockerr %s' % e)
        
        if (tomap):
            #print 'len:', bodylen, 'recv body:', body
            body = json.loads(body)
        return cmdid,seqid,body

    def close(self):
        if self.cli:
            self.cli.close()
            self.cli = None
        self.step = 0 # closed

    # return 大于0正常
    def tell_whoami(self):
        ret = self.sndMsg(*self.whoami_str())
        if ret > 0:
            ret, seqid, rsp = self.rcvMsg()
            if ret == CMD_WHOAMI_RSP:
                self.svrid = rsp["svrid"]
                print('svrid setto %d'%self.svrid)
                return ret
        return -1
    
    def whoami_str(self, seqid=1):
        hhh, ppp = self.cli.getsockname()
        shellstr = ' '.join(sys.argv)
        shellstr = shellstr.replace('\\', '/')
        rqbody = {
            "localsock": hhh+":"+str(ppp),
            "svrid": self.svrid,
            "pid": os.getpid(),
            "svrname": self.progName,
            "desc": self.progDesc,
            "clitype": self.clitype,
            "tag": self.tag,
            "begin_time": int(time.time()),
            "shell": shellstr
        }
        return CMD_WHOAMI_REQ, seqid, rqbody

if __name__ == '__main__':
    cliobj = ScommCli(scomm_sevr_addr, 20)
    
    sndret = cliobj.sndMsg(CMD_GETCLI_REQ, 1, {})
    print("sndret=%d" % sndret)

    rspcmd, seqid, rspmsg = cliobj.rcvMsg(False)
    print("response cmd=0x%X" % rspcmd)
    print(rspmsg)
    cliobj.close()

    print("main exit 0")
    