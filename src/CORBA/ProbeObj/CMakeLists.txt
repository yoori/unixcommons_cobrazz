add_executable(ProbeObj
  Application.cpp
)

target_link_libraries(ProbeObj
  PRIVATE
    CORBACommons
    CORBAConfigParser
    Generics
    Logger
    ProcessControlIDL
)
