## CppCloud python3客户端sdk

install: pip install cppcloud
package: cppcloud

```
import cppcloud

if __name__ == "__main__":
    if not cppcloud.init('serv_host', 4800, svrname="TestApp"):
        print('CloudApp start fail, exit')
        exit(-1)

    # service job here

    cppcloud.uninit()
```

CloudApp实例是*CppCloud python*客户端sdk的核心，提供与服务端cppcloud_serv的长连接，
只要初始化成功后便会自动保持心跳，接收/响应服务器命令，上报本服务状态等。
服务治理的其他业务（配置，服务提供，服务消费）都基于cloudapp之上，各业务实例均采用
单实例模型，下面的函数即是提供应该获得业务实例用的：

    confObject() 分布式配置实例
    statObject() 统计模块实例
    providerObject() 服务提供者 # 无需实例，仅作初始化用
    invokerObject() 服务消费者实例

ps: 如果某种实例无需用到，则不要调用对应的xxxObject()即可。