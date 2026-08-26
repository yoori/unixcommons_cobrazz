set(proj "CORBAConfigParser")

add_library(${proj}  SHARED
  CorbaConfigReader.cpp
  ParameterConfig.cpp
)
target_link_libraries(${proj}
  PRIVATE
    CORBACommons
    Generics
    ProcessControlIDL
    XercesC::XercesC
    XMLUtility
)
