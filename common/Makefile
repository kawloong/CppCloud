SrcCpp:=comm/strparse.cpp comm/log.cpp comm/config.cpp comm/lock.cpp comm/thread.cpp
SrcCpp+=comm/taskpool.cpp comm/file.cpp comm/sock.cpp comm/timef.cpp comm/timespand.cpp
SrcCpp+=comm/base64.cpp comm/simplehttp.cpp comm/hep_base.cpp comm/hepoll.cpp
SrcCpp+=rapidxml/bmshxml.cpp
#SrcCpp+=redis/redis.cpp redis/redispool.cpp redis/redispooladmin.cpp
SrcCpp+=comm/queue.cpp



LINKNAME:=libhocomm
TSONAME=$(LINKNAME).so.1
TARGET=$(TSONAME).1.0
ATARGET:=$(LINKNAME).a # 若要生成静态库.a使用make a
INSTALLDIR:=/usr/lib/cppcloud
LDCONFIG_PATH:=/etc/ld.so.conf.d/cppcloud.conf

MKLLOGFILE:=make.log
CONF_HEAD_INC+=-DSOCKET_LOG
ifeq ($(LOG2STD), 1) 
CONF_HEAD_INC+=-DLOG2STDOUT#日志输出至标准输出
endif



INCLUDEDIR+=-I. # -I$(LIBEVENT)/include/
CXX=g++
CXXFLAGS=-Wall -g -fPIC
AR=ar rcs

CPPFLAGS+=$(CONF_HEAD_INC) $(INCLUDEDIR)
LDFLAGS=-Wall -g 
LDLIBS= -shared
SONAMEFLAG=-Wl,-soname,$(TSONAME)
ALIBS=

# 以下是使用静态库时选项
LOADLIBES+=-L/usr/local/lib
LDLIBS+=-lpthread # -lhiredis -lcrypto

SRC=$(SrcCpp)
OBJ=$(SRC:.cpp=.o)

# linking operation from *.o and lib.
$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LOADLIBES) $(LDLIBS) $(SONAMEFLAG) $(ALIBS)
	ln -sf $@ $(LINKNAME).so;
	ln -sf $@ $(TSONAME)
	@echo 
	@echo "make $(TARGET) ok."  $(CONF_DESCRIPTION)
	@echo `date "+%Y%m%d %H:%M:%S"` :  `md5sum $(TARGET)` >> $(MKLLOGFILE)
	@echo $(CONF_DESCRIPTION) >> $(MKLLOGFILE)
	@echo >> $(MKLLOGFILE)

a: $(ATARGET) $(Alib)

$(ATARGET): $(OBJ) $(Alib)
	$(AR) $@ $^ $(ALIBS)

install:
	@install -d $(INSTALLDIR)
	@if [ -e $(TARGET) ];  then install -p -s -t $(INSTALLDIR) $(TARGET);  fi
	@if [ -e $(ATARGET) ]; then install -p -s -t $(INSTALLDIR) $(ATARGET); fi
	@if [ ! -e $(LDCONFIG_PATH) ]; then echo $(INSTALLDIR) >> $(LDCONFIG_PATH); fi
	@ldconfig
	@echo "install success to $(INSTALLDIR)"

uninstall:
	rm -rf $(INSTALLDIR)
	@echo "uninstalled $(INSTALLDIR)"

#test code # 增加测试程度像如下test1样式添加
test1:  $(patsubst main.o,test1.o,$(OBJ))
	$(CXX) $(LDFLAGS) $^ $(ALIBS) -o $@ $(LOADLIBES) $(LDLIBS)
towork:
	sudo cp $(TARGET) /usr/local/lib/hocomm/ 
	sudo ln -sf $(TARGET) /usr/local/lib/hocomm/$(TSONAME)

# ---------------------------------------- #
sinclude .depend
.PHONY: clean depend install uninstall
clean:
	@rm -rf $(TARGET) $(OBJ) $(COMMON_OBJ) $(LINKNAME) $(TSONAME)

depend: $(SRC)
	$(CXX) -MM $< $(INCLUDEDIR) $^ > .depend
