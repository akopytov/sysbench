# Copyright (C) 2008 MySQL AB
# Copyright (C) 2008-2017 Alexey Kopytov <akopytov@gmail.com>
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

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
  sb_histogram.c
  sb_histogram.h
  sb_list.h 
  db_driver.h 
  db_driver.c
  sb_win.c
  sb_win.h
  sb_rand.c
  sb_rand.h
  sb_thread.c
  sb_thread.h
  sb_barrier.c
  sb_barrier.h
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
endif(MSVC)


if(USE_MYSQL)
  message("using mysql driver")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_MYSQL")
  include_directories(${MYSQLH_PATH})
  add_subdirectory(drivers/mysql)
  target_link_libraries (sysbench sbmysql)
  target_link_libraries (sysbench ${LIBMYSQL_LIB})
endif(USE_MYSQL)

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
