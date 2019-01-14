
#import sys
#import os
#sys.path.append(os.path.split(os.path.realpath(__file__))[0])

from .cloudapp import CloudApp, getCloudApp # python cppcloud-sdk 核心
cloudconf_obj = None
cloudinvoker_obj = None

'''
CloudApp实例是CppCloud python客户端sdk的核心，提供与服务端cppcloud_serv的长连接，
只要初始化成功后便会自动保持心跳，接收/响应服务器命令，上报本服务状态等。
服务治理的其他业务（配置，服务提供，服务消费）都基于cloudapp之上，各业务实例均采用
单实例模型，下面的函数即是提供应该获得业务实例用的：

    confObject() 分布式配置实例
    statObject() 统计模块实例
    providerObject() 服务提供者 # 无需实例，仅作初始化用
    invokerObject() 服务消费者实例

ps: 如果某种实例无需用到，则不要调用对应的xxxObject()即可。
'''

# kvarg 可选key: svrid, svrname, tag, desc, aliasname, clitype
#                reqTimeOutSec(请求的超时时间); 
#       可以定义其他的key，只不过cppcloud_serv不会用到。
def init(serv_host, serv_port, **kvarg):
    '''
    sdk核心初始化，最开始调用的函数， 成功时返回CloudApp实例，失败时None
    # kvarg 可选key: svrid, svrname, tag, desc, aliasname, clitype
    #                reqTimeOutSec(请求的超时时间); 
    #       可以定义其他的key，只不过cppcloud_serv不会用到。
    '''
    cloudapp = CloudApp(serv_host, serv_port, **kvarg)
    if cloudapp.start():
        return cloudapp

def confObject(*listFileName):
    '''
    获取分布式配置的查询实例，传入后续需要用到的配置文件名列表，失败时返回None
    '''
    from .cloudconf import CloudConf # 分布式配置类
    global cloudconf_obj
    if None != cloudconf_obj:
        return cloudconf_obj
    cloudconf_obj = CloudConf()
    ng = cloudconf_obj.loadConfFile(*listFileName)
    if ng > 0:
        cloudconf_obj = None
    return cloudconf_obj

def statObject():
    '''
    返回服务治理的统计实例，可用于服务提供者回馈本次调用结果成功or失败，
    同样适用于服务消费者（CloudInvoker.call()返回的3rd参数功能相同）
    '''
    from .svrstat import svrstat
    return svrstat

def providerObject():
    '''
    服务提供者声明，返回None，仅仅把需要用到的服务提供者模块导入，
    要实现服务提供者功能应该继承自对应的基类，调用相应的基类方法
    '''
    from .provider import ProviderBase # 服务提供者基类
    from .tcpprovider import TcpProviderBase # TCP服务提供者基类(基于socketserver处理tcp请求)


def invokerObject(*regNameList):
    '''
    获取服务消费者操作实例，此实例现已提供tcp和http方式的消费调用，初始时只需传入
    所要消费的服务名列表，cloudapp核心会维护调用者状态
    成功时返回的是CloudInvoker对象，方法可参考cloudinvoker.py
    '''
    from .cloudinvoker import CloudInvoker # 服务消费者类（tcp & http）
    cloudinvoker_obj = CloudInvoker()
    ng = cloudinvoker_obj.init(*regNameList)
    if ng > 0:
        cloudinvoker_obj = None
    return cloudinvoker_obj


def uninit():
    '程序退出时调用， 与init()成对出现'
    cloudapp = getCloudApp()
    if cloudapp:
        cloudapp.shutdown()
        cloudapp.join()