if(WIN32)
  # On Windows, we just use own header-only library
  if(NOT TARGET ConcurrencyKit)
    add_library(ConcurrencyKit INTERFACE)
    target_include_directories(
      ConcurrencyKit INTERFACE ${PROJECT_SOURCE_DIR}/src/win/ck
                               ${PROJECT_SOURCE_DIR}/src/win/pthread)
  endif()
  set(ConcurrencyKit_FOUND TRUE)
  return()
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CK QUIET ck)
endif()

find_path(
  CK_INCLUDE_DIR
  NAMES ck_md.h
  PATHS ${PC_CK_INCLUDE_DIRS}
  PATH_SUFFIXES ck)
find_library(
  CK_LIBRARY
  NAMES ck
  PATHS ${PC_CK_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  ConcurrencyKit
  FOUND_VAR ConcurrencyKit_FOUND
  REQUIRED_VARS CK_LIBRARY CK_INCLUDE_DIR)
mark_as_advanced(CK_LIBRARY CK_INCLUDE_DIR)
if(ConcurrencyKit_FOUND AND NOT TARGET ConcurrencyKit)
  add_library(ConcurrencyKit UNKNOWN IMPORTED)
  set_target_properties(
    ConcurrencyKit PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CK_INCLUDE_DIR}
                              IMPORTED_LOCATION ${CK_LIBRARY})
endif()
