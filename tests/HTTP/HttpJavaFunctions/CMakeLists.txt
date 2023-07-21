set(proj "TestHttpJavaFunctions")

add_executable(${proj}
  Application.cpp
)

target_link_libraries(${proj} HttpJavaFunctions)
