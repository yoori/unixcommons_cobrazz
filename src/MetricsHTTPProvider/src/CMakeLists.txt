#cmake_minimum_required (VERSION 2.6)




add_library(MetricsHTTPProvider  SHARED


MetricsHTTPProvider.cpp
config_service.cpp

)
include_directories(
${USERVER}/core/include 
${USERVER}
${USERVER}/core/src 
${USERVER}/shared/include 
${USERVER}/shared/src 
/usr/local/include
/usr/include/userver



                )

if(APPLE)
set(MACLINK "userver-stacktrace")
endif()
if(LINUX)
set(LINUXLINK "Generics")
endif()


target_link_libraries(MetricsHTTPProvider
        userver-core 
        boost_locale
        curl 
        lber
        ldap 
        userver-uboost-coro 
        boost_filesystem 
        boost_program_options 
        boost_iostreams 
        boost_regex 
        crypto 
        ssl 
        z 
        yaml-cpp 
        cctz 
        cryptopp 
        http_parser 
        ev 
        spdlog 
        fmt
        ${MACLINK}
        ${LINUXLINK}
        
        

z  pthread dl)


