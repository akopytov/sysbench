if(WIN32)
  message(FATAL_ERROR "concurrency kit can't be built on Windows")
endif()

# Compile concurrency kit from source Currently, we do not even need to link the
# library we only use functionality from headers only

set(build_dir ${PROJECT_BINARY_DIR}/build/third_party/concurrency_kit/ck)
set(install_dir ${build_dir}/install)

include(ExternalProject)
ExternalProject_Add(
  buildConcurrencyKit
  URL ${PROJECT_SOURCE_DIR}/third_party/concurrency_kit/ck
  SOURCE_DIR ${build_dir}
  CONFIGURE_COMMAND ./configure --prefix=${install_dir}
  BUILD_BYPRODUCTS ${install_dir}/lib/libck.a
  BUILD_COMMAND make
  INSTALL_COMMAND make install
  BUILD_IN_SOURCE 1
  BUILD_ALWAYS FALSE)

add_library(ConcurrencyKit STATIC IMPORTED)
add_dependencies(ConcurrencyKit buildConcurrencyKit)
make_directory(${install_dir}/include)
set_target_properties(
  ConcurrencyKit
  PROPERTIES IMPORTED_LOCATION
             ${install_dir}/lib/libck.a
             INTERFACE_INCLUDE_DIRECTORIES
             ${install_dir}/include)
