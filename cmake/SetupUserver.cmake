if (${USE_USERVER_PACKAGE})
  if (NOT userver_FOUND)
    if (TARGET c-ares)
      add_library(c-ares::cares ALIAS c-ares)
    endif()
    find_package(userver REQUIRED)
    target_include_directories(userver::core
      INTERFACE /usr/include/userver/
    )
    add_library(userver-core ALIAS userver::core)
    add_library(userver-grpc ALIAS userver::grpc)
    add_library(userver-api-common-protos ALIAS userver::api-common-protos)
  endif()
endif()
