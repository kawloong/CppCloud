#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
示例演示服务消费者使用
'''

import cppcloud
import threading

invokerServiceName = 'TestPrvd'

# 发送调用消息的线程
def sendThread1(msg, inker):
    print(("thread sendding " + msg))
    result, rspmsg, errhand = inker.call(invokerServiceName, msg)
    print(("Response " + str(result) + "| " + rspmsg))
    if errhand: errhand(result)


def main():
    if not cppcloud.init('vpc2', 4800, svrname='InvkerTest'):
        print('CloudApp start fail, exit')
        exit(-1)

    inker = cppcloud.invokerObject(invokerServiceName)
    if inker:
        for i in range(5): # 简单模拟并发多个请求同时进行，验证可以无序且有效地接收响应
            sendStr = "hello" + str(i)
            th = threading.Thread(target=sendThread1, args=(sendStr, inker))
            th.setDaemon(True)
            th.start()

        while "q" != sendStr: # 接收从键盘输入消息并发送给服务方法（开辟个线程进行）
            sendStr = input('input message to send out(input "q" exit):').strip()
            th = threading.Thread(target=sendThread1, args=(sendStr, inker))
            th.setDaemon(True)
            th.start()

    cppcloud.uninit()


if __name__ == "__main__":
    main()