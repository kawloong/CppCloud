## 4大功能待下周开发
+ 主动connect to serv.
+ 主动keepalive （epoll timer）.
+ 利用map<> lower_bound() upper_bound()实现cli分类等功能.
+ 分布式元数据同步，采用a.每个serv记录一个uptimestamp（cli有关键数据修改时更新），
之后每隔5min广播，接收到广播者接收消息轨迹，比较uptimestamp新旧，不匹配则向发动源
请求要一份全时数据（附带timestamp参数)。
   或者广播用[s1:tsp1, s2:tsp2].
