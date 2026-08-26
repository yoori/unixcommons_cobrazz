add_library(HttpJavaFunctions SHARED
  HttpJavaFunctions.cpp
)

target_include_directories(HttpJavaFunctions
  PRIVATE
    ${JNI_INCLUDE_DIRS}
)

target_link_libraries(HttpJavaFunctions
  PRIVATE
    Generics
    HTTP
    BLogic
    GenericSegmentor
    Polyglot
)
