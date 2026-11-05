set(proj "TestHTTPAddress")

add_executable(${proj}
Application.cpp

)
target_link_libraries(${proj}
  PRIVATE
    HTTP
)

add_test(NAME ${proj}
         COMMAND ${proj})

add_executable(TestHTTPUrlOptimization UrlOptimizationTest.cpp)
target_link_libraries(TestHTTPUrlOptimization PRIVATE HTTP)
add_test(NAME TestHTTPUrlOptimization COMMAND TestHTTPUrlOptimization)
