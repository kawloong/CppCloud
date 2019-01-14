#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
cppcloud_serv的python客户端sdk主要类，
负责接收处理相关的业务（譬如，接收关闭通知，响应存活检测等）
'''


import time
import json
import subprocess
import os
from .const import CMDID_MID, CMD_EVNOTIFY_REQ, CMD_SETARGS_REQ
from .tcpclient import TcpClient

cloudapp = None # 单例
def getCloudApp():
    return cloudapp

class CloudApp(TcpClient):

    # kvarg 可选key: svrid, svrname, tag, desc, aliasname, clitype
    #                reqTimeOutSec(请求的超时时间); 
    #       可以定义其他的key，只不过cppcloud_serv不会用到。
    def __init__(self, serv_host, serv_port, **kvarg):
        svraddr = (serv_host, serv_port)
        super(CloudApp, self).__init__(svraddr, **kvarg) # 调用基类初始化
        self.setCmdHandle(CMD_EVNOTIFY_REQ, self.onNotify)
        global cloudapp
        cloudapp = self
        
        self.setNotifyCallBack("check-alive", self._onChkAlive)
        self.setNotifyCallBack("exit", self._onExit)
        self.setNotifyCallBack("shellcmd", self._onShellcmd)
        self.setNotifyCallBack("iostat", self._onIOstat)
        self.setNotifyCallBack("aliasname", self._onSetAliasName)
        ## self.setNotifyCallBack("connected", self.xxx, False)
        ## self.setNotifyCallBack("cfg_change", self.)

 

    def _onChkAlive(self, cmdid, seqid, msgbody):
        return 0, time.time()
    def _onExit(self, cmdid, seqid, msgbody):
        self.bexit = True
        return 0, 'closing'
    def _onShellcmd(self, cmdid, seqid, msgbody):
        shellcmdid = int(msgbody.get('cmdid', 0))
        shellcmdarr = ["uptime", "free -h", "df -h"]
        if os.name == 'nt':
            return 1, 'windows上未支持'
        else:
            if shellcmdid > 0 and shellcmdid < len(shellcmdarr) + 1: ## Linux
                chiproc = subprocess.Popen(shellcmdarr[shellcmdid-1], shell=True, close_fds=True, stdout=subprocess.PIPE)
                return 0, str(chiproc.stdout.read())
            return 400, 'invalid cmdid ' + str(shellcmdid)
    def _onSetAliasName(self, cmdid, seqid, msgbody):
        self.aliasname = msgbody["aliasname"]
        req = {"aliasname": self.aliasname}
        self.request_nowait(CMD_SETARGS_REQ, req)
        return 0, 'ok'
    def _onIOstat(self, cmdid, seqid, msgbody):
        return 0, { "all": [self.recv_bytes, self.send_bytes, self.recv_pkgn, self.send_pkgn] }

    # 通告消息notify处理
    def onNotify(self, cmdid, seqid, msgbody):
        msgbody = json.loads(msgbody)
        print("-- ", msgbody)
        notify = msgbody['notify']
        fromsvrid = msgbody.get('from', 0)

        callResult = self.invokerNotifyCallBack(notify, cmdid, seqid, msgbody)

        if 0 == fromsvrid: 
            return
        code, result = callResult if callResult and len(callResult) > 1 else (0, '')

        return (cmdid|CMDID_MID, 
            seqid, { "to": fromsvrid, 
            "code": code,"result": result })


if __name__ == "__main__":
    pyapp = CloudApp('vpc2', 4800, svrname="PyAppTest")
    ret = pyapp.start()
    if not ret:
        print("start fail");
        exit(-1)
 
    pyapp.join()
    print("end 0")