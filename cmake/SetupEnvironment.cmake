include_guard(GLOBAL)

# set base compile options
set(CPP_DEBUGINFO_FLAGS "-ggdb3")
# -Wno-address used for ignore ACE TAO warnings on (0 == &X) expressions : remove after TAO deprecate
set(CPP_COMMON_FLAGS "-Wno-deprecated-declarations -Wno-address -D_REENTRANT -m64 -march=x86-64 -DPIC -pthread -W -Wall -Werror -Wno-error=nonnull-compare -Wno-error=maybe-uninitialized")
set(CPP_DEBUG_OPT_FLAGS "-O0 -fno-inline -DDEV_DEBUG -rdynamic")
set(CPP_RELEASE_OPT_FLAGS "-O3")
set(CPP_LINKER_FLAGS "-pthread -W -Wall -m64 -march=x86-64")

set(CMAKE_C_FLAGS_DEBUG "${CPP_COMMON_FLAGS} ${CPP_DEBUGINFO_FLAGS} ${CPP_DEBUG_OPT_FLAGS}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CPP_COMMON_FLAGS} ${CPP_DEBUGINFO_FLAGS} ${CPP_RELEASE_OPT_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CPP_COMMON_FLAGS} ${CPP_RELEASE_OPT_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_RELWITHDEBINFO}")

set(CMAKE_CXX_FLAGS_DEBUG "${CPP_COMMON_FLAGS} ${CPP_DEBUGINFO_FLAGS} ${CPP_DEBUG_OPT_FLAGS} -fcoroutines")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CPP_COMMON_FLAGS} ${CPP_DEBUGINFO_FLAGS} ${CPP_RELEASE_OPT_FLAGS} -fcoroutines")
set(CMAKE_CXX_FLAGS_RELEASE "${CPP_COMMON_FLAGS} ${CPP_RELEASE_OPT_FLAGS} -fcoroutines")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS_RELWITHDEBINFO}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/libiconv/") # workaround for iconv installed to /usr/libiconv/
