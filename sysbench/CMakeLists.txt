# Copyright (C) 2008 MySQL AB
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
   cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(sysbench)
add_executable(sysbench
  sysbench.c 
  sysbench.h 
  sb_timer.c 
  sb_timer.h 
  sb_options.c 
  sb_options.h 
  sb_logger.c 
  sb_logger.h 
  sb_list.h 
  db_driver.h 
  db_driver.c
  sb_win.c
  sb_win.h
  tests/sb_cpu.h
  tests/sb_fileio.h
  tests/sb_memory.h
  tests/sb_threads.h
  tests/sb_mutex.h
  tests/sb_oltp.h
)
if(MSVC)
  #  Link C runtime statically to avoid hassle with CRT dll redistribution.
  STRING(REPLACE "/MD"  "/MT"  CMAKE_C_FLAGS_RELEASE          ${CMAKE_C_FLAGS_RELEASE})
  STRING(REPLACE "/MD"  "/MT"  CMAKE_C_FLAGS_RELWITHDEBINFO   ${CMAKE_C_FLAGS_RELWITHDEBINFO})
  STRING(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG            ${CMAKE_C_FLAGS_DEBUG})
  STRING(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG_INIT       ${CMAKE_C_FLAGS_DEBUG_INIT})

  STRING(REPLACE "/MD"  "/MT"  CMAKE_CXX_FLAGS_RELEASE        ${CMAKE_CXX_FLAGS_RELEASE})
  STRING(REPLACE "/MD"  "/MT"  CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
  STRING(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG          ${CMAKE_CXX_FLAGS_DEBUG})
  STRING(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG_INIT     ${CMAKE_CXX_FLAGS_DEBUG_INIT})
  
  #Silence "deprecated API" warnings.
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996")
  
  #### BEGIN MySQL ####
  #Look for mysql.h  in INCLUDE paths
  find_path(MYSQLH_PATH mysql.h  ENV INCLUDE)
  message("mysql.h directory = ${MYSQLH_PATH}")
  
  #Looks for libmysql.lib in LIB paths
  find_file(LIBMYSQL_LIB libmysql.lib ENV LIB)
  message("libmysql.lib = ${LIBMYSQL_LIB}")
  
  #Look for libmysql.dll
  find_file(LIBMYSQL_DLL libmysql.dll ENV LIB ENV PATH)
  message("libmysql.dll = ${LIBMYSQL_DLL}")
  
  #If mysql header file and client library are found, build with mysql
  if(MYSQLH_PATH AND LIBMYSQL_LIB)
    set(USE_MYSQL 1)
    
  #If libmysql.dll found, copy it next to sysbench.exe in the postbuild step
  if(LIBMYSQL_DLL)
   file(TO_NATIVE_PATH ${LIBMYSQL_DLL} LIBMYSQL_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBMYSQL_DLL} $(OutDir)
   )
  endif(LIBMYSQL_DLL)
  endif(MYSQLH_PATH AND LIBMYSQL_LIB)
  #### END MySQL ####
  
  #### BEGIN PgSQL ####
  #Look for libpq-fe.h  in INCLUDE paths
  find_path(PGSQLH_PATH libpq-fe.h  ENV INCLUDE)
  message("libpq-fe.h directory = ${PGSQLH_PATH}")
  
  #Looks for libpq.lib in LIB paths
  find_file(LIBPGSQL_LIB libpq.lib ENV LIB)
  message("libpq.lib = ${LIBPGSQL_LIB}")
  
  #Look for libpq.dll
  find_file(LIBPGSQL_DLL libpq.dll ENV LIB ENV PATH)
  message("libpq.dll = ${LIBPGSQL_DLL}")
  
  if(LIBPGSQL_DLL)
  #Look for ssleay.dll
  find_file(LIBPGSQL_SSLEAY_DLL ssleay.dll ENV LIB ENV PATH)
  message("ssleay.dll = ${LIBPGSQL_SSLEAY_DLL}")

  #Look for ssleay32.dll
  find_file(LIBPGSQL_SSLEAY32_DLL ssleay32.dll ENV LIB ENV PATH)
  message("ssleay32.dll = ${LIBPGSQL_SSLEAY32_DLL}")
  
  #Look for libeay.dll
  find_file(LIBPGSQL_LIBEAY_DLL libeay.dll ENV LIB ENV PATH)
  message("libeay.dll = ${LIBPGSQL_LIBEAY_DLL}")

  #Look for libeay32.dll
  find_file(LIBPGSQL_LIBEAY32_DLL libeay32.dll ENV LIB ENV PATH)
  message("libeay32.dll = ${LIBPGSQL_LIBEAY32_DLL}")
  
  #Look for comerr.dll
  find_file(LIBPGSQL_COMERR_DLL comerr.dll ENV LIB ENV PATH)
  message("comerr.dll = ${LIBPGSQL_COMERR_DLL}")

  #Look for comerr32.dll
  find_file(LIBPGSQL_COMERR32_DLL comerr32.dll ENV LIB ENV PATH)
  message("comerr32.dll = ${LIBPGSQL_COMERR32_DLL}")
  
  #Look for gssapi.dll
  find_file(LIBPGSQL_GSSAPI_DLL gssapi.dll ENV LIB ENV PATH)
  message("gssapi.dll = ${LIBPGSQL_GSSAPI_DLL}")

  #Look for gssapi32.dll
  find_file(LIBPGSQL_GSSAPI32_DLL gssapi32.dll ENV LIB ENV PATH)
  message("gssapi32.dll = ${LIBPGSQL_GSSAPI32_DLL}")
  
  #Look for k5sprt.dll
  find_file(LIBPGSQL_K5SPRT_DLL k5sprt.dll ENV LIB ENV PATH)
  message("k5sprt.dll = ${LIBPGSQL_K5SPRT_DLL}")

  #Look for k5sprt32.dll
  find_file(LIBPGSQL_K5SPRT32_DLL k5sprt32.dll ENV LIB ENV PATH)
  message("k5sprt32.dll = ${LIBPGSQL_K5SPRT32_DLL}")
  
  #Look for krb5_.dll
  find_file(LIBPGSQL_KRB5__DLL krb5_.dll ENV LIB ENV PATH)
  message("krb5_.dll = ${LIBPGSQL_KRB5__DLL}")

  #Look for krb5_32.dll
  find_file(LIBPGSQL_KRB5_32_DLL krb5_32.dll ENV LIB ENV PATH)
  message("krb5_32.dll = ${LIBPGSQL_KRB5_32_DLL}")
  
  #Look for libiconv2.dll
  find_file(LIBPGSQL_LIBICONV2_DLL libiconv2.dll ENV LIB ENV PATH)
  message("libiconv2.dll = ${LIBPGSQL_LIBICONV2_DLL}")
  
  #Look for libintl3.dll
  find_file(LIBPGSQL_LIBINTL3_DLL libintl3.dll ENV LIB ENV PATH)
  message("libintl3.dll = ${LIBPGSQL_LIBINTL3_DLL}")
  
  endif(LIBPGSQL_DLL)
  
  #If pgsql header file and client library are found, build with pgsql
  if(PGSQLH_PATH AND LIBPGSQL_LIB)
    set(USE_PGSQL 1)
    
  #If libpq.dll found, copy it next to sysbench.exe in the postbuild step
  if(LIBPGSQL_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_DLL} LIBPGSQL_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_DLL} $(OutDir)
   )
  endif(LIBPGSQL_DLL)
  
  if(LIBPGSQL_SSLEAY_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_SSLEAY_DLL} LIBPGSQL_SSLEAY_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_SSLEAY_DLL} $(OutDir)
   )
  endif(LIBPGSQL_SSLEAY_DLL)
  if(LIBPGSQL_SSLEAY32_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_SSLEAY32_DLL} LIBPGSQL_SSLEAY32_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_SSLEAY32_DLL} $(OutDir)
   )
  endif(LIBPGSQL_SSLEAY32_DLL)
  
  if(LIBPGSQL_LIBEAY_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_LIBEAY_DLL} LIBPGSQL_LIBEAY_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_LIBEAY_DLL} $(OutDir)
   )
  endif(LIBPGSQL_LIBEAY_DLL)
  if(LIBPGSQL_LIBEAY32_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_LIBEAY32_DLL} LIBPGSQL_LIBEAY32_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_LIBEAY32_DLL} $(OutDir)
   )
  endif(LIBPGSQL_LIBEAY32_DLL)
  
  if(LIBPGSQL_COMERR_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_COMERR_DLL} LIBPGSQL_COMERR_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_COMERR_DLL} $(OutDir)
   )
  endif(LIBPGSQL_COMERR_DLL)
  if(LIBPGSQL_COMERR32_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_COMERR32_DLL} LIBPGSQL_COMERR32_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_COMERR32_DLL} $(OutDir)
   )
  endif(LIBPGSQL_COMERR32_DLL)
  
  if(LIBPGSQL_GSSAPI_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_GSSAPI_DLL} LIBPGSQL_GSSAPI_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_GSSAPI_DLL} $(OutDir)
   )
  endif(LIBPGSQL_GSSAPI_DLL)
  if(LIBPGSQL_GSSAPI32_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_GSSAPI32_DLL} LIBPGSQL_GSSAPI32_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_GSSAPI32_DLL} $(OutDir)
   )
  endif(LIBPGSQL_GSSAPI32_DLL)
  
  if(LIBPGSQL_K5SPRT_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_K5SPRT_DLL} LIBPGSQL_K5SPRT_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_K5SPRT_DLL} $(OutDir)
   )
  endif(LIBPGSQL_K5SPRT_DLL)
  if(LIBPGSQL_K5SPRT32_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_K5SPRT32_DLL} LIBPGSQL_K5SPRT32_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_K5SPRT32_DLL} $(OutDir)
   )
  endif(LIBPGSQL_K5SPRT32_DLL)
  
  if(LIBPGSQL_KRB5__DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_KRB5__DLL} LIBPGSQL_KRB5__DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_KRB5__DLL} $(OutDir)
   )
  endif(LIBPGSQL_KRB5__DLL)
  if(LIBPGSQL_KRB5_32_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_KRB5_32_DLL} LIBPGSQL_KRB5_32_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_KRB5_32_DLL} $(OutDir)
   )
  endif(LIBPGSQL_KRB5_32_DLL)
  
  if(LIBPGSQL_LIBICONV2_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_LIBICONV2_DLL} LIBPGSQL_LIBICONV2_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_LIBICONV2_DLL} $(OutDir)
   )
  endif(LIBPGSQL_LIBICONV2_DLL)
  
  if(LIBPGSQL_LIBINTL3_DLL)
   file(TO_NATIVE_PATH ${LIBPGSQL_LIBINTL3_DLL} LIBPGSQL_LIBINTL3_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBPGSQL_LIBINTL3_DLL} $(OutDir)
   )
  endif(LIBPGSQL_LIBINTL3_DLL)
  
  endif(PGSQLH_PATH AND LIBPGSQL_LIB)
  #### END PgSQL ####
  
  #### BEGIN Oracle ####
  #Look for oci.h  in INCLUDE paths
  find_path(ORACLEH_PATH oci.h  ENV INCLUDE)
  message("oci.h directory = ${ORACLEH_PATH}")
  
  #Looks for oci.lib in LIB paths
  find_file(LIBORACLE_LIB oci.lib ENV LIB)
  message("oci.lib = ${LIBORACLE_LIB}")
  
  #Look for oci.dll
  find_file(LIBORACLE_DLL oci.dll ENV LIB ENV PATH)
  message("oci.dll = ${LIBORACLE_DLL}")
  
  #If oracle header file and client library are found, build with oracle
  if(ORACLEH_PATH AND LIBORACLE_LIB)
    set(USE_ORACLE 1)
    
  #If oci.dll found, copy it next to sysbench.exe in the postbuild step
  if(LIBORACLE_DLL)
   file(TO_NATIVE_PATH ${LIBORACLE_DLL} LIBORACLE_DLL)
   ADD_CUSTOM_COMMAND(
   TARGET sysbench
   POST_BUILD
   COMMAND copy /y ${LIBORACLE_DLL} $(OutDir)
   )
  endif(LIBORACLE_DLL)
  endif(ORACLEH_PATH AND LIBORACLE_LIB)
  #### END Oracle ####
