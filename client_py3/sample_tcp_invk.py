#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
示例演示服务消费者使用
'''

from .cloudapp import CloudApp
from .cloudinvoker import CloudInvoker
import threading

invokerServiceName = 'TestPrvd'


def sendThread1(cloudapp, msg, inker):
    print(("thread sendding " + msg))
    result, rspmsg, errhand = inker.call(invokerServiceName, msg)
    print(("Response " + str(result) + "| " + rspmsg))
    if errhand: errhand(result)


def main():
    cloudapp = CloudApp('vpc2', 4800, svrname='InvkerTest')
    if not cloudapp.start(): return -1

    inker = CloudInvoker()
    if inker.init(invokerServiceName) == 0:

        for i in range(5):
            sendStr = "hello" + str(i)
            th = threading.Thread(target=sendThread1, args=(cloudapp, sendStr, inker))
            th.setDaemon(True)
            th.start()

        while "q" != sendStr:
            sendStr = input('input message to send out(input "q" exit):').strip()
            th = threading.Thread(target=sendThread1, args=(cloudapp, sendStr, inker))
            th.setDaemon(True)
            th.start()

    cloudapp.shutdown()
    cloudapp.join()


if __name__ == "__main__":
    main()