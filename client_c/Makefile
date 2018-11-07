CloudCpp:=switchhand.cpp exception.cpp svrprop.cpp 
SrcCpp:=climanage.cpp listen.cpp iohand.cpp cloudapp.cpp tcp_invoker.cpp tcp_invoker_mgr.cpp 
SrcCpp+=synchand.cpp config_json.cpp config_mgr.cpp provd_mgr.cpp svrconsumer.cpp client_c.cpp
SrcCpp+=svr_item.cpp shttp_invoker_mgr.cpp

LINKNAME:=libsdk_cppcloud.so
TSONAME=$(LINKNAME).1
TARGET=$(TSONAME).1.0
ATARGET:=$(LINKNAME).a

MKLLOGFILE:=make.log
CONF_HEAD_INC+=-DLOG2STDOUT -DDEBUG #日志输出至标准输出

LIBCOMMON=../common

INCLUDEDIR+=-I.. -I$(LIBCOMMON) 
CXX=g++
CXXFLAGS=-Wall -g -std=c++11 -fPIC -I$(INCLUDEDIR)
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

a: $(ATARGET) $(Alib)

$(ATARGET): $(OBJ) $(Alib)
	$(AR) $@ $^ $(ALIBS)	

sinclude .depend
.PHONY: clean depend
clean:
	@rm -rf $(TARGET) $(OBJ) $(COMMON_OBJ) $(LINKNAME) $(TSONAME)

depend: $(SRC)
	$(CXX) -MM -std=c++11 $< $(INCLUDEDIR) $^ > .depend

