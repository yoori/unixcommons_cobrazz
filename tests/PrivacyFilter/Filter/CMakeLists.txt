
set(proj "TestFilter")

add_executable(${proj}

Application.cpp

)


target_link_libraries(${proj} Generics Logger TestCommons2 PrivacyFilter)


add_test(NAME ${proj}
         COMMAND ${proj})
