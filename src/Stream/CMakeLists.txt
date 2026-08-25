set(proj "Stream")
add_library(${proj}  SHARED
  BinaryStream.cpp
  BzlibStreams.cpp
  FileStreamBuf.cpp
  GzipStreams.cpp
  SocketStream.cpp

)
target_link_libraries(${proj}
  PUBLIC
    ACE::ACE
  PRIVATE
    z
    bz2
)
#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})
