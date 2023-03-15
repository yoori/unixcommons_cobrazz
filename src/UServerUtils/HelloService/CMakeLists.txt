project(userver-samples-hello_service CXX)

add_executable(${PROJECT_NAME} "HelloService.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

userver_sample_testsuite_add()
