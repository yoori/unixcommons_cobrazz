
set(proj "TestIsProperty")


add_executable(${proj}
IsProperty.cpp
)


target_link_libraries(${proj} Generics Logger TestCommons2 String)
add_test(NAME ${proj}
         COMMAND ${proj})


