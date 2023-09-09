if(WIN32)
  add_definitions(
    -DWIN32_MEAN_AND_LEAN
    -DNOGDI
    -DNOMINMAX
    -DNOCOMM
    -DNOUSER
    -DNOCRYPT # minimize windows.h
    -D_CRT_NONSTDC_NO_WARNINGS
    -D_CRT_SECURE_NO_WARNINGS # Remove warnings about using "insecure" functions
  )
endif()

if(MSVC)
  # MSVC warning settings
  set(MSVC_WARNING_LEVEL "3" CACHE STRING "MSVC warning level (1-4)")
  add_compile_options(
      /W${MSVC_WARNING_LEVEL}
      /wd4324  # ignore C4324 '<unnamed-tag>': structure was padded due to alignment specifier
      /wd4200  # ignore C4200 nonstandard extension used: zero-sized array in struct/union
  )
endif()

if(((CMAKE_C_COMPILER_ID STREQUAL GNU) OR (CMAKE_C_COMPILER_ID MATCHES "Clang"))
   AND NOT MSVC)
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
  if(MINGW)
    # mingw gcc is buggy wrt format (does not know %zd, etc)
    add_compile_options(-Wno-format)
  endif()
endif()

if(CMAKE_COMPILE_WARNING_AS_ERROR AND (CMAKE_VERSION VERSION_LESS "3.24"))
  if(MSVC)
    add_compile_options(-WX)
  elseif(CMAKE_COMPILER_IS_GNUC)
    add_compile_options(-Werror)
  endif()
endif()

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
