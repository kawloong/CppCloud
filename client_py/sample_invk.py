#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
示例演示服务消费者使用
'''

from cloudapp import CloudApp
from cloudinvoker import CloudInvoker

invokerServiceName = 'prvd1'


def main():
    cloudapp = CloudApp('vpc2', 4800, svrname='InvkerTest')
    if not cloudapp.start(): return -1

    inker = CloudInvoker()
    if inker.init(invokerServiceName) > 0: return -2

    sendStr = "hello"
    while "q" != sendStr:
        result, rspmsg, errhand = inker.call(invokerServiceName, sendStr)
        print("Response " + str(result) + "| " + rspmsg)
        sendStr = raw_input('input message to send out(input "q" exit):').strip()

        # 上报执行结果给统计模块
        if errhand: errhand(0 == result)
    
    cloudapp.shutdown()
    cloudapp.join()


if __name__ == "__main__":
    main()