/******************************************************************* 
 *  summery: 消息队列处理类
 *  author:  hejl
 *  date:    2017-03-15
 *  description: 封装ipc msgget msgsnd msgrcv ...
 ******************************************************************/

class IpcMsg
{
public:
    IpcMsg(void);
    int init(int key, bool creatIfno);

    // 获取队列的数量
    int qsize(void);
    // 删除
    int del(void);

    // 发送(阻塞mode)
    int send(const void* dataptr, unsigned int size);
    // 接收(阻塞mode)
    int recv(void* dataptr, unsigned int size, bool noerr, bool noblock = false);
    // 清空队列的数据
    int clear(int num = 0);

private:
    int m_key;
    int m_msgid;
};

