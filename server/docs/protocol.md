## 服务端通信报文协议说明

### 概况
> 待完善

### 报文定义

#### 自报身份
|符号名称|解释|
|-------|----|
|CMD_WHOAMI_REQ|0x0001|
|svrid|app唯一代表值|
|*clisock|客户ip:port(服务器set,无需提供)|
|svrname|app名称|
|localsock|客户ip:port|
|clitype| 10 监控进程; 20 web serv; 30 观察进程|
|connect_time|连接建立时间|
|pid|客户进程pid|
|shell|客户进程启动时的shell命令|

#### 对等服务端Serv自报身份
|符号名称|解释|
|-------|----|
|CMD_IAMSERV_REQ|0x0008|
|svrid|本serv端的ID,读自配置|
|clitype| 1 Serv进程|
|svrname|=Serv|

#### 对等服务端Serv自报身份响应
|符号名称|解释|
|-------|----|
|CMD_IAMSERV_RSP|0x0008|
|(其他同CMD_IAMSERV_REQ)||


