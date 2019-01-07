#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
示例演示服务提供者操作
'''

from .cloudapp import CloudApp
from .tcpprovider import TcpProviderBase


class PrvdTcp(TcpProviderBase):
    #regname = 'TestPrvd' # 不设置，默认用cloudapp.svrname
    #host = '192.168.1.101'  # 不设置，默认自动检测
    #port = 3744 # 不设置，默认用随机端口


    # 默认消息消息处理方法()
    def onRequest(self):
        import threading
        print((".. do job ..", self.reqcmdid, self.reqseqid, self.reqbody))
        respsor = self.response_async()
        self.seqid = 0
        self.reqcmdid = 0
        
        if not getattr(self, 'timer', None):
            self.timer = threading.Timer(6, self.threadTest, (respsor,))
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
    cloudapp = CloudApp('vpc2', 4800, svrname="TestPrvd")
    if cloudapp.start():
        PrvdTcp.Create()
        PrvdTcp.Start()
        PrvdTcp2.Create()
        PrvdTcp2.Start()

        eval(input('press any key to exit'))
        
        PrvdTcp2.Shutdown()
        PrvdTcp.Shutdown()
        cloudapp.shutdown()
        cloudapp.join()
    else:
        print('CloudApp start fail, exit')
