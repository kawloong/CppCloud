## CppCloud公共模块说明 ##

----------


###  编译环境 ###
* **OS:** linux 2.6.32以上  
* **动态库依赖:** -lhiredis -lcrypto -lz  
* **yum install参考:** zlib-devel.x86_64 openssl-devel.x86_64  
* **redis驱动安装：**[下载](http://download.redis.io/redis-stable.tar.gz)用tar解压后, 
cd /deps/hiredis;make;make install


----------

### 功能API ###

#### comm ####
> 常用操作,工具等的封装,用文件名可知其大概功能,详细参见相应的.h头文件

#### rapidjson ####
> 引用腾讯开发的json解析库,本作者在rapidjson上进一步封出[json.hpp](rapidjson/json.hpp "json.hpp"), 使用细则见头文件注释
#### redis ####
> 本作者开发的对redis原生提供的客户端驱动hiredis进行封装, 并且实现了连接池及池的配置管理等功能.

