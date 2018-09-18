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
            throw NormalExceptionOn(404, cmdid|CMDID_MID, seqid, "body json invalid"); }  \
        return on_##CMDID(iohand, &doc, seqid);  }


#endif