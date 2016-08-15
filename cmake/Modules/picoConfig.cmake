INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_PICO pico)

FIND_PATH(
    PICO_INCLUDE_DIRS
    NAMES pico/api.h
    HINTS $ENV{PICO_DIR}/include
        ${PC_PICO_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    PICO_LIBRARIES
    NAMES gnuradio-pico
    HINTS $ENV{PICO_DIR}/lib
        ${PC_PICO_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PICO DEFAULT_MSG PICO_LIBRARIES PICO_INCLUDE_DIRS)
MARK_AS_ADVANCED(PICO_LIBRARIES PICO_INCLUDE_DIRS)

