## **CppCloud分布式配置设计**
---

### **概况**
>借鉴spring cloud config的设计({application}-{profile}.properties)
>> - **{application}** `即是配置文件名中的前部分`
>> - **{profile}**  `配置文件的后部分`
>> - **{label}** `所属git的分支`

> CppCloud采用{servname}-{profile}.json形式定位配置文件资源，
> 目前仅支持json格式配置，提供查询某一部分配置和文件timestamp比较，并且客户端实现层叠配置读取（类似css属性)。

### **配置文件样例**

|json-key|功能作用|
|--------|--------|
|_remXX| 以_rem开头的属于注释，客户端应该忽略|
|mtime| 文件的最后更新时间|
|appnames|分布式应用（服务）名称，如果没有，则由当前配置文件名代替; 这样设计可以多个应该公共一个配置,避免写过多重复的公有配置|


dev/app.json

```
{
	".meta": {
		"_rem": "顶层公共配置",
		"mtime": 15593379939,
		"appnames": ["app1-dev", "app1-test"]
	},
	"/": {
		"name1": "value1",
		"name2": 22,

		"app1": {
			"ip": "192.168.1.100"
		},
		"app2": {
			"name1": "recover_value1"
		}
	}
}
```

_profile.json  配置各app的profile,属于`元配置`, 层叠式越深层匹配合适,越优先采用.
```json
{
	"app1serv": {
		"profile": "dev",
		"host": {
			"192.168.1.2": {
				"profile": "dev1"
			}
		},
		"tag": {
			"tag1": {
				"profile": "dev2"
			}
		}
	},
	"profile": "default"
}
```

### **协议报文**
1. 启动获取{profile}
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

2. 获取具体配置
   + 请求报文
   ````json
   {
	   "svrname": "app1serv",
	   "profile": "dev",
	   "up_mtime": 153123456789,
	   "path": []
   }
   
   参数均可选: 作为app端如果之前调用过以一请求,则中心端会记住状态; web端则需要相应填写.
   ````
   + 响应报文
   ```json
   {
	   "code": 0,
	   "desc": "成功",
	   "mtime": 1540000000,
	   "data":{

	   }
   }
   ```

3. 修改配置
   + 文件级修改(含创建)
	 + 请求报文
	 ```json
	 {
		 "filename": "appn.json",
		 "isreload": true,
		 "data":{}
	 }
	 ```
	 + 响应报文
	 ```json
	 {
		 "code": 0,
		 "desc": "success"
	 }
	 ```
   + 修改已有配置(没有则创建)
	 + 请求报文
	 ```json
	 {
		 "filename": "appn.json",
		 "isreload": true,
		 "keyname": ["name1", "name2"],
		 "data":{}
	 }
	 ```
	 + 响应报文
	 ```json
	 {
		 "code": 0,
		 "desc": "success"
	 }
	 ```