#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
服务提供者模块
处理与cppcloud_serv的注册事件
'''


from .cloudapp import getCloudApp
from .const import CMD_SVRREGISTER_REQ, CMDID_MID


class ProviderBase(object):
    regname = ''
    host = ''
    port = 0  # 不提供时会选择一些随机端口
    url = ''
    scheme = '' # 和protocol，任选一个指定协议
    protocol = 0 # 1 tcp, 2 udp, 3 http, 4 https
    weight = 100
    enable = 1
    desc = ''
    prvdid = 0
    async_response = False
    http_path = ''
    cloudapp = None
    

    
    # 调用前请确保已初始化CloudApp实例
    @classmethod
    def Regist(cls, reg):
        cloudapp = getCloudApp()
        cls.cloudapp = cloudapp
        if not cloudapp:
            print("Error: cloudapp not init")
            return None

        if not cls.regname:
            cls.regname = cloudapp.svrname
        
        cls.prvdid = ProviderBase.prvdid
        ProviderBase.prvdid += 1
        if 0 == cls.port:
            cls.port = 2000 + cls.prvdid

        cls._buildUrl()

        cloudapp.setNotifyCallBack("reconnect_ok", cls._onServReconnect) # 重连成功回调
        cloudapp.setNotifyCallBack("provider", cls._onSetProvider) # 设置weight/enable回调
        if reg:
            cls.regProvider()

        return cls
    
    @classmethod
    def _buildUrl(cls):
        if 0 == cls.protocol:
            schemeProtocol = {'tcp': 1, 'udp': 2, 'http': 3, 'https': 4, '': 0}
            cls.protocol = schemeProtocol[cls.scheme]
        if cls.url:
            return True
        urlprefix = ('unknow', 'tcp', 'udp', 'http', 'https')

        if not cls.host:
            cls.host = cls.cloudapp.cliIp

        cls.url = urlprefix[cls.protocol] + '://' + cls.host + ':' + str(cls.port) + cls.http_path
    
    # cloudapp断开再连接时，应该再次注册服务提供信息
    @classmethod
    def _onServReconnect(cls, *param):
        print("Found serv reconnect ok")
        cls.regProvider()

    @classmethod
    def _onSetProvider(cls, cmdid, seqid, msgbody):
        if msgbody["regname"] == cls.regname and msgbody["prvdid"] == cls.prvdid:
            if 'enable' in msgbody:
                cls.enable = msgbody['enable']
            if 'weight' in msgbody:
                cls.weight = msgbody['weight']
            cls.regProvider('weight', 'enable')
            return 0, 'update ok'
    
    # 参数：prop如果为空，则发送所有
    @classmethod
    def regProvider(cls, *prop):
        if not prop:
            prop = ("prvdid", "url", "desc", "protocol", "weight", "enable", "idc", "rack")
        svrprop = {}
        for key in prop:
            val = getattr(cls, key, None)
            if None != val:
                svrprop[key] = val

        cls.cloudapp.request_nowait(CMD_SVRREGISTER_REQ, {
            "regname": cls.regname,
            "svrprop": svrprop
        })
    
   
 
