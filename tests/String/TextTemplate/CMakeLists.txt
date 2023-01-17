
set(proj "TestTextTemplate")


add_executable(${proj}
TestTemplate.cpp
)


target_link_libraries(${proj} Generics Logger TestCommons2 String)
add_test(NAME ${proj}
         COMMAND ${proj})


