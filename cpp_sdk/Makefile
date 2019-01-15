CloudCpp:=switchhand.cpp exception.cpp svrprop.cpp 
SrcCpp:=climanage.cpp listen.cpp iohand.cpp cloudapp.cpp tcp_invoker.cpp tcp_invoker_mgr.cpp 
SrcCpp+=synchand.cpp config_json.cpp config_mgr.cpp provd_mgr.cpp svrconsumer.cpp client_c.cpp
SrcCpp+=svr_item.cpp shttp_invoker_mgr.cpp asyncprvdmsg.cpp svr_stat.cpp 
SrcCpp+=tcpaio_invoker.cpp tcpaio_invoker_mgr.cpp

LINKNAME:=libsdk_cppcloud.so
TSONAME=$(LINKNAME).1
TARGET=$(TSONAME).1.0
ATARGET:=$(LINKNAME).a

MKLLOGFILE:=make.log
ifeq ($(LOG2STD), 1) 
CONF_HEAD_INC+=-DLOG2STDOUT#日志输出至标准输出
endif

LIBCOMMON=../common

INCLUDEDIR+=-I.. -I$(LIBCOMMON) 
CXX=g++
CXXFLAGS=-Wall -g -std=c++11 -fPIC $(CONF_HEAD_INC) $(INCLUDEDIR)
#AR=ar rcs

LDFLAGS=-Wall -g 
LDLIBS= -shared
SONAMEFLAG=-Wl,-soname,$(TSONAME)

ALIBS=

# 以下是使用静态库时选项
LOADLIBES+=-L/usr/local/lib -L$(LIBCOMMON) -Wl,-rpath,.
# LDLIBS+=-lhocomm -lpthread -lcrypto # CppCloud底层动态库

SRC=$(SrcCpp)  $(CloudCpp:%.cpp=../common/cloud/%.cpp)
OBJ=$(SRC:.cpp=.o)
SAMPLES=sample_conf sample_prvd sample_tcp_invk sample_http_invk agent_prvd

# linking operation from *.o and lib.
$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LOADLIBES) $(LDLIBS) $(SONAMEFLAG) $(ALIBS)
	ln -sf $@ $(LINKNAME);
	ln -sf $@ $(TSONAME)
	@echo 
	@echo "make $(TARGET) ok."  $(CONF_DESCRIPTION)
	@echo `date "+%Y%m%d %H:%M:%S"` :  `md5sum $(TARGET)` >> $(MKLLOGFILE)
	@echo $(CONF_DESCRIPTION) >> $(MKLLOGFILE)
	@echo >> $(MKLLOGFILE)

all: $(TARGET) $(SAMPLES)
	@echo 'success'

a: $(ATARGET) $(Alib)

$(ATARGET): $(OBJ) $(Alib)
	$(AR) $@ $^ $(ALIBS)	

sinclude .depend
.PHONY: clean depend help all
clean:
	@rm -rf $(TARGET) $(OBJ) $(COMMON_OBJ) $(LINKNAME) $(TSONAME) $(SAMPLES)

depend: $(SRC)
	$(CXX) -MM -std=c++11 $< $(INCLUDEDIR) $^ > .depend

help:
	@echo "Command                     Target/Description"
	@echo "make                     -> $(LINKNAME)"
	@echo "make all                 -> $(LINKNAME) $(SAMPLES)"
	@echo "make clean               -> clean all .o file"
	@echo "make .depend             -> generate a .depend file by use g++ -MM"
	@echo "make sample_prvd         -> demo for provider"
	@echo "make sample_tcp_invk     -> demo for tcp invoker(consumer)"
	@echo "make sample_http_invk    -> demo for http invoker(consumer)"
	@echo "make agent_prvd          -> a simple tool of static agent provider"

## sample make
sample_conf: sample_conf.cpp
	g++ -Wall -g -std=c++11 $^  -o $@ libsdk_cppcloud.so  ../common/libhocomm.so
sample_prvd: sample_prvd.cpp
	g++ -Wall -g -std=c++11 $^  -o $@ libsdk_cppcloud.so  ../common/libhocomm.so
sample_tcp_invk: sample_tcp_invk.cpp
	g++ -Wall -g -std=c++11 $^  -o $@ libsdk_cppcloud.so  ../common/libhocomm.so
sample_http_invk: sample_http_invk.cpp
	g++ -Wall -g -std=c++11 $^  -o $@ libsdk_cppcloud.so  ../common/libhocomm.so
agent_prvd: agent_prvd.cpp
	g++ -Wall -g -std=c++11 $^  -o $@ libsdk_cppcloud.so  ../common/libhocomm.so
