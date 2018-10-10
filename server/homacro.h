#ifndef _HO_MACRO_H_
#define _HO_MACRO_H_

#define CMDID2FUNCALL_BEGIN                                                               \
    IOHand* iohand = (IOHand*)ptr;                                                        \
	IOBuffItem* iBufItem = (IOBuffItem*)param;                                            \
	unsigned seqid = iBufItem->head()->seqid;                                             \
	char* body = iBufItem->body();
#define CMDID2FUNCALL_CALL(CMDID)                                                         \
    if(CMDID==cmdid) { Document doc;                                                      \
        if (doc.ParseInsitu(body).HasParseError()) {                                      \
            LOGERROR("MSGJSON| body=%s", body);                                           \
            throw NormalExceptionOn(404, cmdid|CMDID_MID, seqid, "body json invalid " __FILE__  ); }  \
        return on_##CMDID(iohand, &doc, seqid);  }

// 消息处理函数开始时的变量接收和包体的初始合法判断, 参数为是否改变body原始包体;
#define MSGHANDLE_PARSEHEAD(isConstBody)                                                                \
    IOHand* iohand = (IOHand*)ptr;                                                                      \
	IOBuffItem* iBufItem = (IOBuffItem*)param;                                                          \
	unsigned seqid = iBufItem->head()->seqid;                                                           \
	char* body = iBufItem->body();                                                                      \
    Document doc;                                                                                       \
    if (isConstBody ? doc.Parse(body).HasParseError():                                                  \
                      doc.ParseInsitu(body).HasParseError()) {                                          \
            LOGERROR("MSGJSON| iohand=%s| body=%s", iohand->m_idProfile.c_str(), body);                 \
            throw NormalExceptionOn(404, cmdid|CMDID_MID, seqid, "body json invalid " __FILE__  ); }    \
    


#endif