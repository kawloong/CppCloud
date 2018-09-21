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


#endif