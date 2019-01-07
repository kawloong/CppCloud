#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
示例演示http消费者使用
'''

from cloudapp import CloudApp
from cloudinvoker import CloudInvoker
import threading

invokerServiceName = 'THttpPrvd'


def main():
    cloudapp = CloudApp('vpc2', 4800, svrname='InvkerTest')
    if not cloudapp.start(): return -1

    inker = CloudInvoker()
    if inker.init(invokerServiceName) == 0:
        sendStr = ''
        while "q" != sendStr:
            sendStr = raw_input('input message to send out(input "q" exit):').strip()
            print("invoker sendding " + sendStr)
            result, rspmsg, errhand = inker.call(invokerServiceName, 'POST',
                data=sendStr)
            print("Response " + str(result) + "| " + rspmsg)
            if errhand: errhand(result)

    cloudapp.shutdown()
    cloudapp.join()


if __name__ == "__main__":
    main()