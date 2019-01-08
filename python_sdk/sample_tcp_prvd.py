#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
示例演示tcp服务提供者操作
'''

import cppcloud
from cppcloud.tcpprovider import TcpProviderBase

class PrvdTcp(TcpProviderBase):
    #regname = 'TestPrvd' # 不设置，默认用cloudapp.svrname
    #host = '192.168.1.101'  # 不设置，默认自动检测
    #port = 3744 # 不设置，默认用随机端口


    # 默认消息消息处理方法()
    # overload
    def onRequest(self):
        import threading
        print((".. do job ..", self.reqcmdid, self.reqseqid, self.reqbody))
        respsor = self.response_async()
        self.seqid = 0
        self.reqcmdid = 0
        
        if not getattr(self, 'timer', None): # 令首个请求延迟返回（模拟一些耗时业务请求）
            self.timer = threading.Timer(2, self.threadTest, (respsor,))
            self.timer.setDaemon(True)
            self.timer.start()
        else:
            respsor("hi, input=" + self.reqbody)
    
    def threadTest(self, respsor):
        print("a-resp: hi, input=inthread send")
        respsor("hi, input=inthread send")

class PrvdTcp2(TcpProviderBase):
    regname = 'pp2'

if __name__ == "__main__":
    if not cppcloud.init('vpc2', 4800, svrname="TestPrvd"):
        print('CloudApp start fail, exit')
        exit(-1)

    PrvdTcp.Start()
    PrvdTcp2.Start()
    input('press any key to exit\n')
    PrvdTcp2.Shutdown()
    PrvdTcp.Shutdown()

    cppcloud.uninit()
