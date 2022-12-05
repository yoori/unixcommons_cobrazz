ifeq ($(ORB),)
#ORB := OMNI
#ORB := MICO
#ORB := TAO
ORB := TAO2
endif

ifeq ($(ORB),OMNI)

ORB_INCLUDE := /home/konstantin_sadov/omni/usr/local/include
ORB_LIB := /home/konstantin_sadov/omni/usr/local/lib

ORB_CFLAGS := -DORB_OMNI -DORB_NAME=\"omniORB4\" -I$(ORB_INCLUDE)
ORB_LDFLAGS := -L$(ORB_LIB) -lomniDynamic4 -lomniORB4 -lomnisslTP4 -lomnithread

ORB_IDL := /home/konstantin_sadov/omni/usr/local/bin/omniidl
ORB_IDLFLAGS := -bcxx -Wbh=.hpp -Wbs=.cpp

endif

ifeq ($(ORB),MICO)

ORB_BIN := /home/konstantin_sadov/mico/bin
ORB_INCLUDE := /home/konstantin_sadov/mico/include
ORB_LIB := /home/konstantin_sadov/mico/lib

ORB_CFLAGS := -DORB_MICO -DORB_NAME=\"mico-local-orb\" -I$(ORB_INCLUDE)
ORB_LDFLAGS := -L$(ORB_LIB) -lmico2.3.13 -lssl

ORB_IDL := /home/konstantin_sadov/mico/bin/idl
ORB_IDLFLAGS := --c++-suffix cpp --hh-suffix hpp --poa --use-quotes

endif

ifneq ($(filter $(ORB),TAO TAO2),)

ORB_INCLUDE := ${HOME}/tao/usr/include
ORB_LIB := ${HOME}/tao/usr/lib64

ORB_CFLAGS = -DORB_TAO -DORB_NAME=\"\" $(and $(ORB_INCLUDE),-I$(ORB_INCLUDE))
ORB_LDFLAGS = $(and $(ORB_LIB),-L$(ORB_LIB)) \
  -lTAO -lTAO_AnyTypeCode -lTAO_CodecFactory \
  -lTAO_DynamicAny -lTAO_EndpointPolicy -lTAO_IORTable -lTAO_Messaging \
  -lTAO_PI -lTAO_PortableServer -lTAO_Strategies -lTAO_TC_IIOP -lTAO_Utils \
  -lTAO_Valuetype -lACE -lssl

ORB_IDL := ${HOME}/tao/usr/bin/tao_idl

TAO = 1

endif

ifeq ($(ORB),TAO2)

ORB_INCLUDE :=
ORB_LIB :=

ORB_IDL := /usr/bin/tao_idl

endif
