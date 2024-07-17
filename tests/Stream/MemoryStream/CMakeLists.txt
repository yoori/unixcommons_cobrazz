
set(proj "TestMemoryStream")

add_executable(${proj}
Main.cpp

)


target_link_libraries(${proj} Generics Logger TestCommons2 Stream String)


add_test(NAME ${proj}
         COMMAND ${proj})
