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
    ZLIB::ZLIB
    BZip2::BZip2
)
