#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
服务消费者客户端连接模块
'''

import urllib.parse
from . import tcpcli
import threading
import time
import requests
from socket import SHUT_WR
from queue import Queue #LILO队列 #from threading import Queue
from .const import CMD_TCP_SVR_REQ

gIvkClis = {}
reqwait_timeout_sec = 4

class InvokerCli(object):
    global reqwait_timeout_sec
    def good(self): return False
    def call(self, *param, **arg): return None, None
    def close(self): pass

class TcpInvokerCli(InvokerCli):
    seqid = 1

    def __init__(self, regObj):
        self.url = regObj["url"]
        hostp = urllib.parse.urlparse(self.url).netloc
        self.host, self.port = hostp.split(':')
        self.clisock = tcpcli.Connect(self.host, self.port)
        self.sndMutex = threading.Lock()
        self.waitRspMap = {}
        self.sendReqQueue = Queue()
        self.exit = False
        self.rcvThread = threading.Thread(target=self._rcvThread, name="rcv:"+self.url)
        self.rcvThread.start()
    
    def good(self):
        return (None != self.clisock)
    
    def _rcvThread(self):
        while not self.exit:
            result,recvBytes,rspid,rspseq,body = tcpcli.Recv(self.clisock, False)
            if 0 != result:
                print(("tcpcli.Recv %d  %s" % (result, body)))
                break
            waitq = self.waitRspMap.get(rspseq)
            if waitq:
                waitq.put((result,recvBytes,rspid,rspseq,body))
                del self.waitRspMap[rspseq]
            else:
                print(("maybe %s respnese too late cmd=0x%x seq=%s" % (self.url, int(rspid), rspseq)))

    def call(self, reqmsg, cmdid = CMD_TCP_SVR_REQ, todict = False):
        rspqw = Queue(1)
        self.sndMutex.acquire()

        TcpInvokerCli.seqid += 1
        seqid = TcpInvokerCli.seqid
        while seqid in self.waitRspMap:
            seqid += 1
            TcpInvokerCli.seqid += 1
        self.waitRspMap[seqid] = rspqw
        
        sndbytes = tcpcli.Send(self.clisock, cmdid, seqid, reqmsg)
        self.sndMutex.release()
        if 0 == sndbytes:
             del self.waitRspMap[seqid]
             return -1, ''
        
        try:
            step = "send"
            self.sendReqQueue.put((reqmsg, cmdid, todict), True, reqwait_timeout_sec)
            step = "recv"
            ret = rspqw.get(True, reqwait_timeout_sec)
            result,recvBytes,rspid,rspseq,body = ret
        except:
            result, body = (1, step + ' timeout to ' + self.url)
        
        if seqid in self.waitRspMap:
            del self.waitRspMap[seqid]

        return result, body
    
    def close(self):
        self.exit = True
        if self.clisock:
            self.clisock.shutdown(SHUT_WR)
            self.clisock.close()
            self.clisock = None
        self.rcvThread.join()
        self.rcvThread = None

class HttpInvokerCli(InvokerCli):
    def __init__(self, regObj):
        self.url = regObj["url"]
        self.error = True
    
    def good(self):
        return self.error
    
    def call(self, method='GET', **kwargs): #kwargs=[params, timeout, method, headers, cookies, files]
        if not 'timeout' in kwargs:
            kwargs['timeout'] = reqwait_timeout_sec
        ret, text = -3, 'except'
        try:
            rsp = requests.request(method, self.url, **kwargs)
            ret = 0 if rsp.status_code == 200 else rsp.status_code
            text = rsp.text
        except Exception as e:
            print(("request Exception", e))

        return ret, text
        

protocol2Class = {1: TcpInvokerCli, 3: HttpInvokerCli}
creatLocker = threading.Lock()

def call(regObj, *param, **arg):
    ivkKey = (regObj["regname"], regObj["svrid"], regObj["prvdid"])
    ivkcli = gIvkClis.get(ivkKey)

    if not ivkcli:
        try: # 创建客户端对象
            creatLocker.acquire()
            ivkcli = gIvkClis.get(ivkKey)
            if not ivkcli:
                cls = protocol2Class.get(regObj['protocol'])
                if not cls:
                    print(("Not implete protocol ", regObj['protocol']))
                    return -1, ''
                ivkcli = cls(regObj)
                if not ivkcli.good(): # 测试是否可用
                    ivkcli.close()
                    return -2, ''
            
                gIvkClis[ivkKey] = ivkcli
        finally:
            creatLocker.release()
    
    return ivkcli.call(*param, **arg)

def remove(regObj):
    ivkKey = (regObj["regname"], regObj["svrid"], regObj["prvdid"])
    creatLocker.acquire()
    if ivkKey in gIvkClis:
        gIvkClis[ivkKey].close()
        del gIvkClis[ivkKey]
    creatLocker.release()


