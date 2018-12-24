#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
示例演示服务提供者操作
'''

from cloudapp import CloudApp
from tcpprovider import TcpProviderBase


class PrvdTcp(TcpProviderBase):
    #regname = 'prvd1' # 不设置，默认用cloudapp.svrname
    #host = '192.168.1.101'  # 不设置，默认自动检测
    #port = 3744 # 不设置，默认用随机端口

    # 默认消息消息处理方法()
    def onRequest(self):
        print(".. do job ..")
        self.response("hi, input=" + self.reqbody)

class PrvdTcp2(TcpProviderBase):
    regname = 'pp2'

if __name__ == "__main__":
    cloudapp = CloudApp('192.168.228.44', 4800, svrname="prvd1")
    if cloudapp.start():
        PrvdTcp.Create()
        PrvdTcp.Start()
        PrvdTcp2.Create()
        PrvdTcp2.Start()

        raw_input('press any key to exit')
        
        PrvdTcp2.Shutdown()
        PrvdTcp.Shutdown()
        cloudapp.shutdown()
        cloudapp.join()
    else:
        print('CloudApp start fail, exit')
