#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
示例演示http消费者使用
'''

import cppcloud
import threading

invokerServiceName = 'httpApp1'


def main():
    if not cppcloud.init('www.cppcloud.cn', 4800, svrname='TestHttpInvoker'): 
        return -1

    inker = cppcloud.invokerObject(invokerServiceName)
    inker.setRefreshTimeout(100) # 刷新服务列表时间
    inker.setInvokeTimeout(3) # 调用时默认超时
    
    if inker:
        sendStr = ''
        while "q" != sendStr:
            sendStr = input('input message to send out(input "q" exit):').strip()
            print(("invoker sendding " + sendStr))
            result, rspmsg, errhand = inker.call(invokerServiceName, 'POST',
                data=sendStr)
            print(("Response " + str(result) + "| " + rspmsg))
            if errhand: errhand(result)

    cppcloud.uninit()


if __name__ == "__main__":
    main()