#cmake_minimum_required (VERSION 2.6)


include_directories( ${IDL_DIRECTORY}  . )

set(proj "CORBACrashCallClient")

add_executable(${proj}
Application.cpp
)

add_dependencies(${proj} TestCrashIDL)


target_link_libraries(${proj} Generics Logger Geoip TestCrashIDL CORBACommons CORBAConfigParser)


add_test(NAME ${proj}
         COMMAND ${proj})
