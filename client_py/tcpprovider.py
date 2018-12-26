#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
tcp服务提供者基类

'''

import threading
import SocketServer
import tcpcli
from cloudapp import getCloudApp
from svrstat import svrstat
from const import CMD_TCP_SVR_REQ, CMD_SVRREGISTER_REQ, CMDID_MID


class TcpProviderBase(SocketServer.BaseRequestHandler):
    regname = ''
    host = ''
    port = 0
    protocol = 1
    weight = 100
    enable = 1
    desc = ''
    handleMap = None
    server = None
    thread = None
    prvdid = 0
    async_response = False
    
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

        cls.handleMap = {}
        cls.BindCmdHandle(CMD_TCP_SVR_REQ, cls.onRequest)
        cloudapp.setNotifyCallBack("reconnect_ok", cls.onServReconnect) # 重连成功回调
        cloudapp.setNotifyCallBack("provider", cls.onServReconnect) # 设置weight/enable回调
        SocketServer.ThreadingTCPServer.daemon_threads = True
        cls.server = SocketServer.ThreadingTCPServer((listenHost, cls.port), cls)
        return cls
    
    # cloudapp断开再连接时，应该再次注册服务提供信息
    @classmethod
    def onServReconnect(cls, *param):
        print("Found serv reconnect ok")
        cls.regProvider()

    @classmethod
    def onSetProvider(cls, cmdid, seqid, msgbody):
        if msgbody["regname"] == cls.regname and msgbody["prvdid"] == cls.prvdid:
            if 'enable' in msgbody:
                cls.enable = msgbody['enable']
            if 'weight' in msgbody:
                cls.weight = msgbody['weight']
            cls.regProvider('weight', 'enable')
            return 0, 'update ok'
    
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

        cls.cloudapp.request_nowait(CMD_SVRREGISTER_REQ, {
            "regname": cls.regname,
            "svrprop": svrprop
        })
    
   
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
        print('no implete...\n')

    def close(self):
        setattr(self, 'closeFlag', True)
    

    #def setError(self, err):
    #    self.handleerr = err

    # 返回数据给客户调用者， 在事件处理函数中调用
    def response(self, rspmsg, err = 0):
        asyncmode = hasattr(self, 'async_response_mutex')
        asyncmode and self.async_response_mutex.acquire()
        ret = tcpcli.Send(self.request, 
                self.reqcmdid|CMDID_MID, self.reqseqid, rspmsg)
        asyncmode and self.async_response_mutex.release()

        svrstat.addPrvdCount(self, 0 == err)
        if 0 == ret:
            self.close()
        return ret
    
    # 异步响应，返回的闭包可以在以某个时间某个线程中调用，给予响应
    def response_async(self):
        rspcmdid = self.reqcmdid|CMDID_MID
        rspseqid = self.reqseqid
        sock = self.request
        if not hasattr(self, 'async_response_mutex'):
            self.async_response_mutex = threading.Lock()
            self.async_response = True
        async_response_mutex = self.async_response_mutex

        def respsor(rspmsg, err = 0):
            try:
                async_response_mutex.acquire()
                tcpcli.Send(sock, rspcmdid, rspseqid, rspmsg)
                svrstat.addPrvdCount(self, 0 == err)
            finally:
                async_response_mutex.release()

        return respsor


    ## override
    def handle(self):
        print("begin handle")
        threading.currentThread().setName('handle:' + str(self.request.getpeername()))
        while not getattr(self, 'closeFlag', False):
            result,recvBytes, cmdid,seqid,body = tcpcli.Recv(self.request, False)
            if 0 == result:
                self.recvBytes = recvBytes
                self.reqcmdid = cmdid
                self.reqseqid = seqid
                self.reqbody = body
                self.handleerr = 0
                func = self.handleMap.get(cmdid)
                if func:
                    '''
                    def respsor(msg, err = 0):
                        rspid = cmdid|CMDID_MID
                        rspseqid = seqid
                        if 0 == tcpcli.Send(self.request, rspid, rspseqid, msg):
                            self.close()
                        svrstat.addPrvdCount(self, 0 == err)
                    func(self, respsor)
                    '''
                    func(self)
                else:
                    print("No CmdFunc cmd=0x%x(%d)" % (cmdid, seqid))
                    self.handleerr = 1
                
            else:
                self.close()
    


 
