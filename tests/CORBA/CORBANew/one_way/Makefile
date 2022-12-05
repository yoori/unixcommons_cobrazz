TOP = ..

TARGET = test_server
SOURCES = test_server.cpp
IDLS = echo.idl

TARGET2 = test
SOURCES2 = test.cpp

LD_FLAGS = -L$(ORB_LIB) -lTAO_FaultTolerance

include $(TOP)/common.mk
include $(TOP)/second_target.mk

