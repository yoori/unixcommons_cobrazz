add_library(HttpJavaFunctions SHARED
  HttpJavaFunctions.cpp
  HttpJavaFunctions.cpp
)

target_include_directories(HttpJavaFunctions
  PRIVATE ${JAVA_INCLUDE_DIRECTORIES}
)

target_link_libraries(HttpJavaFunctions
  Generics
  Stream
  HTTP
  BLogic
  GenericSegmentor
  Polyglot
)
