## **CppCloud分布式配置设计**
---

### **概况**
>借鉴spring cloud config的设计({application}-{profile}.properties)
>> - **{application}** `即是配置文件名中的前部分`
>> - **{profile}**  `配置文件的后部分`
>> - **{label}** `所属git的分支`

> CppCloud采用{servname}-{profile}形式定位配置文件资源，
> 目前仅支持json格式配置，提供查询某一部分配置和文件timestamp比较，并且客户端实现层叠配置读取（类似css属性)。

### **配置文件样例**

|json-key|功能作用|
|--------|--------|
|_remXX| 以_rem开头的属于注释，客户端应该忽略|
|mtime| 文件的最后更新时间|
|appnames|分布式应用（服务）名称，如果没有，则由当前配置文件名作为|


dev/app.json

```
{
	".meta": {
		"_rem": "顶层公共配置",
		"mtime": 155933793939,
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

### **请求报文**
```
if (x > y){
	printf();
}
```

