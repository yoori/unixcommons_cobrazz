set(proj "HttpTestCommons")

add_library(${proj} SHARED
 CommonClasses.cpp
           ApachePorts.cpp
)
target_link_libraries(${proj}
  PRIVATE
    Generics
    Logger
    HTTP
    TestCommons
)

add_test(NAME ${proj}
         COMMAND ${proj})
