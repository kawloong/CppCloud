#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
服务消费者客户端连接模块
'''

import urlparse
import tcpcli
from const import CMD_TCP_SVR_REQ

gIvkClis = {}

class InvokerCli(object):
    def good(self): return False
    def call(self, *param, **arg): return None, None
    def close(self): pass

class TcpInvokerCli(InvokerCli):
    seqid = 1

    def __init__(self, regObj):
        self.url = regObj["url"]
        hostp = urlparse.urlparse(self.url).netloc
        self.host, self.port = hostp.split(':')
        self.clisock = tcpcli.Connect(self.host, self.port)
    
    def good(self):
        return (None != self.clisock)

    def call(self, reqmsg, cmdid = CMD_TCP_SVR_REQ, todict = False):
        TcpInvokerCli.seqid += 1
        seqid = TcpInvokerCli.seqid
        sndbytes = tcpcli.Send(self.clisock, cmdid, seqid, reqmsg)
        if 0 == sndbytes: return -1, ''
        
        result,recvBytes,rspid,rspseq,body = tcpcli.Recv(self.clisock, todict)
        if int(rspseq) != seqid:
            print("Warn: seqid not match %d/%d" % (seqid, rspseq))
        return result, body
    
    def close(self):
        if self.clisock:
            self.clisock.close()
            self.clisock = None

class HttpInvokerCli(InvokerCli):
    def __init__(self, regObj):
        pass

protocol2Class = {1: TcpInvokerCli, 3: HttpInvokerCli}

def call(regObj, *param, **arg):
    ivkKey = (regObj["regname"], regObj["svrid"], regObj["prvdid"])
    ivkcli = gIvkClis.get(ivkKey)

    if not ivkcli: # 创建客户端对象
        cls = protocol2Class.get(regObj['protocol'])
        if not cls:
            print("Not implete protocol ", regObj['protocol'])
            return -1, ''
        ivkcli = cls(regObj)
        if not ivkcli.good(): # 测试是否可用
            return -2, ''
        
        gIvkClis[ivkKey] = ivkcli
    
    return ivkcli.call(*param, **arg)

def remove(regObj):
    ivkKey = (regObj["regname"], regObj["svrid"], regObj["prvdid"])
    if ivkKey in gIvkClis:
        gIvkClis[ivkKey].close()
        del gIvkClis[ivkKey]



if __name__ == "__main__":
    testFun("param1", "pa222")