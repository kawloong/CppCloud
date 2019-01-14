#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
服务消费者模块
'''

import json
import threading
import time
import random
from . import invokercli
from .const import CMD_SVRSEARCH_REQ, CMD_SVRSEARCH_RSP
from .cloudapp import getCloudApp
from .svrstat import svrstat

class CloudInvoker:
    def __init__(self):
        self.cloudapp = getCloudApp()
        self.invkers = {}
        self.timerIntervalSec = 300 # 5min
        self.endTimestamp = 0
        self.timer = None
        self.offInvk = {}
    

    # 初始化将所有需要消费的服务名传进来，
    # 成功返回0， 失败返回服务发现时失败服务个数
    def init(self, *regNameList):
        ng = 0
        for regName in regNameList:
            resp = self.cloudapp.request(CMD_SVRSEARCH_REQ, {
                "regname" : regName,
                "bookchange" : 1
            })
            ng += self._setPrvdData(regName, resp)
        

        self.cloudapp.setNotifyCallBack("provider_down", self.onProviderDown)
        self.cloudapp.setCmdHandle(CMD_SVRSEARCH_RSP, self._onCMD_SVRSEARCH_RSP)
        if 0 == ng:
            self._setTimerRefreshInvokers(self.timerIntervalSec);
        return ng
    
    # default 300sec (5min)
    def setRefreshTimeout( self, sec ):
        self.timerIntervalSec = sec
    # default 4sec
    # 每次调用的call方法里也可以通过关键字参数timeout=?来为某次调用指定超时值
    def setInvokeTimeout( self, sec ):
        invokercli.reqwait_timeout_sec = sec
    
    # 消费者接口调用
    # return (callresult, response, errCallBack)
    def call(self, regname, *param, **arg):
        ivkObj = self.invkers.get(regname)
        if (not ivkObj) or len(ivkObj["reglist"]) == 0 or ivkObj["weightSum"] <= 1:
            print(("Error: not service=" + regname))
            return -1, 'not service=' + regname, None
        
        randN = random.randint(0, ivkObj["weightSum"]-1)
        selItem = ivkObj["reglist"][0]
        selIndex = 0
        selWeight = 0
        tmpn = 0


        for item in ivkObj["reglist"]:
            selWeight = item["weight"]
            tmpn += selWeight
            if tmpn > randN:
                selItem = item
                break
            else:
                selIndex += 1
        
        result, rsp = invokercli.call(selItem, *param, **arg)
        if result < 0: # 调用过程失败要移除 # =1 timeout
            ivkObj["weightSum"] -= selWeight
            invokercli.remove(selItem)
            del ivkObj["reglist"][selIndex]
        
        def setError(errno):
            svrstat.addInvkCount(selItem, 0 == errno)
        
        return result, rsp, setError

    
    # 重新请求拉取服务提供者信息
    def onTimer(self):
        self.timer = None
        self.endTimestamp = 0
        if self.offInvk:
            for regName in self.offInvk:
                self.cloudapp.request_nowait(CMD_SVRSEARCH_REQ, {
                        "regname" : regName,
                        "bookchange" : 1
                    })
            self._setTimerRefreshInvokers(5)
        else:
            for regName in self.invkers:
                self.cloudapp.request_nowait(CMD_SVRSEARCH_REQ, {
                        "regname" : regName,
                        "bookchange" : 1
                    })
            self._setTimerRefreshInvokers(self.timerIntervalSec)
       
    
    def _setPrvdData(self, regName, resp):
        ng = 0
        if not resp: return 1
            
        resp = json.loads(resp)
        resplist = resp.get("data")
        if 0 == resp["code"] and resplist:
            weightSum = 0
            if not regName: 
                regName = resplist[0].get('regname', 'Unknow')
            for item in resplist:
                weightSum += item.get("weight", 0)
            self.invkers[regName] = {
                "weightSum": weightSum,
                "reglist": list(resplist)
                }
            if weightSum > 0 and regName in self.offInvk: 
                del self.offInvk[regName]
        else:
            print(("No Service=", self.offInvk))
            print(resp)
            ng = 1
        
        return ng
    
    def _onCMD_SVRSEARCH_RSP(self, cmdid, seqid, msgbody):
        self._setPrvdData('', msgbody)
    
       
    def onProviderDown(self, cmdid, seqid, msgbody):
        invokercli.remove(msgbody)
        regname = msgbody.get('regname')
        invker = self.invkers.get(regname)
        if invker:
            def downFilter(itm):
                if msgbody['svrid'] != itm['svrid']:
                    return True
                elif 0 == msgbody['prvdid'] or msgbody['prvdid'] == itm['prvdid']:
                    return False
                return  True
            invker['reglist'] = list(filter(downFilter, invker['reglist']))

        if invker and len(invker.get('reglist')) == 0:
            print(("Error: no provider at " + regname))
            self.offInvk[regname] = True
            self._setTimerRefreshInvokers(5)

    def _setTimerRefreshInvokers(self, dtSec):
        now = time.time()
        if 0 == self.endTimestamp or now + dtSec + 1 < self.endTimestamp:
            if self.timer:
                self.timer.cancel()
            self.timer = threading.Timer(dtSec, self.onTimer)
            self.timer.setName("RefreshIvker-" + str(dtSec) + "sec")
            self.endTimestamp = now + dtSec
            self.timer.start()
            

