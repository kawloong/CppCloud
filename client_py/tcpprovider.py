#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
tcp服务提供者基类

'''

import threading
import SocketServer
import tcpcli
from cloudapp import getCloudApp
from const import CMD_TCP_SVR_REQ, CMD_SVRREGISTER_REQ, CMDID_MID


class TcpProviderBase(SocketServer.BaseRequestHandler):
    regname = ''
    host = ''
    port = 0
    protocol = 1
    weight = 100
    enable = 1
    desc = ''
    handleMap = {}
    server = None
    thread = None
    prvdid = 0
    
    # 绑定消息处理函数
    @classmethod
    def BindCmdHandle(cls, cmdid, func):
        cls.handleMap[cmdid] = func
    
    # 服务提供者构造器(对外)
    # 调用前请确保已初始化CloudApp实例
    @classmethod
    def Create(cls):
        cloudapp = getCloudApp()
        cls.cloudapp = cloudapp
        if not cloudapp:
            print("Error: cloudapp not init")
            return None
        listenHost = cls.host

        if not cls.regname:
            cls.regname = cloudapp.svrname
        
        cls.prvdid = TcpProviderBase.prvdid
        TcpProviderBase.prvdid += 1
        if 0 == cls.port:
            cls.port = 2000 + cls.prvdid

        if not cls.host:
            listenHost = cloudapp.cliIp
            cls.url = 'tcp://' + listenHost + ':' + str(cls.port)
        else:
            cls.url = 'tcp://' + cls.host + ':' + str(cls.port)
        
        cls.BindCmdHandle(CMD_TCP_SVR_REQ, cls.onRequest)
        SocketServer.ThreadingTCPServer.daemon_threads = True
        cls.server = SocketServer.ThreadingTCPServer((listenHost, cls.port), cls)
        return cls
    
    # 参数：prop如果为空，则发送所有
    @classmethod
    def regProvider(cls, *prop):
        if not prop:
            prop = ("prvdid", "url", "desc", "protocol", "weight", "enable", "idc", "rack")
        svrprop = {}
        for key in prop:
            val = getattr(cls, key, None)
            if None != val:
                svrprop[key] = val

        regResult = cls.cloudapp.request(CMD_SVRREGISTER_REQ, {
            "regname": cls.regname,
            "svrprop": svrprop
        })
        print(regResult)
    
        

    @classmethod
    def llop(cls):
        cls.server.serve_forever()
        print("llop exited")
    
    @classmethod
    def Start(cls):
        cls.thread = threading.Thread(target=cls.server.serve_forever, 
                name='PrvdServer:'+cls.regname)
        #cls.thread.setDaemon(True)
        cls.thread.start()
        cls.regProvider()

    @classmethod
    def Shutdown(cls):
        print("Shutdown Provider " + cls.url)
        tcpcli.TellExit()
        cls.server.shutdown()
        cls.server.server_close()
        cls.server = None
        cls.thread.join() # daemon 可不join
        print("Shutdown Provider join" )
        cls.thread = None

    ## override // 默认的tcp微服务请求处理方法（即请求cmdid=CMD_TCP_SVR_REQ时的回调）
    def onRequest(self):
        print('oncall...\n')
        pass

    def close(self):
        setattr(self, 'closeFlag', True)
    
    def response(self, rspmsg):
        ret = tcpcli.Send(self.request, 
                self.reqcmdid|CMDID_MID, self.reqseqid, rspmsg)
        if 0 == ret:
            self.close()
        return ret

    ## override
    def handle(self):
        print("begin handle")
        threading.currentThread().setName('handle:' + str(self.request.getpeername()))
        while not getattr(self, 'closeFlag', False):
            result,recvBytes, cmdid,seqid,body = tcpcli.Recv(self.request, True)
            if 0 == result:
                self.recvBytes = recvBytes
                self.reqcmdid = cmdid
                self.reqseqid = seqid
                self.reqbody = body
                func = self.handleMap.get(cmdid)
                if func:
                    func(self)
                else:
                    print("No CmdFunc cmd=0x%x(%d)" % (cmdid, seqid))
            else:
                self.close()
    

class PrvdTcp(TcpProviderBase):
    regname = 'prvd1'
    host = '192.168.1.101'
    #port = 3744

    def onCustom(self):
        print("do job .")

if __name__ == "__main__":
    prvd = PrvdTcp.Create()
    prvd.BindCmdHandle(1, PrvdTcp.onCustom)
    import time

    prvd.Start()
    time.sleep(10)
    prvd.Shutdown()
 
