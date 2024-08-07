# Build libmariadb from source On *Nix, this is done when mariadb/mysql client
# system library is missing, and USE_MYSQL is not set to OFF
#
# On Windows, we build it by default, "thanks" to subpar quality of vcpkg's
# libmariadb/libmysql libraries

include(ExternalProject)
set(src_dir ${PROJECT_BINARY_DIR}/build/third_party/src/libmariadb)
set(build_dir ${PROJECT_BINARY_DIR}/build/third_party/libmariadb)
set(install_dir ${build_dir}/install)

if(WIN32)
  set(mariadbclient_LIBRARY_NAME libmariadb.dll)
else()
  set(mariadbclient_LIBRARY_NAME libmariadbclient.a)
endif()

set(mariadbclient_IMPORTED_LOCATION
    ${install_dir}/lib/mariadb/${mariadbclient_LIBRARY_NAME})

set(_EXTRA_CMAKE_ARGS -DCMAKE_COMPILE_WARNING_AS_ERROR=OFF)

if(WIN32)
  if(MINGW)
    set(import_suffix .dll.a)
  else()
    set(import_suffix .lib)
  endif()
  list(APPEND _EXTRA_CMAKE_ARGS -DCLIENT_PLUGIN_PVIO_NPIPE=STATIC)
  set(mariadbclient_IMPORTED_IMPLIB
      ${install_dir}/lib/mariadb/${CMAKE_SHARED_LIBRARY_PREFIX}libmariadb${import_suffix}
  )
endif()

if(MSVC)
list(APPEND  _EXTRA_CMAKE_ARGS
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE=ON
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO=ON
    -DCMAKE_POLICY_DEFAULT_CMP0069=NEW
)
ENDIF()

set(LIBMARIADB_GIT_TAG
    v3.3.11
    CACHE
      STRING
      "Git tag of mariadb client library. Set to empty string to get most recent revision"
)
if(LIBMARIADB_GIT_TAG)
  set(_GIT_TAG GIT_TAG ${LIBMARIADB_GIT_TAG})
endif()
find_package(ZLIB)
if(ZLIB_FOUND)
  list(APPEND _EXTRA_CMAKE_ARGS -DWITH_EXTERNAL_ZLIB=ON)
endif()

# pass down toolchain related variables (important for vcpkg)
foreach(varname CMAKE_TOOLCHAIN_FILE VCPKG_TARGET_TRIPLET VCPKG_HOST_TRIPLET)
  if(${varname})
    list(APPEND _EXTRA_CMAKE_ARGS "-D${varname}=${${varname}}")
  endif()
endforeach()

if(CMAKE_PREFIX_PATH)
  # Create a list with an alternate separator e.g. pipe symbol
  string(REPLACE ";" "|" CMAKE_PREFIX_PATH_ALT_SEP "${CMAKE_PREFIX_PATH}")
  list(APPEND _EXTRA_CMAKE_ARGS
       "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP}")
  list(APPEND _LIST_SEPARATOR LIST_SEPARATOR |)
else()
  set(_LIST_SEPARATOR)
endif()

ExternalProject_Add(
  buildLibmariadb
  GIT_REPOSITORY https://github.com/mariadb-corporation/mariadb-connector-c
  ${_GIT_TAG}
  SOURCE_DIR ${src_dir}
  BINARY_DIR ${build_dir}
  UPDATE_COMMAND ""
  PATCH_COMMAND ""
  BUILD_ALWAYS FALSE
  ${_LIST_SEPARATOR}
  CMAKE_ARGS -G${CMAKE_GENERATOR}
             -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_BUILD_TYPE=RelWithDebInfo
             -DCLIENT_PLUGIN_CACHING_SHA2_PASSWORD=STATIC
             -DCLIENT_PLUGIN_DIALOG=OFF
             -DCLIENT_PLUGIN_CLIENT_ED25519=OFF
             -DCLIENT_PLUGIN_SHA256_PASSWORD=STATIC
             -DCLIENT_PLUGIN_MYSQL_CLEAR_PASSWORD=OFF
             -DCLIENT_PLUGIN_ZSTD=OFF
             -DSKIP_TESTS=1
             -DCMAKE_DISABLE_FIND_PACKAGE_CURL=1
             -DCMAKE_INSTALL_PREFIX=${install_dir}
             ${_EXTRA_CMAKE_ARGS}
             -Wno-dev
             --no-warn-unused-cli
  BUILD_BYPRODUCTS ${mariadbclient_IMPORTED_LOCATION}
                   ${mariadbclient_IMPORTED_IMPLIB})
make_directory(${install_dir}/include/mariadb)
if(WIN32)
  add_library(mariadbclient SHARED IMPORTED GLOBAL)
  set_target_properties(
    mariadbclient
    PROPERTIES IMPORTED_LOCATION ${mariadbclient_IMPORTED_LOCATION}
               IMPORTED_IMPLIB ${mariadbclient_IMPORTED_IMPLIB}
               INTERFACE_INCLUDE_DIRECTORIES ${install_dir}/include/mariadb)
else()
  find_package(OpenSSL)
  if(NOT OpenSSL_FOUND)
    message(
      FATAL_ERROR
        "Could not find OpenSSL, which is required to build libmariadb from source"
    )
  endif()
  set(mariadbclient_INTERFACE_LINK_LIBRARIES m ${CMAKE_DL_LIBS} OpenSSL::SSL)
  if(ZLIB_FOUND)
    list(APPEND mariadbclient_INTERFACE_LINK_LIBRARIES ZLIB::ZLIB)
  endif()
  add_library(mariadbclient STATIC IMPORTED GLOBAL)
  set_target_properties(
    mariadbclient
    PROPERTIES IMPORTED_LOCATION ${mariadbclient_IMPORTED_LOCATION}
               INTERFACE_INCLUDE_DIRECTORIES ${install_dir}/include/mariadb
               INTERFACE_LINK_LIBRARIES
               "${mariadbclient_INTERFACE_LINK_LIBRARIES}")
endif()
add_dependencies(mariadbclient buildLibmariadb)
