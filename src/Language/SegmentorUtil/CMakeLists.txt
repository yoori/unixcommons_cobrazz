add_executable(LinguistUtil
  Application.cpp
)

target_link_libraries(LinguistUtil
  PRIVATE
    Generics
    BLogic
    GenericSegmentor
    Polyglot
)
