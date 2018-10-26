SubComCpp:=
SrcCpp:=climanage.cpp listen.cpp iohand.cpp

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

SRC=$(SrcCpp) $(SubComCpp:%.cpp=../scomm/%.cpp)
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


depend: $(SRC)
	$(CXX) -MM -std=c++11 $< $(INCLUDEDIR) $^ > .depend

