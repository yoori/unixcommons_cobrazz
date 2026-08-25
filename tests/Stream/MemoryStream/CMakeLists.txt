
set(proj "TestMemoryStream")

add_executable(${proj}
Main.cpp

)




add_test(NAME ${proj}
         COMMAND ${proj})
