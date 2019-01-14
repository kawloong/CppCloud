#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
分布式配置处理
'''

import json
from .cloudapp import CloudApp, getCloudApp 
from .const import CMD_GETCONFIG_REQ,CMD_BOOKCFGCHANGE_REQ,CMD_GETCONFIG_RSP

class CloudConf(object):
    
    def __init__(self):
        self.cloudapp = getCloudApp()
        self.files = {}
        # 主配置文件名
        ## 作用：当要查询主配置文件中的内存时，query的qkey参数文件名部分可省略
        self.mainConfName = ''
    
    # 分布式配置初始化
    # confNamePatten 以空格分格的配置文件名列表 (no use)
    # listfname 配置文件名列表 
    def loadConfFile(self, *listfname):
        #listfname = confNamePatten.split()
        ngcnt = 0
        for fname in listfname:
            rsp = self.cloudapp.request(CMD_GETCONFIG_REQ, {"file_pattern": fname, "incbase": 1})
            rsp = json.loads(rsp)
            if 0 != rsp["code"] or None == rsp["contents"]:
                ngcnt += 1
                print(("No conf file=" + fname, rsp))
            else:
                self._setFile(rsp)
                self.cloudapp.request_nowait(
                    CMD_BOOKCFGCHANGE_REQ, {
                    "file_pattern": fname, "incbase": 1
                })
        
        self.mainConfName = self.cloudapp.mconf
        self.cloudapp.setCmdHandle(CMD_GETCONFIG_RSP, self.onConfResponse)
        self.cloudapp.setNotifyCallBack('cfg_change', self.onChangeNotify)

        return ngcnt
    

    # data格式：  {'code': 0, 'mtime': 1545380285, 'file_pattern': 'app1.json', 'contents': { ...}}
    def _setFile(self, data):
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
        if len(filename) <= 0 or filename not in self.files:
            print(("ERROR: Not filename=" + filename))
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
                    print(("ERROR: nokey=" + key + " in " + filename))
                    return defval
            else:
                return defval
        
        if isinstance(retVal, str):
            retVal = str(retVal)

        return retVal;

    def onChangeNotify(self, cmdid, seqid, msgbody):
        fname = msgbody['filename']
        if msgbody["mtime"] > self.getMTime(fname):
            self.cloudapp.request_nowait(CMD_GETCONFIG_REQ, {"file_pattern": fname, "incbase": 1})

    def onConfResponse(self, cmdid, seqid, msgbody):
        msgbody = json.loads(msgbody)
        if 0 == msgbody["code"] and msgbody["contents"]:
            self._setFile(msgbody)


if __name__ == "__main__":
    pyapp = CloudApp('192.168.1.68', 4800, svrname="PyAppTest")
    ret = pyapp.start()
    cloudconf = CloudConf()

    # 测试：分布式配置获取
    cloudconf.loadConfFile('app1.json app2.json')

    while True:
        val = cloudconf.query('app1.json/key3/keyarr/3')
        print((type(val), val))
        input = eval(input("input q to exit\n"))
        if 'q' == input.strip(): 
            pyapp.shutdown()
            break

    pyapp.join()
    print("end 0")