add_executable(
  sysbench
  db_driver.c
  db_driver.h
  sb_barrier.c
  sb_barrier.h
  sb_ck_pr.h
  sb_counter.c
  sb_counter.h
  sb_global.h
  sb_histogram.c
  sb_histogram.h
  sb_list.h
  sb_logger.c
  sb_logger.h
  sb_lua.c
  sb_lua.h
  sb_options.c
  sb_options.h
  sb_rand.c
  sb_rand.h
  sb_thread.c
  sb_thread.h
  sb_timer.c
  sb_timer.h
  sb_util.c
  sb_util.h
  sysbench.c
  sysbench.h
  tests/sb_cpu.h
  tests/cpu/sb_cpu.c
  tests/sb_fileio.h
  tests/fileio/sb_fileio.c
  tests/fileio/crc32.c
  tests/fileio/crc32.h
  tests/fileio/crc32tbl.h
  tests/sb_memory.h
  tests/memory/sb_memory.c
  tests/sb_mutex.h
  tests/mutex/sb_mutex.c
  tests/sb_threads.h
  tests/threads/sb_threads.c
  xoroshiro128plus.h
  config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/config.h
                            PROPERTIES GENERATED 1)

target_include_directories(sysbench PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
                                            ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(sysbench PRIVATE -DHAVE_CONFIG_H)
target_compile_features(sysbench PRIVATE c_std_99)
add_subdirectory(lua/internal)

set_target_properties(sysbench PROPERTIES ENABLE_EXPORTS 1)
if(WIN32)
  # Use pthreads port from Lockless.inc
  add_subdirectory(win/pthread)
  add_subdirectory(win)
else()
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  target_link_libraries(sysbench PRIVATE Threads::Threads)
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
endif()
include(systemchecks)
set(PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
set(PACKAGE ${CMAKE_PROJECT_NAME})
include(GNUInstallDirs)
set(DATADIR
    ${CMAKE_INSTALL_FULL_DATADIR}/${CMAKE_PROJECT_NAME}
    CACHE STRING "full path to pkgdatadir")
set(LIBDIR
    ${CMAKE_INSTALL_FULL_LIBDIR}/${CMAKE_PROJECT_NAME}
    CACHE STRING "full path to pkglibdir")

# Use LuaJit library, either as system library or build from bundled source
set(WITH_SYSTEM_LUAJIT
    OFF
    CACHE BOOL "Use system-provided LuaJIT headers and library")
if(WITH_SYSTEM_LUAJIT)
  # Find system library
  find_package(LuaJit REQUIRED)
  set(SB_WITH_LUAJIT "system")
else()
  # Build from bundled source
  include(BuildLuaJit)
  set(SB_WITH_LUAJIT "bundled")
endif()
target_link_libraries(sysbench PRIVATE luajit::libluajit)

# On Windows, we prefer libmariadb built from source (as external project) to
# provide better named pipe support On *nix, we prefer system
# libmysql/libmariadb
if(WIN32)
  set(_WITH_LIBMARIADB_DEFAULT ON)
else()
  set(_WITH_LIBMARIADB_DEFAULT OFF)
endif()
set(WITH_LIBMARIADB
    ${_WITH_LIBMARIADB_DEFAULT}
    CACHE BOOL "Build libmariadb from source (as external project)")
if(WITH_LIBMARIADB)
  include(BuildLibmariadb)
  target_link_libraries(sysbench PRIVATE mariadbclient)
endif()

set(WITH_MYSQL
    ON
    CACHE BOOL "Enable mysql support for database tests")
if(WITH_MYSQL AND (NOT WITH_LIBMARIADB))
  find_package(MySQL REQUIRED)
  target_link_libraries(sysbench PRIVATE MySQL::MySQL)
  set(CMAKE_REQUIRED_INCLUDES ${MySQL_INCLUDE_DIRS})
  include(CheckCSourceCompiles)
  check_c_source_compiles(
    "
    #include <mysql.h>
    int main()
    {
      return (int)MYSQL_OPT_SSL_MODE;
    }"
    HAVE_MYSQL_OPT_SSL_MODE)
endif()

if(WITH_MYSQL OR WITH_LIBMARIADB)
  target_compile_definitions(sysbench PRIVATE -DUSE_MYSQL)
  target_sources(sysbench PRIVATE drivers/mysql/drv_mysql.c)
endif()

# Postgres driver
set(WITH_PGSQL
    OFF
    CACHE BOOL "Whether to use postgres driver.")
if(WITH_PGSQL)
  # workaround CMake issue # 17223 (non-essential header lookup)
  set(PostgreSQL_TYPE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  find_package(PostgreSQL REQUIRED)
  target_sources(sysbench PRIVATE drivers/pgsql/drv_pgsql.c)
  target_compile_definitions(sysbench PRIVATE -DUSE_PGSQL)
  target_link_libraries(sysbench PRIVATE PostgreSQL::PostgreSQL)
endif()

set(WITH_SYSTEM_CONCURRENCY_KIT
    OFF
    CACHE BOOL "Use concurrency kit library provided by system")
if(WITH_SYSTEM_CONCURRENCY_KIT OR WIN32)
  find_package(ConcurrencyKit REQUIRED)
else()
  include(BuildConcurrencyKit)
endif()
add_dependencies(sysbench ConcurrencyKit)
target_link_libraries(sysbench PRIVATE ConcurrencyKit)

# Find git hash, if built from git
include(githash)
githash(SB_GIT_SHA)
if(SB_GIT_SHA)
  set(SB_GIT_SHA "-${SB_GIT_SHA}")
endif()

configure_file(config.h.in config.h @ONLY)

# Installation related stuff
add_subdirectory(lua)


# Install tests
if(WITH_MYSQL)
  set(USE_MYSQL 1)
endif()
if(WITH_PGSQL)
  set(USE_MYSQL 1)
endif()

configure_file(${PROJECT_SOURCE_DIR}/tests/include/config.sh.in
               ${PROJECT_BINARY_DIR}/tests/include/config.sh @ONLY)
install(
  FILES ${PROJECT_BINARY_DIR}/tests/include/config.sh
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/tests/include)

install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/tests
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}
  USE_SOURCE_PERMISSIONS FILES_MATCHING
  PATTERN "*.t"
  PATTERN "*.sh"
  PATTERN "*.md"
  PATTERN "*.lua")

if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
  # make test is broken on out-of-source
  add_custom_target(
    test
    COMMAND sh -c ./test_run.sh
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests)
endif()

install(
  TARGETS sysbench
  EXPORT sysbenchTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
  EXPORT sysbenchTargets
  FILE sysbenchTargets.cmake
  NAMESPACE sysbench::
  DESTINATION share/sysbench/cmake)

include(CMakePackageConfigHelpers)
# generate the config file that includes the exports
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/sysbenchConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/sysbenchConfig.cmake
  INSTALL_DESTINATION share/sysbench/cmake
  NO_SET_AND_CHECK_MACRO NO_CHECK_REQUIRED_COMPONENTS_MACRO)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/sysbenchConfigVersion.cmake
  COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbenchConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/sysbenchConfigVersion.cmake
        DESTINATION share/sysbench/cmake)
include(CPack)
