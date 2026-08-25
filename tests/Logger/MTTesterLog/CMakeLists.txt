
set(proj "MTTesterExceptionsLog")

add_executable(${proj}
MTTesterLoggerTest.cpp



)


target_link_libraries(${proj} Generics Logger Sync)


add_test(NAME ${proj}
         COMMAND ${proj})
