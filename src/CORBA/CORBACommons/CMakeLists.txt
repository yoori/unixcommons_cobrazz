#cmake_minimum_required (VERSION 2.6)
add_idl(StatsIDL Stats.idl ${IDL_DIRECTORY}/CORBACommons)
add_idl(CorbaObjectRefIDL CorbaObjectRef.idl ${IDL_DIRECTORY}/CORBACommons)
add_idl(ProcessControlIDL ProcessControl.idl ${IDL_DIRECTORY}/CORBACommons)

set(proj "CORBACommons")

add_library(${proj} SHARED
  AceLogger.cpp
  CorbaAdapters.cpp
  CorbaClientAdapter.cpp
  CorbaServerAdapter.cpp
  ProcessControlImpl.cpp
  Reactor.cpp
)

target_link_libraries(${proj} StatsIDL CorbaObjectRefIDL ProcessControlIDL
    Generics ACE Logger Sync String
    pcre ssl crypto z event
    TAO TAO_AnyTypeCode  TAO_CodecFactory TAO_CosEvent TAO_CosNaming TAO_CosNotification TAO_DynamicAny TAO_EndpointPolicy TAO_FaultTolerance
    TAO_FT_ClientORB TAO_FT_ServerORB TAO_FTORB_Utils TAO_IORManip TAO_IORTable
    TAO_Messaging TAO_PI TAO_PI_Server TAO_PortableGroup TAO_PortableServer TAO_Security TAO_SSLIOP TAO_TC TAO_TC_IIOP TAO_Valuetype ACE_SSL
    )
#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})
