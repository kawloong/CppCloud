SubComCpp:=
SrcCpp:=flowctrl.cpp climanage.cpp act_mgr.cpp listen.cpp iohand.cpp begn_hand.cpp query_hand.cpp main.cpp 
SrcCpp+=cppcloud_config.cpp exception.cpp

TARGET=cppcloud_serv

MKLLOGFILE:=make.log
#CONF_HEAD_INC+=-DLOG2STDOUT #日志输出至标准输出

LIBCOMMON=../common

INCLUDEDIR+=-I.. -I$(LIBCOMMON) 
CXX=g++
CXXFLAGS=-Wall -g 
#AR=ar rcs

CPPFLAGS+=$(CONF_HEAD_INC) $(INCLUDEDIR)
LDFLAGS=-Wall
LDLIBS=

ALIBS=

# 以下是使用静态库时选项
LOADLIBES+=-L/usr/local/lib -L$(LIBCOMMON) -Wl,-rpath,/usr/local/lib/bmsh
LDLIBS+=-lhocomm -lpthread -lcrypto # CppCloud底层动态库

SRC=$(SrcCpp) $(SubComCpp:%.cpp=../scomm/%.cpp)
OBJ=$(SRC:.cpp=.o)

# linking operation from *.o and lib.
$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LOADLIBES) $(LDLIBS) $(ALIBS)
	


# ---------------------------------------- #
sinclude .depend
.PHONY: clean depend
clean:
	@rm -rf $(TARGET) $(OBJ) $(COMMON_OBJ) $(LINKNAME)

depend: $(SRC)
	$(CXX) -MM $< $(INCLUDEDIR) $^ > .depend