endif(MSVC)


if(USE_MYSQL)
  message("using mysql driver")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_MYSQL")
  include_directories(${MYSQLH_PATH})
  add_subdirectory(drivers/mysql)
  target_link_libraries (sysbench sbmysql)
  target_link_libraries (sysbench ${LIBMYSQL_LIB})
endif(USE_MYSQL)

if(USE_PGSQL)
  message("using pgsql driver")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_PGSQL")
  include_directories(${PGSQLH_PATH})
  add_subdirectory(drivers/pgsql)
  target_link_libraries (sysbench sbpgsql)
  target_link_libraries (sysbench ${LIBPGSQL_LIB})
endif(USE_PGSQL)

if(USE_ORACLE)
  message("using oracle driver")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_ORACLE")
  include_directories(${ORACLEH_PATH})
  add_subdirectory(drivers/oracle)
  target_link_libraries (sysbench sboracle)
  target_link_libraries (sysbench ${LIBORACLE_LIB})
endif(USE_ORACLE)

add_subdirectory(tests/cpu)
target_link_libraries (sysbench sbcpu)

add_subdirectory(tests/fileio)
target_link_libraries (sysbench sbfileio)

add_subdirectory(tests/mutex)
target_link_libraries (sysbench sbmutex)

add_subdirectory(tests/threads)
target_link_libraries (sysbench sbthreads)

add_subdirectory(tests/memory)
target_link_libraries (sysbench sbmemory)

add_subdirectory(tests/oltp)
target_link_libraries (sysbench sboltp)
