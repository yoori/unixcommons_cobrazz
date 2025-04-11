add_library(SyslogJavaAdapter SHARED
  JavaAdapter.cpp
)

target_include_directories(SyslogJavaAdapter
  PRIVATE ${JAVA_INCLUDE_DIRECTORIES}
)
