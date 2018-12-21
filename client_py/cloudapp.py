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
from const import *
from tcpclient import TcpClient
from cloudconf import CloudConf

class CloudApp(TcpClient):

    # kvarg 可选key: svrid, svrname, tag, desc, aliasname, clitype
    #                reqTimeOutSec(请求的超时时间); 
    #       可以定义其他的key，只不过cppcloud_serv不会用到。
    def __init__(self, serv_host, serv_port, **kvarg):
        svraddr = (serv_host, serv_port)
        super(CloudApp, self).__init__(svraddr, **kvarg) # 调用基类初始化
        self.setCmdHandle(CMD_EVNOTIFY_REQ, self.onNotify)
        self.setCmdHandle(CMD_GETCONFIG_RSP, self.onConfResponse)
    
    # 分布式配置初始化
    # confNamePatten 以空格分隔的配置文件名列表
    def loadConfFile(self, confNamePatten):
        self.cloudconf = CloudConf()
        listfname = confNamePatten.split()
        ngcnt = 0
        for fname in listfname:
            rsp = self.request(CMD_GETCONFIG_REQ, {"file_pattern": fname, "incbase": 1})
            rsp = json.loads(rsp)
            if 0 != rsp["code"] or None == rsp["contents"]:
                ngcnt += 1
                print("No conf file=" + fname, rsp)
            else:
                self.cloudconf.setFile(rsp)
                self.request_nowait(CMD_BOOKCFGCHANGE_REQ, {
                    "file_pattern": fname, "incbase": 1
                })

        if self.mconf:
            CloudConf.mainConfName = self.mconf
        return ngcnt
    
    def queryConf(self, qkey, defval = None):
        return self.cloudconf.query(qkey, defval)

    def onConfResponse(self, cmdid, seqid, msgbody):
        msgbody = json.loads(msgbody)
        if 0 == msgbody["code"] and msgbody["contents"]:
            self.cloudconf.setFile(msgbody)

    # 通告消息notify处理
    def onNotify(self, cmdid, seqid, msgbody):
        msgbody = json.loads(msgbody)
        print("-- ", msgbody);
        notify = msgbody['notify']
        code = 0
        result = ''
        fromsvrid = msgbody.get('from', 0)
        if "check-alive" == notify:
            result = time.time()
        elif "exit" == notify:
            self.bexit = True
            result = 'closing'
            #if msgbody.get('force', 0) > 0:
            #    exit(1)
        elif "shellcmd" == notify:
            shellcmdid = int(msgbody.get('cmdid', 0))
            shellcmdarr = ["uptime", "free -h", "df -h"]
            if os.name == 'nt':
                code = 1
                result = 'windows上未支持'
            else:
                if shellcmdid > 0 and shellcmdid < len(shellcmdarr) + 1: ## Linux
                    chiproc = subprocess.Popen(shellcmdarr[shellcmdid-1], shell=True, close_fds=True, stdout=subprocess.PIPE)
                    result = str(chiproc.stdout.read())
                else:
                    code = 400
                    result = 'invalid cmdid ' + str(shellcmdid)
        elif "iostat" == notify:
            result = { "all": [self.recv_bytes, self.send_bytes, self.recv_pkgn, self.send_pkgn] }
        elif "aliasname" == notify:
            self.aliasname = msgbody["aliasname"]
            req = {"aliasname": self.aliasname}
            self.request_nowait(CMD_SETARGS_REQ, req)
            result = 'ok'
        elif "cfg_change" == notify:
            fname = msgbody['filename']
            if msgbody["mtime"] > self.cloudconf.getMTime(fname):
                self.request_nowait(CMD_GETCONFIG_REQ, {"file_pattern": fname, "incbase": 1})
        else:
            code = 404
            result = 'undefine handle'
            print("no hand notify=" + notify)

        if 0 == fromsvrid:
            return

        return (cmdid|CMDID_MID, 
            seqid, {
            "to": fromsvrid,
            "code": code,
            "result": result
            })



if __name__ == "__main__":
    pyapp = CloudApp('192.168.228.44', 4800, svrname="PyAppTest")
    pyapp.run()

    # 测试：分布式配置获取
    pyapp.loadConfFile('app1.json app2.json')

    while True:
        val = pyapp.queryConf('/key3/keyarr/3')
        print(type(val), val)
        time.sleep(1)

    pyapp.join()
    print("end 0")