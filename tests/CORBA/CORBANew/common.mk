TOP ?= .
include $(TOP)/orb.mk

CXX = g++
CPPFLAGS += -D_REENTRANT
CXXFLAGS += -pthread -W -Wall -g -O0 -fno-inline
LDFLAGS =

RM = rm -f

ifndef TAO

.SECONDARY: \
  $(addsuffix .cpp, $(basename $(IDLS))) \
  $(addsuffix .h, $(basename $(IDLS)))

else

.SECONDARY: \
  $(addsuffix S.cpp, $(basename $(IDLS))) \
  $(addsuffix S.inl, $(basename $(IDLS))) \
  $(addsuffix S.h, $(basename $(IDLS))) \
  $(addsuffix C.cpp, $(basename $(IDLS))) \
  $(addsuffix C.inl, $(basename $(IDLS))) \
  $(addsuffix C.h, $(basename $(IDLS)))

endif

ifndef TAO

OBJECTS = $(addsuffix .o, $(basename $(SOURCES)))
IDL_OBJECTS = $(addsuffix .o, $(basename $(IDLS)))

else

OBJECTS = $(addsuffix .o, $(basename $(SOURCES)))
IDL_OBJECTS = $(addsuffix S.o, $(basename $(IDLS))) \
  $(addsuffix C.o, $(basename $(IDLS)))
HEADER = $(addsuffix .hpp, $(basename $(IDLS)))

endif

HEADERS = $(addsuffix .hpp, $(basename $(IDLS)))
REMOVE += $(TARGET) $(OBJECTS) $(IDL_OBJECTS) $(HEADERS) \
  $(addsuffix .cpp, $(basename $(IDLS)))

ifdef TAO

REMOVE += $(TARGET) $(OBJECTS) $(IDL_OBJECTS) $(HEADER) \
  $(addsuffix S.cpp, $(basename $(IDLS))) \
  $(addsuffix S.h, $(basename $(IDLS))) \
  $(addsuffix S.inl, $(basename $(IDLS))) \
  $(addsuffix C.cpp, $(basename $(IDLS))) \
  $(addsuffix C.inl, $(basename $(IDLS))) \
  $(addsuffix C.h, $(basename $(IDLS)))

endif

.SUFFIXES:
.SUFFIXES: .o .hpp .cpp .idl
.PHONY: all clean check debug

all: $(TARGET)

clean:
	$(RM) $(REMOVE)

check: all
	-LD_LIBRARY_PATH=$(ORB_LIB):$$LD_LIBRARY_PATH ./test

DBG ?= ddd

debug: all
	-@LD_LIBRARY_PATH=$(ORB_LIB):$$LD_LIBRARY_PATH $(DBG) ./test

$(TARGET): $(OBJECTS) $(IDL_OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(ORB_CFLAGS) -o $@ $^ $(ORB_LDFLAGS) $(LD_FLAGS)

$(OBJECTS) : $(IDL_OBJECTS)

$(IDL_OBJECTS): $(HEADERS)


ifndef TAO

%.cpp %.hpp : %.idl
	PATH=$(ORB_BIN):$$PATH LD_LIBRARY_PATH=$(ORB_LIB):$$LD_LIBRARY_PATH $(ORB_IDL) $(CPPFLAGS) $(ORB_IDLFLAGS) $<

else

%S.hpp %S.cpp %C.hpp %C.cpp : %.idl
	PATH=$(ORB_BIN):$$PATH LD_LIBRARY_PATH=$(ORB_LIB):$$LD_LIBRARY_PATH $(ORB_IDL) $(CPPFLAGS) $(ORB_IDLFLAGS) $<

endif

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(ORB_CFLAGS) -c -o $@ $<

ifdef TAO

$(OBJECTS) : $(HEADER)

$(HEADER) : $(IDLS)
	echo "#include \""$(addsuffix S.h, $(basename $(IDLS)))"\"" >$(HEADER)
	echo "#include \"tao/PortableServer/PortableServer.h\"" >>$(HEADER)
	echo "#include \"tao/IORTable/IORTable.h\"" >>$(HEADER)
	echo "#include \"tao/Valuetype/ValueFactory.h\"" >>$(HEADER)
	echo "#include \"tao/DynamicAny/DynamicAny.h\"" >>$(HEADER)
	echo "#include \"tao/Messaging/Messaging.h\"" >>$(HEADER)

endif
