set(proj "PlainStorage")

add_library(${proj}  SHARED
  BlockFileAdapter.cpp
  Map.cpp

)
target_link_libraries(${proj}
  PRIVATE
    Generics
)
