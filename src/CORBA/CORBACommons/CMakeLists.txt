add_idl(StatsIDL Stats.idl ${IDL_DIRECTORY}/CORBACommons)
add_idl(CorbaObjectRefIDL CorbaObjectRef.idl ${IDL_DIRECTORY}/CORBACommons NO_SERVANT)
add_idl(ProcessControlIDL ProcessControl.idl ${IDL_DIRECTORY}/CORBACommons VALUETYPE)

set(proj "CORBACommons")

add_library(${proj} SHARED
  AceLogger.cpp
  CorbaAdapters.cpp
  CorbaClientAdapter.cpp
  CorbaServerAdapter.cpp
  ProcessControlImpl.cpp
  Reactor.cpp
)

add_dependencies(${proj} CorbaObjectRefIDL)

target_link_libraries(${proj}
  PRIVATE
    ProcessControlIDL
    Generics
    ACE::ACE
    Sync
    OpenSSL::SSL
    OpenSSL::Crypto
    TAO::TAO
    TAO::AnyTypeCode
    TAO::CodecFactory
    TAO::EndpointPolicy
    TAO::IORTable
    TAO::Messaging
    TAO::PortableServer
    TAO::Security
    TAO::TC_IIOP
    TAO::Valuetype
    ACE::SSL
)
