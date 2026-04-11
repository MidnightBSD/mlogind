# - Try to find FONTCONFIG
# Once done this will define
#
#  FONTCONFIG_FOUND - system has fontconfig
#  FONTCONFIG_INCLUDE_DIR - the fontconfig include directory
#  FONTCONFIG_LIBRARY - the fontconfig library
#  FONTCONFIG_LIBRARIES - the fontconfig libraries
#  FONTCONFIG_INCLUDE_DIRS - the fontconfig include directories

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_FONTCONFIG QUIET fontconfig)
endif()

find_path(FONTCONFIG_INCLUDE_DIR
    NAMES fontconfig/fontconfig.h
    HINTS ${PC_FONTCONFIG_INCLUDE_DIRS}
    PATH_SUFFIXES fontconfig
)

find_library(FONTCONFIG_LIBRARY
    NAMES fontconfig
    HINTS ${PC_FONTCONFIG_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FONTCONFIG
    REQUIRED_VARS FONTCONFIG_LIBRARY FONTCONFIG_INCLUDE_DIR
)

if(FONTCONFIG_FOUND)
    set(FONTCONFIG_LIBRARIES ${FONTCONFIG_LIBRARY})
    set(FONTCONFIG_INCLUDE_DIRS ${FONTCONFIG_INCLUDE_DIR})
endif()

mark_as_advanced(FONTCONFIG_INCLUDE_DIR FONTCONFIG_LIBRARY)
