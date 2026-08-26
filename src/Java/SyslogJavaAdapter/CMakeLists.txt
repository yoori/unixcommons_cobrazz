add_library(SyslogJavaAdapter SHARED
  JavaAdapter.cpp
)

target_include_directories(SyslogJavaAdapter
  PRIVATE
    ${JNI_INCLUDE_DIRS}
)
