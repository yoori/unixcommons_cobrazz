include(FindPackageHandleStandardArgs)

find_path(Nghttp2_INCLUDE_DIR "nghttp2/nghttp2.h")

find_library(Nghttp2_LIBRARY NAMES nghttp2)

find_package_handle_standard_args(Nghttp2
    FOUND_VAR
      Nghttp2_FOUND
    REQUIRED_VARS
      Nghttp2_LIBRARY
      Nghttp2_INCLUDE_DIR
)

set(Nghttp2_INCLUDE_DIRS ${Nghttp2_INCLUDE_DIR})
set(Nghttp2_LIBRARIES ${Nghttp2_LIBRARY})

mark_as_advanced(Nghttp2_INCLUDE_DIRS Nghttp2_LIBRARIES)

add_library(Nghttp2 SHARED IMPORTED)

set_property(TARGET Nghttp2 PROPERTY IMPORTED_LOCATION "${Nghttp2_LIBRARY}")

set_target_properties(Nghttp2 PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS ""
  INTERFACE_INCLUDE_DIRECTORIES "${Nghttp2_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "${Nghttp2_LIBRARIES}"
)

# INTERFACE_LINK_DIRECTORIES "${Nghttp2_LIBRARY_DIRS}"
