#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
分布式配置处理
'''

import json
from cloudapp import CloudApp 
from const import *

class CloudConf(object):
    
    def __init__(self, cloudapp):
        self.cloudapp = cloudapp
        self.files = {}
        # 主配置文件名
        ## 作用：当要查询主配置文件中的内存时，query的qkey参数文件名部分可省略
        self.mainConfName = ''
    
    # 分布式配置初始化
    # confNamePatten 以空格分隔的配置文件名列表
    def loadConfFile(self, confNamePatten):
        listfname = confNamePatten.split()
        ngcnt = 0
        for fname in listfname:
            rsp = self.cloudapp.request(CMD_GETCONFIG_REQ, {"file_pattern": fname, "incbase": 1})
            rsp = json.loads(rsp)
            if 0 != rsp["code"] or None == rsp["contents"]:
                ngcnt += 1
                print("No conf file=" + fname, rsp)
            else:
                self.setFile(rsp)
                self.cloudapp.request_nowait(
                    CMD_BOOKCFGCHANGE_REQ, {
                    "file_pattern": fname, "incbase": 1
                })
        
        self.mainConfName = self.cloudapp.mconf
        self.cloudapp.setCmdHandle(CMD_GETCONFIG_RSP, self.onConfResponse)
        self.cloudapp.setNotifyCallBack('cfg_change', self.onChangeNotify, False)

        return ngcnt
    

    # data格式：  {'code': 0, 'mtime': 1545380285, 'file_pattern': 'app1.json', 'contents': { ...}}
    def setFile(self, data):
        filename = data["file_pattern"]
        self.files[filename] = dict(data)

    def getMTime(self, fname):
        fileMsg = self.files.get(fname)
        if fileMsg:
            return fileMsg["mtime"]
        return 0
    
    # 查询操作
    # qkey格式："filename/key1/key2"
    def query(self, qkey, defval = None):
        keyls = qkey.split('/')
        filename = keyls[0] if len(keyls[0]) > 0 else self.mainConfName
        if len(filename) <= 0 or not self.files.has_key(filename):
            print("ERROR: Not filename=" + filename)
            return defval
        
        del keyls[0]
        retVal = self.files.get(filename).get("contents")
        for key in keyls:
            if isinstance(retVal, dict):
                retVal = retVal.get(key)
            elif isinstance(retVal, list):
                if key.isdigit():
                    key = int(key)
                    retVal = retVal[key]
                else:
                    print("ERROR: nokey=" + key + " in " + filename)
                    return defval
            else:
                return defval
        
        if isinstance(retVal, unicode):
            retVal = str(retVal)

        return retVal;

    def onChangeNotify(self, cmdid, seqid, msgbody):
        fname = msgbody['filename']
        if msgbody["mtime"] > self.getMTime(fname):
            self.cloudapp.request_nowait(CMD_GETCONFIG_REQ, {"file_pattern": fname, "incbase": 1})

    def onConfResponse(self, cmdid, seqid, msgbody):
        msgbody = json.loads(msgbody)
        if 0 == msgbody["code"] and msgbody["contents"]:
            self.setFile(msgbody)