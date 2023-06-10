add_library(HttpJavaFunctions SHARED
  HttpJavaFunctions.cpp
  HttpJavaFunctions.cpp
)

target_include_directories(HttpJavaFunctions
  PRIVATE /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.362.b09-2.el8_7.x86_64/include/
    /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.362.b09-2.el8_7.x86_64/include/linux/
)

target_link_libraries(HttpJavaFunctions
  Generics
  Stream
  HTTP
  BLogic
  GenericSegmentor
  Polyglot
  )
