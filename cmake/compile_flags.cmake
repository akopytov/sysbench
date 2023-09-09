if((CMAKE_C_COMPILER_ID STREQUAL GNU) OR (CMAKE_C_COMPILER_ID MATCHES "Clang"))
  add_compile_options(
    -Wall
    -Wextra
    -Wpointer-arith
    -Wbad-function-cast
    -Wstrict-prototypes
    -Wnested-externs
    -Wno-format-zero-length
    -Wundef
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wmissing-declarations
    -Wredundant-decls
    -Wcast-align
    -Wvla)
endif()

if(CMAKE_COMPILE_WARNING_AS_ERROR AND (CMAKE_VERSION VERSION_LESS "3.24"))
  if(MSVC)
    add_compile_options(-WX)
  elseif(CMAKE_COMPILER_IS_GNUC)
    add_compile_options(-Werror)
  endif()
endif()
