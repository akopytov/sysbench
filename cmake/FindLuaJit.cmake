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

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LuaJit_FOUND to TRUE if
# all listed variables exist
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LuaJit
  REQUIRED_VARS LUA_LIBRARY LUA_INCLUDE_DIR)

MARK_AS_ADVANCED(LUA_INCLUDE_DIR LUA_LIBRARY)

if (LuaJit_FOUND AND NOT TARGET luajit::libluajit)
  add_library(luajit::libluajit UNKNOWN IMPORTED)
  set_target_properties(luajit::libluajit PROPERTIES
    IMPORTED_LOCATION "${LUA_LIBRARY}"
    INTERFACE_LINK_LIBRARIES "m;${CMAKE_DL_LIBS}" # Linuxism?
  set_target_properties(luajit::libluajit PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}")
endif()
