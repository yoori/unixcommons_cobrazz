set(proj "TestCommons2")

add_library(${proj} SHARED
           CheckFileMessages.cpp
)
target_link_libraries(${proj}
  PRIVATE
    Generics
)
