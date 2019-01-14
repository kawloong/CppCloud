#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
与cppcloud_serv通信模块,封tcp报文收发
用法比较简单，可参见文件末的__main__处.
'''

import os
import sys
import time
import socket
import struct
import json
from .const import CMD_WHOAMI_REQ,CMD_WHOAMI_RSP,CMD_GETCLI_REQ
#from cliconfig import config as cfg

g_version = 1
g_headlen = 10


def Connect(host, port):
    return TcpCliBase.Connect(host, port)
def Recv(clisock, tomap):
    return TcpCliBase.Recv(clisock, tomap)
def Send(clisock, cmdid, seqid, body):
    return TcpCliBase.Send(clisock, cmdid, seqid, body)
def TellExit():
    TcpCliBase.TellExit()

class TcpCliBase(object):
    exit_flag = 0

    def __init__(self, svraddr):
        self.svraddr = svraddr
        self.cliIp = ''
        self.cli = None
        self.svrid = 0 # 这个是服务端为本连接分配置的ID, 由whoami请求获得
        self.clitype = 200 # 默认普通py应用
        self.step = 0
        self.svrname = "unknow"
        self.desc = ""
        self.tag = ""
        self.aliasname = ""
        # 统计信息
        self.send_bytes = 0
        self.recv_bytes = 0
        self.send_pkgn = 0
        self.recv_pkgn = 0
    
    @staticmethod
    def Connect(host, port):
        ret = None
        try:
            port = int(port)
            cli = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            cli.connect((host, port))
            ret = cli
        except socket.error as e:
            print(('connect to %s:%d fail: %s'%(host, port, e)))
            cli.close()
        return ret
    
    # return 大于0正常; 0失败
    def checkConn(self):
        if self.step <= 0:
            clisock = TcpCliBase.Connect(self.svraddr[0], self.svraddr[1])
            if not clisock: return 0

            self.cli = clisock
            print(('connected tcp to ' + str(self.svraddr) ))
            self.cliIp = self.cli.getsockname()[0]
            self.step = 1
            self.tell_whoami()
        return self.step
    
    @staticmethod
    def TellExit():
        TcpCliBase.exit_flag = 1

    # 发送包，可对外提供接口
    @staticmethod
    def Send(clisock, cmdid, seqid, body):
        if isinstance(body, dict) or isinstance(body, list):
            body = json.dumps(body)
        bodylen = len(body)
        head = struct.pack("!bbIHH", g_version, 
            g_headlen, bodylen, cmdid, seqid)
        head += body.encode('utf-8')

        try:
            # print('sndMsg bodylen=',bodylen)
            retsnd = clisock.send(head)
            return retsnd
        except socket.error as e:
            print(('except happen ', e))
            return 0

    # return 大于0成功，0失败
    def sndMsg(self, cmdid, seqid, body):
        if self.checkConn() <= 0:
            return 0

        retsnd = self.Send(self.cli, cmdid, seqid, body)
        if retsnd <= 0:
            self.cli.close()
            self.cli = None
        else:
            self.send_pkgn += 1
            self.send_bytes += retsnd

        return retsnd

    @staticmethod
    def Recv(clisock, tomap):
        recvBytes = 0;
        try:
            # 接收头部
            headstr = b''
            while len(headstr) < g_headlen: # 在慢网环境
                recvmsg = clisock.recv(g_headlen - len(headstr))
                if TcpCliBase.exit_flag:
                    return -1,0, 0,0,'programe exit'

                headstr += recvmsg
                # 当收到空时,可能tcp连接已断开
                if len(recvmsg) == 0:
                    return -10,0, 0,0,'recv 0 peerclose'
                else:
                    recvBytes += len(recvmsg)
            
            ver, headlen, bodylen, cmdid, seqid = struct.unpack("!bbIHH", headstr)
            if ver != g_version or headlen != g_headlen or bodylen > 1024*1024*5:
                print(("Recv Large Package| ver=%d headlen=%d bodylen=%d cmdid=0x%X"%(
                        ver, headlen, bodylen, cmdid)))


            body = b''
            while len(body) < bodylen:
                body += clisock.recv(bodylen)
                if TcpCliBase.exit_flag:
                    return -1,0, 0,0,'programe exit'
            recvBytes += len(body)
            body = body.decode('utf-8')

        except socket.error as e:
            print(('except happen ', e))
            return -11,0, 0, 0, ('sockerr %s' % e)
        
        if (tomap):
            body = json.loads(body)
        
        return 0,recvBytes, cmdid,seqid,body
    
    # 失败第1个参数是0;
    def rcvMsg(self, tomap=True):
        if self.step <= 0:
            self.checkConn()
            return 0, 0, ''
        
        result,rbytes, cmdid,seqid,body = self.Recv(self.cli, tomap)
        if 0 == result:
            self.recv_pkgn += 1
            self.recv_bytes += rbytes
        else:
            self.close()
            cmdid = 0

        return cmdid,seqid,body

    def close(self):
        if self.cli:
            self.cli.close()
            self.cli = None
        self.step = 0 # closed
    
    def shutdownWrite(self):
        if self.cli: self.cli.shutdown(socket.SHUT_WR)

    # return 大于0正常
    def tell_whoami(self):
        ret = self.sndMsg(*self.whoami_str())
        if ret > 0:
            ret, seqid, rsp = self.rcvMsg()
            if ret == CMD_WHOAMI_RSP:
                self.svrid = rsp["svrid"]
                print(('svrid setto %d'%self.svrid))
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
            "svrname": self.svrname,
            "desc": self.desc,
            "clitype": self.clitype,
            "tag": self.tag,
            "aliasname": self.aliasname,
            "begin_time": int(time.time()),
            "shell": shellstr
        }
        return CMD_WHOAMI_REQ, seqid, rqbody

if __name__ == '__main__':
    scomm_sevr_addr = ("192.168.228.44", 4800)
    cliobj = TcpCliBase(scomm_sevr_addr, 20)
    
    sndret = cliobj.sndMsg(CMD_GETCLI_REQ, 1, {})
    print(("sndret=%d" % sndret))

    rspcmd, seqid, rspmsg = cliobj.rcvMsg(False)
    print(("response cmd=0x%X" % rspcmd))
    print(rspmsg)
    cliobj.close()

    print("main exit 0")
    