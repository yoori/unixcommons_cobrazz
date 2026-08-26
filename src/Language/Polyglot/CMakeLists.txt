set(proj "Polyglot")
add_library(${proj} SHARED
DictionaryLoader.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
)
