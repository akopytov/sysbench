# This comes from https://github.com/Kitware/VTK/blob/master/CMake/FindMySQL.cmake

#[==[
Provides the following variables:

  * `MySQL_INCLUDE_DIRS`: Include directories necessary to use MySQL.
  * `MySQL_LIBRARIES`: Libraries necessary to use MySQL.
  * A `MySQL::MySQL` imported target.
#]==]

# No .pc files are shipped with MySQL on Windows.
set(_MySQL_use_pkgconfig 0)
if (NOT WIN32)
  find_package(PkgConfig)
  if (PkgConfig_FOUND)
    set(_MySQL_use_pkgconfig 1)
  endif ()
endif ()

if (_MySQL_use_pkgconfig)
  pkg_check_modules(_libmariadb "libmariadb" QUIET IMPORTED_TARGET)
  unset(_mysql_target)
  if (_libmariadb_FOUND)
    set(_mysql_target "_libmariadb")
  else ()
    pkg_check_modules(_mariadb "mariadb" QUIET IMPORTED_TARGET)
    if (NOT _mariadb_FOUND)
      pkg_check_modules(_mysql "mysql" QUIET IMPORTED_TARGET)
      if (_mysql_FOUND)
        set(_mysql_target "_mysql")
      endif ()
    else ()
      set(_mysql_target "_mariadb")
      if (_mariadb_VERSION VERSION_LESS 10.4)
        get_property(_include_dirs
          TARGET    "PkgConfig::_mariadb"
          PROPERTY  "INTERFACE_INCLUDE_DIRECTORIES")
        # Remove "${prefix}/mariadb/.." from the interface since it breaks other
        # projects.
        list(FILTER _include_dirs EXCLUDE REGEX "\\.\\.")
        set_property(TARGET "PkgConfig::_mariadb"
          PROPERTY
            "INTERFACE_INCLUDE_DIRECTORIES" "${_include_dirs}")
        unset(_include_dirs)
      endif ()
    endif ()
  endif ()

  set(MySQL_FOUND 0)
  if (_mysql_target)
    set(MySQL_FOUND 1)
    set(MySQL_INCLUDE_DIRS ${${_mysql_target}_INCLUDE_DIRS})
    set(MySQL_LIBRARIES ${${_mysql_target}_LINK_LIBRARIES})
    if (NOT TARGET MySQL::MySQL)
      add_library(MySQL::MySQL INTERFACE IMPORTED)
      target_link_libraries(MySQL::MySQL
        INTERFACE "PkgConfig::${_mysql_target}")
    endif ()
  endif ()
  unset(_mysql_target)
  unset(_MySQL_use_pkgconfig)
  if (TARGET MySQL::MySQL)
    return()
  endif()
endif()


set(_MySQL_mariadb_versions 10.2 10.3)
set(_MySQL_versions 5.0)
set(_MySQL_paths)
foreach (_MySQL_version IN LISTS _MySQL_mariadb_versions)
  list(APPEND _MySQL_paths
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MariaDB ${_MySQL_version};INSTALLDIR]"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MariaDB ${_MySQL_version} (x64);INSTALLDIR]")
endforeach ()
foreach (_MySQL_version IN LISTS _MySQL_versions)
  list(APPEND _MySQL_paths
    "C:/Program Files/MySQL/MySQL Server ${_MySQL_version}/lib/opt"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server ${_MySQL_version};Location]"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server ${_MySQL_version};Location]")
endforeach ()
unset(_MySQL_version)
unset(_MySQL_versions)
unset(_MySQL_mariadb_versions)
find_path(MySQL_INCLUDE_DIR
  NAMES mysql.h
  PATHS
    "C:/Program Files/MySQL/include"
    "C:/MySQL/include"
    ${_MySQL_paths}
  PATH_SUFFIXES include include/mysql
  DOC "Location of mysql.h")
mark_as_advanced(MySQL_INCLUDE_DIR)
find_library(MySQL_LIBRARY
  NAMES libmariadb mysql libmysql mysqlclient
  PATHS
    "C:/Program Files/MySQL/lib"
    "C:/MySQL/lib/debug"
    ${_MySQL_paths}
  PATH_SUFFIXES lib lib/opt
  DOC "Location of the mysql library")

if (MySQL_LIBRARY MATCHES "libmariadb.lib$")
  set(MYSQL_DLL libmariadb.dll)
elseif (MYSQL_LIBRARY MATCHES "libmysql.lib$")
  set(MYSQL_DLL libmysql.dll)
endif()
if(MYSQL_DLL)
  find_file(MYSQL_DLL_PATH
    NAMES ${MYSQL_DLL}
    PATHS
    PATH_SUFFIXES ../../bin
  )
  set(MYSQL_DLL_PATH_VAR MYSQL_DLL_PATH)
ENDIF()
mark_as_advanced(MySQL_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL
  REQUIRED_VARS MySQL_INCLUDE_DIR MySQL_LIBRARY ${MYSQL_DLL_PATH_VAR})

if (MySQL_FOUND)
  set(MySQL_INCLUDE_DIRS "${MySQL_INCLUDE_DIR}")
  set(MySQL_LIBRARIES "${MySQL_LIBRARY}")
  if (NOT TARGET MySQL::MySQL)
    if (MYSQL_DLL_PATH)
      add_library(MySQL::MySQL SHARED IMPORTED)
      set_target_properties(MySQL::MySQL PROPERTIES
        IMPORTED_IMPLIB "${MySQL_LIBRARY}"
        IMPORTED_LOCATION "${MYSQL_DLL_PATH}"
        INTERFACE_INCLUDE_DIRECTORIES "${MySQL_INCLUDE_DIR}")
    else()
      add_library(MySQL::MySQL UNKNOWN IMPORTED)
      set_target_properties(MySQL::MySQL PROPERTIES
        IMPORTED_LOCATION "${MySQL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MySQL_INCLUDE_DIR}")
    endif()
  endif()
endif()
unset(_MySQL_use_pkgconfig)
