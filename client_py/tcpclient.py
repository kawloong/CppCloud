#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
提供连接cppclud_serv后，协调发送线程和接收线程，封装出同步调用方法收发tcp报文
'''


import threading
import subprocess
import time
import json
from const import *
from fileop import str2file
from tcpcli import TcpCliBase
from Queue import Queue #LILO队列

class TcpClient(TcpCliBase):
    def __init__(self, svraddr, **kvarg):
        super(TcpClient, self).__init__(svraddr) # 调用基类初始化
        self.reqTimeOutSec = 5
        self.cmd2doFun = {} # cmdid -> handle-fun

        for kv in kvarg:
            setattr(self, kv, kvarg[kv])
        #svraddr = getattr(self, 'scomm_sevr_addr', cfg.scomm_sevr_addr)
        self.sndQ = Queue()
        self.rcvQ = Queue()
        self.waitRspQMap = {} # 同步等待某个响应队列
        self.seqid = 100
        self.running = False
        self.bexit = False
        self.mconf = None

    def _sendLoop(self):
        while not self.bexit:
            itemsnd = self.sndQ.get()
            if (0 == itemsnd):
                return 0
            ret = self.sndMsg(*itemsnd)
            print('*%s* Send ret=%d| cmd=0x%x| %s'% (time.ctime(), ret, itemsnd[0], itemsnd))

    def _recvLoop(self):
        while not self.bexit:
            rspcmd, seqid, rspmsg = self.rcvMsg(False)
            print('*%s* Recv cmd=0x%x(%d)| seq=%s'% (time.ctime(), rspcmd, rspcmd, seqid))
            cQ = None
            if CMD_WHOAMI_RSP == rspcmd:
                rspmsg = json.loads(rspmsg)
                self.svrid = rspmsg["svrid"]
                self.mconf = rspmsg.get('mconf', '')
                print('svrid setto %d, mconf=%s'% (self.svrid, self.mconf))
                continue
            if 0 == rspcmd: # 断开,重连
                if self.checkConn() <=0:
                    time.sleep(5)
                else:
                    self.sndQ.put( self.whoami_str() )
                continue

            if rspcmd > CMDID_MID:
                cQ = self.waitRspQMap.pop(seqid)
            elif CMD_EXCHANG_REQ == rspcmd:
                self._doCMD_EXCHANG_REQ(rspcmd, seqid, rspmsg)
                continue
            elif CMD_KEEPALIVE_REQ == rspcmd:
                self.sndQ.put( (CMD_KEEPALIVE_RSP, seqid, '') )
                continue
            else:
                pass
            if cQ:
                if isinstance(cQ, Queue): # 同步方式
                    cQ.put(rspmsg)
                else: # 异常方式回调
                    cQ(rspcmd, seqid, rspmsg)
            else:
                cmdhandfunc = self.cmd2doFun.get(rspcmd, self._doUnknowCMD)
                rsptuple = cmdhandfunc(rspcmd, seqid, rspmsg)
                if rsptuple:
                    self.sndQ.put(rsptuple)

    def _doUnknowCMD(self, cmdid, seqid, msgbody):
        print('recv Undefine Cmd=0x%x,msgbody=%s' % (cmdid, msgbody))
        if cmdid < CMDID_MID: # discard or not?
            return (cmdid|CMDID_MID, seqid, {'code': 404, 'desc': 'unknow command'})

    def _doCMD_EXCHANG_REQ(self, cmdid, seqid, msgbody):
        msgbody = json.loads(msgbody)
        if self.svrid != msgbody.get('to', 0):
            print('unmatch svrid=%d msg recv' % msgbody.get('to', 0))
            return
        command = msgbody.get('command', '')
        funccmd = getattr(self, 'on_'+command, None)
        if callable(funccmd):
            fromsvrid = msgbody.get('from', 0)
            ret,desc = funccmd(cmdid, seqid, msgbody)
            self.sndQ.put( (
                cmdid|CMDID_MID, seqid, {
                'code': ret, 
                'to': fromsvrid,
                'from': self.svrid,
                'desc': desc}
                ) )
        else:
            print('unknow command=%x'%cmdid)
            self.sndQ.put( (cmdid|CMDID_MID, seqid, {
                'to': msgbody.get('from'), 'from': self.svrid, 'code': 410, 'desc': 'unknow command'}) )
    
    # param: func is def function_name(cmd, seqid, msg)
    def setCmdHandle(self, cmdid, func):
        self.cmd2doFun[cmdid] = func;

    def tell_whoami(self): # 覆盖基类，无操作
        pass
    # 发出请求，并等待回应
    def request(self, cmdid, reqbody):
        self.seqid += 1
        seqid = self.seqid
        waitq = Queue(1)
        self.waitRspQMap[seqid] = waitq
        self.sndQ.put( (cmdid, seqid, reqbody) )
        resp = None
        try:
            resp = waitq.get(timeout=self.reqTimeOutSec) # 
        except:
            print('req 0x%x timeout (body=%s)' % (cmdid, reqbody))
        #self.waitRspQMap.pop(seqid)
        return resp
    
    def _dumresp(self, rspcmd, seqid, rspmsg):
        print(rspcmd, seqid, rspmsg)
    def request_nowait(self, cmdid, reqboby, callback=None):
        self.seqid += 1
        seqid = self.seqid
        #waitq = Queue(1)
        self.waitRspQMap[seqid] = callback # if callback else self._dumresp
        self.sndQ.put( (cmdid, seqid, reqboby) )
    def post_msg(self, cmdid, seqid, reqbody):
        self.sndQ.put( (cmdid, seqid, reqbody) )
    
    def run(self):
        if not self.running:
            if self.checkConn() <= 0:
                return False
            self.trecvthread = threading.Thread(target=self._recvLoop, name='RecvThread')
            self.trecvthread.start()
            self.tsendthread = threading.Thread(target=self._sendLoop, name='SendThread')
            self.tsendthread.start()

            self.checkConn()
            self.sndQ.put( self.whoami_str() )
            self.running = True
        return True

    def isAliveWait(self, timeout_sec):
        self.trecvthread.join(timeout_sec)
        return self.trecvthread.isAlive()

    def join(self):
        if self.running:
            self.trecvthread.join()
            self.tsendthread.join()
            self.running = False
            self.close()
    
    
    # 下载更新文件
    def on_download(self, cmdid, seqid, msgbody):
        filename = msgbody['filename']
        context = msgbody['context']
        ret = str2file(context, filename, True)
        print('on_download result ', ret, 'filename is ', filename)

        # 下载完成后的其他动作
        desc = 'fail' if 0 != ret else  'savefile ok'
        return ret, desc


    def on_checkalive(self, cmdid, seqid, msgbody):
        nowstr = time.strftime('%Y-%m-%d %H:%M:%S',time.localtime(time.time()))
        return 0, nowstr


if __name__ == '__main__':
    obj2 = TcpClient(
        ('192.168.228.44', 4800),
        clitype = 22,
        progName='progName',
        desc='123')
    obj2.run()
    obj2.join()
    print 'prog exit'
