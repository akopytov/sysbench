# Locate LuaJIT library
# This module defines
#  LUAJIT_FOUND, if false, do not try to link to Lua
#  LUA_LIBRARY, where to find the lua library
#  LUA_INCLUDE_DIR, where to find lua.h
#
# This module is similar to FindLua51.cmake except that it finds LuaJit instead.

FIND_PATH(LUA_INCLUDE_DIR luajit.h
  HINTS
  $ENV{LUA_DIR}
  PATH_SUFFIXES include/luajit-2.1 include/luajit-2.0 include/luajit-5_1-2.1 include/luajit-5_1-2.0 include luajit
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

# Test if running on vcpkg toolchain
if(WIN32 OR (DEFINED VCPKG_TARGET_TRIPLET AND DEFINED VCPKG_APPLOCAL_DEPS))
  # On vcpkg luajit is 'lua51' and normal lua is 'lua'
  FIND_LIBRARY(LUA_LIBRARY
    NAMES lua51
    HINTS
    $ENV{LUA_DIR}
    PATH_SUFFIXES lib
  )
else()
  FIND_LIBRARY(LUA_LIBRARY
    NAMES
    luajit-5.1
    HINTS
    $ENV{LUA_DIR}
    PATH_SUFFIXES lib64 lib
    PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /sw
    /opt/local
    /opt/csw
    /opt
  )
endif()

IF(WIN32)
  FIND_FILE(LUA_LIBRARY_DLL
    NAMES lua51.dll
    PATH_SUFFIXES ../../bin
  )
  set(LUA_LIBRARY_DLL_VAR LUA_LIBRARY_DLL)
else()
  set(LUA_LIBRARY_DLL_VAR)
endif()

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LuaJit_FOUND to TRUE if
# all listed variables exist
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LuaJit
  REQUIRED_VARS LUA_LIBRARY LUA_INCLUDE_DIR ${LUA_LIBRARY_DLL_VAR})

MARK_AS_ADVANCED(LUA_INCLUDE_DIR LUA_LIBRARY ${LUA_LIBRARY_DLL_VAR})

if (LuaJit_FOUND AND NOT TARGET luajit::libluajit)
  if(WIN32)
    add_library(luajit::libluajit SHARED IMPORTED)
    get_filename_component(LUA_LIBRARY_DLL "${LUA_LIBRARY_DLL}" REALPATH)
    set_target_properties(luajit::libluajit PROPERTIES
      IMPORTED_LOCATION "${LUA_LIBRARY_DLL}"
      IMPORTED_IMPLIB "${LUA_LIBRARY}")
  else()
    add_library(luajit::libluajit UNKNOWN IMPORTED)
    set_target_properties(luajit::libluajit PROPERTIES
      IMPORTED_LOCATION "${LUA_LIBRARY}"
      INTERFACE_LINK_LIBRARIES "m;${CMAKE_DL_LIBS}" # Linuxism?
    )
  endif()
  set_target_properties(luajit::libluajit PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}")
endif()
