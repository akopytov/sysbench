# Build the bundled luajit library and create imported library.
# luajit::libluajit.
#
# This follows the instructions on the luajit page Both Unix and MSVC build are
# done in-source, with "make" rsp batch file On *Nix, luajit::libluajit is a
# static library,on Windows its a  DLL (this has to be so)
include(ExternalProject)

set(build_dir ${PROJECT_BINARY_DIR}/build/third_party/luajit)
set(install_dir ${build_dir}/install)
set(url ${PROJECT_SOURCE_DIR}/third_party/luajit/luajit)

if(NOT WIN32)
  # Note : luajit's Makefile is non-portable (GNU)it could fail on BSD, unless
  # gmake is there´
  find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)
  # Makefile requires MACOSX_DEPLOYMENT_TARGET on Apple
  if (APPLE)
    if("${CMAKE_OSX_DEPLOYMENT_TARGET}")
      set(MACOSX_DEPLOYMENT_TARGET
        "MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    else()
      message(WARNING "CMAKE_OSX_DEPLOYMENT_TARGET was not set")
      set(MACOSX_DEPLOYMENT_TARGET
        "MACOSX_DEPLOYMENT_TARGET=10.12")
    endif()
  endif()
  set(libluajit_static_lib ${install_dir}/lib/libluajit-5.1.a)
  ExternalProject_Add(
    buildLuaJit
    URL ${url}
    SOURCE_DIR ${build_dir}
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_ALWAYS FALSE
    BUILD_COMMAND  ${MAKE_EXECUTABLE} install PREFIX=${install_dir} ${MACOSX_DEPLOYMENT_TARGET}
    BUILD_BYPRODUCTS ${libluajit_static_lib})
  add_library(luajit::libluajit STATIC IMPORTED)
  make_directory(${install_dir}/include/luajit-2.1)
  set_target_properties(
    luajit::libluajit
    PROPERTIES IMPORTED_LOCATION
               ${libluajit_static_lib}
               INTERFACE_INCLUDE_DIRECTORIES
               ${install_dir}/include/luajit-2.1
               INTERFACE_LINK_LIBRARIES "m;${CMAKE_DL_LIBS}")
else()
  if (MINGW)
    # GNU compatible make required
    find_program(MAKE_EXECUTABLE NAMES gmake mingw32-make REQUIRED)
    set(buildCommand ${MAKE_EXECUTABLE})
    set(luajit_implib ${build_dir}/src/libluajit-5.1.dll.a)
  else()
    #MSVC
    set(buildCommand ${CMAKE_COMMAND} -E chdir src cmd.exe /c msvcbuild.bat debug)
    set(luajit_implib ${build_dir}/src/lua51.lib)
  endif()
  set(luajit_dll ${build_dir}/src/lua51.dll)
  ExternalProject_Add(
    buildLuaJit
    URL ${url}
    SOURCE_DIR ${build_dir}
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
    BUILD_COMMAND ${buildCommand}
    BUILD_BYPRODUCTS ${luajit_implib} ${luajit_dll}
    BUILD_ALWAYS FALSE)
  add_library(luajit::libluajit SHARED IMPORTED)
  set_target_properties(
    luajit::libluajit
    PROPERTIES IMPORTED_IMPLIB
               ${luajit_implib}
               IMPORTED_LOCATION
               ${luajit_dll}
               INTERFACE_INCLUDE_DIRECTORIES
               ${PROJECT_SOURCE_DIR}/third_party/luajit/luajit/src)
endif()

add_dependencies(luajit::libluajit buildLuaJit)

if(WIN32)
  # Lua files below are used by sysbench --luajit-cmd
  # The location must be bin\lua\jit
  install(
    DIRECTORY ${build_dir}/src/jit
    DESTINATION ${CMAKE_INSTALL_BINDIR}/lua
    USE_SOURCE_PERMISSIONS FILES_MATCHING
    PATTERN "*.lua")
endif()
