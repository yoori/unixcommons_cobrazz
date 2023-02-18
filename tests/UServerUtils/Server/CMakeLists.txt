
set(proj "TestServer")

add_executable(${proj} 
           TestServer.cpp
)



target_link_libraries(${proj} Generics Logger UServerUtils)


add_test(NAME ${proj}
         COMMAND ${proj})

