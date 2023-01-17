set(proj "Stream")
add_library(${proj}  SHARED
  BinaryStream.cpp 
  BzlibStreams.cpp 
  FileStreamBuf.cpp 
  GzipStreams.cpp 
  SocketStream.cpp 

)
target_link_libraries(${proj} z bz2 ACE)
install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})


