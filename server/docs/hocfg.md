## **CppCloud分布式配置设计**
---

### 1.**概况**
>借鉴spring cloud config的设计({application}-{profile}.properties)
>> - **{application}** `即是配置文件名中的前部分`
>> - **{profile}**  `配置文件的后部分`
>> - **{label}** `所属git的分支`

> CppCloud建议（但不限）采用{servname}-{profile}.json形式定位配置文件资源，
> 目前支持json格式配置，提供查询某一部分配置或者文件级查询，配置可以指定继承关系,从而避免大量重复的键值出现。

### 2.**元配置**
> 即CppCloud分布式配置模块程序的配置,CppCloud启动时加载_meta.json（默认配置文件目录是./conf，可以重新指定）,该文件指定好各配置文件的继承关系; 元配置_meta.json可选,当没有时那个所有配置无继承关系。

```json
_meta.json:

{
    "app1-dev.json": "common.json app1.json", # 从左到右关系越来越密，空格分开
    "app1-test.json": "app1.json"
}
```

### 3.**普通配置**

>要求必须符合**标准JSON格式**的文件，建议命名为{appname}-{profile}.json形式。
```json
app1.json:

{
    "key1":"key1 in app1",
    "hojl":"hejinlong"
}


app1-dev.json:

{
    "key1":"key1 in app1-dev",
    "key2":"key2 in app1-dev",
    "key3":	
    {
        "subkey3": "value3"
    }
}


app2-dev.json:

{
    "key1":"key1 in app2-dev",
    "key3":"key3 in app2-dev",
    "key4": 1234
}
```


### 4.**协议报文**
#### 4.1 启动获取{profile} ####
   > 应用一启动时应该向配置中心询问本次启动所使用的profile, 后续的获取具体配置请求时附带profile作为参数; 此作用主要是方便各应用在不同环境有不同配置, 譬如开发环境会获取profile=dev, 测试profile=test.
   + 请求报文
    ```json
        {
            "svrname": "app1serv",
            "svrid": 10,
            "host": "192.168.1.101",
            "tag": ""
        }
        必填: svrname
    ```
    + 响应报文
    ```json
        {
            "profile": "dev"
        }
    ```

#### 4.2 列出所有配置文件 ####
+ 请求报文 *(CMD_GETCFGNAME_REQ=0x0010)*  
    *无需包体*

+ 响应报文 *(CMD_GETCFGNAME_RSP=0x1010)*
  |键名|说明|
  |----|--|
  |filename|配置名，响应返回所有配置文件名|
  |flag|文件存在标记，0不存在或已被删除，1正常存在可用|
  |mtime|文件修改时间截，分布式配置文件同步时以此时间作衡量，新的可覆盖旧的，旧的不能覆盖新的。|
  ```json
    {
        "$filename": [$flag, $mtime ],
        "app1-dev": [1, 1539252977]
    }
  ```

#### 4.3 获取具体配置 ####
 + 请求报文 *(命令CMD_GETCONFIG_REQ=0x000E)*

    |键名|必填|说明|
    |---|---|----|
    |file_pattern|是|配置名，譬如app1-dev.json，或应用配置比较多时，可以使用目录前缀|
    |key_pattern|否|查询键串，当要返回配置中的某一部分时使用；如果不指定默认返回整个文件级内容，多级键之前用/分隔，譬如/key1/key12/key123。|
    |inbase|否|是否包含继承关系，包含的话则先按照_meta.json元配置定义来先生成一个完整json内容。    |

    ````json
    {
       "file_pattern": "app1-dev",
       "key_pattern": "/",
       "incbase": 0
    }
   ````

+ 响应报文 *(命令CMD_GETCONFIG_RSP=0x100E)*
    ```json
    {
       "key1": "value1",
       "key2": 2222
    }
    ```
+ 示例
  >譬如，拿上面的普通配置示例说明，
    ````json
    A请求
    {
        "file_pattern": "app1-dev", 
        "incbase": 0
    }
    A响应 -->
    {
        "key3":{"subkey3":"value3"},
        "key2":"key2 in app1-dev",
        "key1":"key1 in app1-dev"
    }

    B请求(继承)
    {
        "file_pattern": "app1-dev", 
        "incbase": 1
    }
    B响应 -->
    {
        "key2":"key2 in app1-dev",
        "hojl":"hejinlong",
        "key3":{"subkey3":"value3"},
        "key1":"key1 in app1-dev"
    }

    C请求(含查询键)
    {
        "file_pattern": "app1-dev", 
        "incbase": 0, 
        "key_pattern": "/key3"
    }
    C响应 -->
    {
        "subkey3":"value3"
    }

    D请求(含查询键，多级)
    {
        "file_pattern": "app1-dev", 
        "incbase": 0, 
        "key_pattern": "/key3/subkey3"
    }
    D响应 -->
    "value3"

    E请求(不存在的键)
    {
        "file_pattern": "app1-dev", 
        "incbase": 0, 
        "key_pattern": "/key9"
    }
    E响应 -->
    null

    ````



#### 4.4 修改配置 ####
   > 修改时应当以原始文件且非继承为单位进行，不能只修改文件的某一部分。

+ 请求报文 *(命令CMD_SETCONFIG_REQ=0x000F)*
  |键名|必填|说明|
  |----|--|----|
  |filename|是|配置名，响应返回所有配置文件名|
  |mtime|否|文件修改时间截，分布式配置文件同步时以此时间作衡量，新的可覆盖旧的，旧的不能覆盖新的。 不填写时会以收到时的时间填充。|
  |contents|是|具体json配置文件的内容，中心端收到请求时将contents值中的内容存放。
     ```json
     {
         "filename": "app1-dev.json",
         "mtime": 1539333414,
         "contents":{ ... }
     }
     ```
+ 响应报文 *(命令CMD_SETCONFIG_RSP=0x100F)*
     ```json
     {
         "code": 0,
         "desc": "success"
     }
     ```
   