cmake_minimum_required(VERSION 3.12..3.26)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
if(APPLE)
  # Add homebrew search paths on Mac
  foreach(pkg openssl mysql-client mariadb-connector-c libpq)
    set(pkgpath /usr/local/opt/${pkg})
    if(EXISTS ${pkgpath})
      list(APPEND CMAKE_PREFIX_PATH ${pkg})
    endif()
  endforeach()
endif()

project(
  sysbench
  VERSION "1.1.0"
  LANGUAGES C)

# Set default build type, if none is given
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(default_build_type Release)
  message(
    STATUS
      "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE
      "${default_build_type}"
      CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()
include(compile_flags)
include(sanitizer)
add_subdirectory(src)
