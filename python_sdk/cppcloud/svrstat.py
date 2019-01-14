#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
服务统计模块： 包括服务提供者、服务消息者的调用状况（成功次数、失败次数）
'''

from . import cloudapp
import threading
from .const import CMD_SVRSTAT_REQ

class SvrStat:
    gStatObj = {}
    gtimerRun = False
    gIntervalSec = 10

    def _getVal(self, obj, name, defval = None):
        val = getattr(obj, name, None)
        if None == val:
            val = obj.get(name, defval) if isinstance(obj, dict) else defval
        return val

    def _getStatObj(self, svrObj):
        regname = self._getVal(svrObj, 'regname')
        if not regname:
            return None
        svrid = self._getVal(svrObj, 'svrid', 0)
        prvdid = self._getVal(svrObj, 'prvdid', 0)
        key = regname + '-' + str(svrid) + '-' + str(prvdid)
        ret = self.gStatObj.get(key)
        if not ret:
            ret = {
                "regname": regname, "svrid": svrid, "prvdid": prvdid,
                'pvd_ok':  0,
                'pvd_ng':  0,
                'ivk_ok':  0,
                'ivk_ng':  0,
                'ivk_dok': 0,
                'ivk_dng': 0
            }
            self.gStatObj[key] = ret
        return ret

    def addPrvdCount(self, svrObj, isOk, dcount = 1):
        statobj = self._getStatObj(svrObj)
        if not statobj:
            print(("_getStatObj Fail ", svrObj))
            return -1
        
        countKey = 'pvd_ok' if isOk else 'pvd_ng'
        statobj[countKey] += dcount
        self._startStatTimer()

    def addInvkCount(self, svrObj, isOk, dcount = 1):
        statobj = self._getStatObj(svrObj)
        if not statobj:
            print(("_getStatObj Fail ", svrObj))
            return -1

        countKey = 'ivk_ok' if isOk else 'ivk_ng'
        statobj[countKey] += dcount
        if self._getVal(svrObj, 'svrid', False):
            countKey = 'ivk_dok' if isOk else 'ivk_dng'
            statobj[countKey] += dcount
        self._startStatTimer()


    def _sendStat(self):
        msg = []
        for stkey in self.gStatObj:
            stati = {}
            for (countKey, val) in list(self.gStatObj[stkey].items()):
                if val != 0:
                    stati[countKey] = val
            if stati:
                msg.append(stati)
                self.gStatObj[stkey]["ivk_dok"] = 0
                self.gStatObj[stkey]["ivk_dng"] = 0

        self.gtimerRun = False
        if len(msg) > 0:
            cloudapp.getCloudApp().request_nowait(CMD_SVRSTAT_REQ, msg)


    def _startStatTimer(self, waitSec = 10):
        if not self.gtimerRun:
            self.gIntervalSec = waitSec
            self.timer = threading.Timer(self.gIntervalSec, self._sendStat)
            self.timer.setName("StatTimer-" + str(waitSec) + "sec")
            self.timer.start()
            self.gtimerRun = True
    
    def shutdown(self):
        if self.timer:
            self.timer.cancel()
            self.timer = None


svrstat = SvrStat()


