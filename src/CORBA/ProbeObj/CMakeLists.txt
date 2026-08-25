add_executable(ProbeObj
  Application.cpp
)

target_link_libraries(ProbeObj
  PRIVATE
    TAO::PortableServer
    TAO::Valuetype
    CORBACommons
    CORBAConfigParser
    Generics
    Logger
    ProcessControlIDL
)
