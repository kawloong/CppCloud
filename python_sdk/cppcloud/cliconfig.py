#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
配置管理 -  以json的形式存放配置
可以方便地进行获取和设备json格式的配置文件
'''

import json

class CliConf:
    '''
    usage: cliconf = CliConf()
           cliconf.load(filename)
           # try to get config
           setting1 = cliconf["key1"]
           setting2 = cliconf["key2"]
           # try to recover setting
           cliconf["key2"] = "NewValue"
           cliconf.write() # save to file
    '''
    def __init__(self):
        self._conf_ = {}
        self.conf = dict( filename = 'cliconf.json' )

    def __getattr__(self, name):
        return self.conf.get(name)
    def __getitem__(self, name):
        return self.conf.get(name)
    def __setitem__(self, name, val):
        self.conf[name] = val

    def get(self, name, defv=None):
        val = self.conf.get(name)
        if not val:
            if None != defv:
                val = defv
                self.conf[name] = defv
        return val
    def setConf(self, mp):
        if isinstance(mp, dict):
            self.conf.update(mp)

    def load(self, fna = None):
        if fna:
            self.conf["filename"] = fna
        try:
            fil = None
            print(("Load Conf File[%s]"%self.conf["filename"]))
            fil = open(self.conf["filename"], 'r')
            self.conf.update(json.load(fil))
        except BaseException as e:
            import os
            print(("OPEN file fail, cwd=" + os.getcwd(), e))
        finally:
            fil and fil.close()

    def save(self, fna = None):
        if fna:
            self.conf["filename"] = fna
        try:
            fil = None
            fil = open(self.conf["filename"], 'w')
            json.dump(self.conf, fil, indent=4)
        except BaseException as e:
            print(e)
        finally:
            fil and fil.close()

config = CliConf()
config.load()

if __name__ == "__main__":
    cli = CliConf()
    print( ' -- ' )
    cli.load()

    print((cli["vvv"]))
    cli.ipaddr6 = 99
    cli.setConf( {"ipaddr2": "windowns"} )
    cli["nitem"] = "i am newitem"
    
    cli.save()
    