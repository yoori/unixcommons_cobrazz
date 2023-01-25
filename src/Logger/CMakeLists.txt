set(proj "Logger")

add_library(${proj}  SHARED
  ActiveObjectCallback.cpp
  DescriptorLogger.cpp
  DistributorLogger.cpp
  FileLogger.cpp
  Logger.cpp
  ProcessLogger.cpp
  SimpleLogger.cpp
  StreamLogger.cpp
  Syslog.cpp

)


target_link_libraries(${proj} Generics pthread)


#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})