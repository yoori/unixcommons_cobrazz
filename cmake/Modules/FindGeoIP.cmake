#
# - Find GeoIP
# Find the native GeoIP includes and library
#
#  GeoIP_INCLUDE_DIRS - where to find GeoIP.h, etc.
#  GeoIP_LIBRARIES    - List of libraries when using GeoIP.
#  GeoIP_FOUND        - True if GeoIP found.

if(GeoIP_INCLUDE_DIRS)
  # Already in cache, be silent
  set(GeoIP_FIND_QUIETLY TRUE)
endif()

# Users may set the (environment) variable GeoIP_ROOT
# to point cmake to the *root* of a directory with include
# and lib subdirectories for GeoIP
if(GeoIP_ROOT)
  set(GeoIP_ROOT PATHS ${GeoIP_ROOT} NO_DEFAULT_PATH)
else()
  set(GeoIP_ROOT $ENV{GeoIP_ROOT})
endif()

find_package(PkgConfig)
pkg_search_module(GeoIP geoip)

# Find the header
find_path(GeoIP_INCLUDE_DIR GeoIP.h
  HINTS
    "${GeoIP_INCLUDEDIR}"
    "${GeoIP_ROOT}"
  PATH_SUFFIXES include Include
)

# Find the library
find_library(GeoIP_LIBRARY
  NAMES GeoIP libGeoIP-1
  HINTS
    "${GeoIP_LIBDIR}"
    "${GeoIP_ROOT}"
  PATH_SUFFIXES lib
)

# handle the QUIETLY and REQUIRED arguments and set GeoIP_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GeoIP DEFAULT_MSG GeoIP_LIBRARY GeoIP_INCLUDE_DIR)

if(GeoIP_FOUND)
  set(GeoIP_LIBRARIES ${GeoIP_LIBRARY})
  set(GeoIP_INCLUDE_DIRS ${GeoIP_INCLUDE_DIR})
endif()

mark_as_advanced(GeoIP_LIBRARIES GeoIP_INCLUDE_DIRS)
