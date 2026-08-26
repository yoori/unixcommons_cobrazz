set(proj "BLogic")

add_library(${proj}  SHARED
NormalizeTrigger.cpp

)

target_link_libraries(${proj}
  PRIVATE
    Generics
)
