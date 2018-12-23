#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
服务消费者模块
'''

import json
import random
import invokercli
from const import CMD_SVRSEARCH_REQ
from cloudapp import getCloudApp

class CloudInvoker:
    def __init__(self):
        self.cloudapp = getCloudApp()
        self.invkers = {}
    

    # 初始化将所有需要消费的服务名传进来，
    # 成功返回0， 失败返回服务发现时失败服务个数
    def init(self, *regNameList):
        ng = 0
        for regName in regNameList:
            resp = self.cloudapp.request(CMD_SVRSEARCH_REQ, {
                "regname" : regName,
                "bookchange" : 1
            })
            resp = json.loads(resp)
            resplist = resp.get("data")
            if 0 == resp["code"] and resplist:
                weightSum = 0
                for item in resplist:
                    weightSum += item.get("weight", 0)
                self.invkers[regName] = {
                    "weightSum": weightSum,
                    "reglist": list(resplist)}
            else:
                print("No Service=" + regName)
                print(resp)
                ng += 1
        return ng
    
    # 消费者接口调用
    # return (callresult, response)
    def call(self, regname, *param, **arg):
        ivkObj = self.invkers.get(regname)
        if not ivkObj:
            return -1, 'not service=' + regname
        
        randN = random.randint(0, ivkObj["weightSum"]-1)
        selItem = ivkObj["reglist"][0]
        selIndex = 0
        selWeight = 0
        tmpn = 0


        for item in ivkObj["reglist"]:
            selWeight = item["weight"]
            tmpn += selWeight
            selIndex += 1
            if tmpn > randN:
                selItem = item
                break
        
        result, rsp = invokercli.call(selItem, *param, **arg)
        if 0 != result: # 调用过程失败要移除
            ivkObj["weightSum"] -= selWeight
            invokercli.remove(selItem)
            del ivkObj["reglist"][selIndex]
        
        return result, rsp
                
        
    
