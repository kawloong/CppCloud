# CppCloud产品说明文档

&emsp;　本产品旨在提供适合多语言开的的分布式微服务架构，功能包括**服务治理（注册/消费）**，**分布式配置**，**服务监控**，**日志收集**等。用户或企业可借此产品可以快速开发具有分布式集群的应用，集中精力来开发自身业务逻辑。
- [技术blog站](http://blog.cppcloud.cn)



## **前言/背景**

&emsp;　近年来移动互联网的兴起，各应用数据量业务量不断攀升；后端架构要不断提高性能和并发能力才能应对大信息时代的来临；[传统单体程序 -> 面向服务soa -> 微服务](https://www.cnblogs.com/imyalost/p/6792724.html)；`SpringCloud`和`Dubbo`的出现为企业开发分布式应用提供了很好的脚手架，Java得益于ioc/aop反向代理/注解等技术,开发者可以轻松用来构建自己的应用；  
&emsp;　Java开发分布式微服务是方便了，然而`SpringCloud`提供java之外的接入文档或sdk却非常少，国内更加少了；微服务不是提昌各类的开发者都能参与进来看发整个系统的某一块服务吗，譬如有关业务计算的让cpp、golang开发，展示的让py开发；虽然Netflix说`SpringCloud`各个接口是Restful，但学到用起来的成本个人觉得还是很高。

## **项目介绍**

&emsp;　本人开发的 ***CppCloud*** 目的就是快速构建分布式服务之余，**特点轻量级**，可以方便国内**不同语言**的开发者参与微服务的开发，而不仅限java；本项目核心服务（**cppcloud_serv**）采用c++开发，支持分布式部署，对外提供tcp服务；外部各应用接入时可以采用原生tcp协议接入、sdk接入、http间接接入；sdk方面目前开发了c++和python，由于一个人精力有限，其他的sdk暂时还未开发出来。  
- [TCP协议接入](http://www.cppcloud.cn/doc/server/tcp_cmd/index.html)
- [C/C++ sdk接入](http://www.cppcloud.cn/doc/cpp_sdk/manual/index.html)
- [Python sdk接入](http://www.cppcloud.cn/doc/python_sdk/manual/index.html)

## **目录组织**
``` lua
cppcloud                          -- 根目录
├── common                        -- c++公共类库，编译出libhocomm.so
|   ├── comm                      -- 基础操作封装类（独立于项目的）
|   ├── cloud                     -- 本项目相关的常量及通用类定义
|   ├── rapidjson                 -- 本项目处理json串所用的第3方库
|   ├── Makefile                  -- 编译构建出libhocomm.so/libhocomm.a
|   ├── *.h *.cpp                 -- 具体的文件
├── server                        -- 中心端程序所在目录
|   ├── docs                      -- 说明文档
|   ├── Makefile
|   ├── *.h *.cpp
├── cpp_sdk                       -- c++应用端sdk
|   ├── docs                      -- 说明文档
|   ├── Makefile                  -- 编译构建出libsdk_cppcloud.so
|   ├── *.h *.cpp
├── python_sdk                    -- python应用端sdk（包）
|   ├── cppcloud                  -- 基于python3的sdk
|   ├── cppcloud_py2              -- 基于python2的sdk(弃用)
|   ├── setup.py                  -- 打包、安装、发布脚本
|   ├── Makefile                  -- 提供一些快捷命令(eg. make clean)
|   ├── sample_*.py               -- 常见服务治理的使用示例参考
├── web_py                        -- 基于python_sdk包而开发的cppcloud-web应用
|   ├── src
|   |    ├── static               -- html/css/js 本web应用采用前后端
|   |    ├── cppcloud_web.py      -- 启动时主入口
|   ├── setup.py                  -- 打包、安装、发布脚本
|   ├── Makefile                  -- 提供一些快捷命令
|   ├── LICENSE                   -- 开源授权信息
├── docker                        -- docker方式打包&部署目录(server+cpp_sdk)
|   |── docker-compose.yml
|   |── docker-entrypoint.sh       -- 容器入口点脚本
|   |── Dockerfile                 -- 主构建镜像配置
|   |── help.txt
|   |── Makefile                   -- 主控makefile
├── client_go                      -- 暂未开发

```

## **术语约定**
|名词|说明|
|--|--|
|serv|后端程序cppcloud_serv|
|svrname|应用名称，指连接Serv的客户端所取的一个名字|
|regname|服务提供者名称，可以和svrname一样|
|cli|客户应用、客户端、客户连接|
|app|同上差不多一样，都指客户端|
|svrid|每个客户应用都有一个区分身份的id|
|era|cppcloud_serv内的术语，译作版本号，用于分布式部署Serv时互相同步客户数据|
|provider| 服务提供者，简写prvd、pvd|
|invoker| 服务调用者，服务消费者|
|consumer|同invoker|
|mtime|修改时间|
|atime|访问时间|
|ctime|初始时间，创建时间|



## **环境及依赖**
&emsp;　cppcloud尽可能少的依赖其他第3方库，部署起来比较轻便。
> #### c++部分
+ **linux**  &nbsp;&nbsp;   *2.6.32以上(建议centos7)*
+ **g++**  &nbsp;&nbsp;&nbsp; *4.8以上支持c++11*
> #### python部分
+ **python**  &nbsp;&nbsp; *3.5以上*
+ **requests**  &nbsp;&nbsp; *无特别要求，本人开发时用2.18.4*
+ **setuptools**  &nbsp;&nbsp; *无特别要求，本人开发时用28.8.0*
+ **flask**  &nbsp;&nbsp; *无特别要求，本人开发时用1.0.2*


## **架构图示**
![图片加载中](/img/summery.png "summery.png")

> ### **服务端Serv**
>> &emsp; CppCloud的核心部分，c++实现，提供分布式服务的服务注册、发现、管理等功能，相当于SpringCloud里的Eureka Server的角色；对外提供tcp接口，支持多个Serv集群提高可靠性；cppcloud_serv内部采用epoll异步io复用([Reactor模型](https://www.cnblogs.com/linganxiong/p/5583415.html))，性能比拟nginx/libevent。&emsp; 
> ### **服务提供Provider**
>> &emsp; 向cppcloud_serv注册了服务的客户应用，与cppcloud_serv通信使用tcp协议，每一类服务都有唯一的名称，以便让调用者（消费者）根据服务名称(regname)调用。 服务注册时需要提供的主要属性有服务名（regname），服务识别号（prvdid，同一进程提供多服务时使用）协议类别（protocol），权重（weight），启用（enable）等。
> ### **服务调用Invoker**
>> &emsp; 需要调用（消费）某服务的客户应用，同样的通过tcp与cppcloud_serv通信。不同服务以服务名称区分，调用前应该是明确知道本次要调用的服务名(regname)。 操作流程是首先拿regname向cppcloud_serv查询服务，返回可用服务列表，然后调用者从列表中选出合适的某个Provider，最后直接向该Provider发起调用，调用的具体状况要看协议类别和需要的参数。

> ### **Web管理**
>> &emsp; 提供可视化web界面操作CppCloud后端，底层用python sdk通过tcp和cppcloud_serv通信，上层对用户提供出restful的接口，和前后端分离的web管理应用。 维护人员可以在web监视服务运行状况、给某应用下发命令，维护分布式配置、修改服务提供者权重等功能。

## **开发部署**
 &emsp; &emsp; *有执行过的步骤无需重复进行， 每步序号对应于"源码开发/部署步骤"节*  

+ serv  
    > [1](#stp1) > [2](#stp2) > [5](#stp5) > [6](#stp6)
+ c++ sdk  
    > [1](#stp1) > [2](#stp2) > [5](#stp5) > [7](#stp7)
+ python sdk  
    > [1](#stp1) > [3](#stp3) > [4](#stp4) > [8](#stp8)
+ web  
    > [1](#stp1) > [3](#stp3) > [4](#stp4) > [9](#stp9)
  
+ 源码开发/部署步骤<span id = "stp1"></span> 
  1. 从git上clone本项目到本地
     >`git clone https://gitee.com/ho999/CppCloud.git`
  <span id = "stp2"></span> 
  2. 安装好编译环境，要求g++ 4.7以上支持c++11特性。如果是centos7就执行如下命令即可；如果centos6的要注意，yum默认不会安装最新版g++，需手动安装，可参考[这里](https://blog.csdn.net/centnethy/article/details/81284657)。
      >`yum -y install gcc-c++`  
      >`yum -y install gdb` (如需调试项目)
  
  <span id = "stp3"></span>
  
  3. python环境(含pip)，要求python3以上；网上教程很多了，[譬如这里](https://www.cnblogs.com/kimyeee/p/7250560.html);
  <span id = "stp4"></span> 

  4. 安装python依赖；本项目依赖比较少了，主要*requests, flask*;
     >` pip3 install requests flask;`
  <span id = "stp5"></span> 
  5. 编译生成公共模块库*libhocomm.so*，即执行
     >`cd CppCloud/common && make`;  
     安装 `sudo make install`
  <span id = "stp6"></span> 
  6. 编译安装*cppcloud_serv*; 正常的话就会输出有*cppcloud_serv*可执行文件。
     >`cd CppCloud/server && make`    
     安装`sudo make install`
  <span id = "stp7"></span> 
  7. 编译安装c++ sdk(cpp_sdk)
      >`cd CppCloud/cpp_sdk && make`  
      `sudo make install;`
  <span id = "stp8"></span> 
  8. 安装python sdk; python无需编译
      >` cd python_sdk; sudo make install;`
  <span id = "stp9"></span> 
  9.  安装web_py;; python无需编译  
       >`cd web_py; sudo make install;`

## **快速部署**
&emsp; 本产品可以通过docker快速部署*c++部分*(cppcloud_serv cpp_sdk)，以及通过pypi快速部署*python部分*(python_sdk web_py)

+ c++部分（前提是先[安装docker](https://baijiahao.baidu.com/s?id=1609874072198430253&wfr=spider&for=pc)）
  > 本产品c++部分为方便大众开发者使用，作者已上传至dockerhup，通过以下命令获取到;  
 &emsp; &emsp; `docker pull valueho/cppcloud:1`  
   
  > 运行容器  
 &emsp; &emsp; 
`docker run -p4800:4800 -it valueho/cppcloud:1 -i <servid> `  
  >运行容器帮助信息  
 &emsp; &emsp; 
`docker run -p4800:4800 -it valueho/cppcloud:1 help `


+ python部分（前提是已安装pip3）
  > python sdk安装一条命令完成  
   &emsp; &emsp;`pip install cppcloud`  
   成功后*import cppcloud*;无错误提示。    
     
  > web_py安装命令  
   &emsp; &emsp;`pip install cppcloud-web`  
   成功安装后可以使用命令*cppcloud-web*

## **应用示例**
&emsp; &emsp; 用简单的demo，帮助开发者快速接入CppCloud，降低学习成本，主要介绍sdk的使用。  
+ cppcloud_serv使用示例  
+ [分布式配置示例(c++)](http://www.cppcloud.cn/doc/cpp_sdk/sample_conf/index.html)  
+ [tcp服务提供者(c++)](http://www.cppcloud.cn/doc/cpp_sdk/sample_prvd/index.html)  
+ [tcp服务消费者(c++)](http://www.cppcloud.cn/doc/cpp_sdk/sample_invk/index.html)  
+ [http服务消费者(c++)](http://www.cppcloud.cn/doc/cpp_sdk/sample_http_invk/index.html)  
+ [tcp服务提供者(python)](http://www.cppcloud.cn/doc/python_sdk/sample_tcp_prvd/index.html)  
+ [tcp服务消费者(python)](http://www.cppcloud.cn/doc/python_sdk/sample_tcp_invk/index.html)  
+ [http服务提供者(python)](http://www.cppcloud.cn/doc/python_sdk/sample_http_prvd/index.html)  
+ [http服务消费者(python)](http://www.cppcloud.cn/doc/python_sdk/sample_http_invk/index.html)  

