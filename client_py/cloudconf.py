#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
分布式配置处理
'''

class CloudConf(object):
    # 主配置文件名
    ## 作用：当要查询主配置文件中的内存时，query的qkey参数文件名部分可省略
    mainConfName = ''
    
    def __init__(self):
        self.files = {}
    
    def setMainFName(self, fname):
        self.mainConfName = fname

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
