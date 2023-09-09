# Build the bundled luajit library and create imported library.
# luajit::libluajit.
#
# This follows the instructions on the luajit page
include(ExternalProject)

set(build_dir ${PROJECT_BINARY_DIR}/build/third_party/luajit)
set(install_dir ${build_dir}/install)
set(url ${PROJECT_SOURCE_DIR}/third_party/luajit/luajit)


# Note : luajit's Makefile is non-portable (GNU)it could fail on BSD, unless
# gmake is there´
find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)
# Makefile requires MACOSX_DEPLOYMENT_TARGET on Apple
if(CMAKE_OSX_DEPLOYMENT_TARGET)
  set(MACOSX_DEPLOYMENT_TARGET
      "MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()
set(lubluajit_static_lib ${install_dir}/lib/libluajit-5.1.a)
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
             ${lubluajit_static_lib}
             INTERFACE_INCLUDE_DIRECTORIES
             ${install_dir}/include/luajit-2.1
             INTERFACE_LINK_LIBRARIES "m;${CMAKE_DL_LIBS}")
add_dependencies(luajit::libluajit buildLuaJit)

