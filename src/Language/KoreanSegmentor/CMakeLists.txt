set(proj "KoreanSegmentor")
add_library(${proj}  SHARED
  Korean.cpp
  Klt.cpp
  Moran.cpp

)

target_link_libraries(${proj}
  PRIVATE
    Generics
)
