# TODO check that the right version is installed? Do we need 3.0.0?
# - Find the CUDD decision diagram library.
# This code defines the following variables:
#
#  CUDD_FOUND                 - TRUE if CUDD was found.
#  CUDD_INCLUDE_DIRS          - Full paths to all include dirs.
#  CUDD_LIBRARIES             - Full paths to all libraries.
#
# Usage:
#  find_package(cudd)
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).
#
# CUDD can be installed for example from here:
# https://github.com/ivmai/cudd
# 
# Important: for a 32bit build, CUDD needs to be configured as follows:
# ./configure --enable-shared --enable-obj --build=x86-pc-linux-gnu "CFLAGS=-m32 -Wall -Wextra -g -O3" "CXXFLAGS=-m32 -Wall -Wextra -g -O3" "LDFLAGS=-m32"
#
# Also, don't forget to run make install.

foreach(BUILDMODE "RELEASE" "DEBUG")
    set(_CMAKE_FIND_ROOT_PATH_${BUILDMODE} ${CMAKE_FIND_ROOT_PATH})

    find_path(CUDD_INCLUDE_DIRS
        NAMES cuddObj.hh
        HINTS ${_CMAKE_FIND_ROOT_PATH_${BUILDMODE}}
        PATH_SUFFIXES cudd/cplusplus include cudd cplusplus
    )

    find_library(CUDD_LIBRARY_${BUILDMODE}
        NAMES cudd
        HINTS ${_CMAKE_FIND_ROOT_PATH_${BUILDMODE}}
        PATH_SUFFIXES cudd/cudd/lib lib
    )
endforeach()

set(CUDD_LIBRARIES optimized ${CUDD_LIBRARY_RELEASE}
    debug ${CUDD_LIBRARY_DEBUG})

if (CUDD_INCLUDE_DIRS)
    get_filename_component(CUDD_DIR ${CUDD_INCLUDE_DIRS} NAME)
    if ("${CUDD_DIR}" STREQUAL "cplusplus")
        # this is needed for a manually installed CUDD, where header files are 
        # distributed across several directories
        get_filename_component(CUDD_ROOT_DIR ${CUDD_INCLUDE_DIRS} DIRECTORY)

        set(CUDD_CUDD_DIR ${CUDD_ROOT_DIR}/cudd)
        set(CUDD_ST_DIR ${CUDD_ROOT_DIR}/st)
        set(CUDD_MTR_DIR ${CUDD_ROOT_DIR}/mtr)
        set(CUDD_EPD_DIR ${CUDD_ROOT_DIR}/epd)

        set(CUDD_INCLUDE_DIRS ${CUDD_ROOT_DIR} ${CUDD_CUDD_DIR} ${CUDD_ST_DIR} ${CUDD_EPD_DIR} ${CUDD_MTR_DIR} ${CUDD_INCLUDE_DIRS})
    endif()
endif()


# Check if everything was found and set CUDD_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Cudd
    REQUIRED_VARS CUDD_INCLUDE_DIRS CUDD_LIBRARIES
)

mark_as_advanced(
    CUDD_LIBRARY_RELEASE CUDD_LIBRARY_DEBUG CUDD_INCLUDE_DIRS CUDD_SOURCE_DIRS CUDD_LIBRARIES
    CUDD_CUDD_DIR CUDD_ST_DIR CUDD_MTR_DIR CUDD_EPD_DIR
)
