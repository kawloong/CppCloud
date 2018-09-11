#include "remote_cli.h"
#include "iohand.h"

HEPMUTICLASS_IMPL(RemoteCli, RemoteCli, HEpBase)


RemoteCli::RemoteCli(void): m_iohand(NULL)
{

}

RemoteCli::~RemoteCli(void)
{
    m_iohand = NULL;
    // 删除各cli .. wait实现
}

int RemoteCli::onEvent( int evtype, va_list ap )
{
	int ret = 0;
	if (HEPNTF_INIT_PARAM == evtype)
	{
		IOHand* ioh = va_arg(ap, IOHand*);
        m_iohand = ioh;
    }
    else if (HEPNTF_NOTIFY_CHILD == evtype)
    {
		IOBuffItem* bufitm = va_arg(ap, IOBuffItem*);
        unsigned cmdid = bufitm->head()->cmdid;
        unsigned seqid = bufitm->head()->seqid;
	    char* body = bufitm->body();
        ret = cmdHandle(cmdid, seqid, body);
    }

    return 0;
}

// return 1 上层IOHand将会继续调用IOHand::cmdProcess()处理
// 注意: 如返回1,则body应该只读,不能改写.
int RemoteCli::cmdHandle( unsigned cmdid, unsigned seqid, char* body )
{

}