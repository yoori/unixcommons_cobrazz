set(proj "Stream")
add_library(${proj}  SHARED
  BinaryStream.cpp
  BzlibStreams.cpp
  FileStreamBuf.cpp
  GzipStreams.cpp
)
target_link_libraries(${proj}
  PRIVATE
    ZLIB::ZLIB
    BZip2::BZip2
)
