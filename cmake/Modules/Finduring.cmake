if (uring_INCLUDE_DIR AND uring_LIBRARY)
  # Already in cache, be silent
  set(uring_FIND_QUIETLY TRUE)
endif (uring_INCLUDE_DIR AND uring_LIBRARY)

find_path(uring_INCLUDE_DIR liburing.h
  PATHS /usr/include
  PATH_SUFFIXES liburing
)

find_library(uring_LIBRARY
  NAMES uring
  PATHS /usr/lib /usr/local/lib
)

set(uring_LIBRARIES ${uring_LIBRARY} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(uring
  DEFAULT_MSG
  uring_INCLUDE_DIR
  uring_LIBRARIES
)

mark_as_advanced(uring_INCLUDE_DIR uring_LIBRARY)
