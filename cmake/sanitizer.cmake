option(WITH_ASAN OFF "Enable Address sanitizer")

if(WITH_ASAN)
  add_compile_options(-fsanitize=address)
  if(MSVC AND (NOT CMAKE_C_COMPILER_ID MATCHES "Clang"))
    add_link_options(-INCREMENTAL:NO)
  else()
    add_link_options(-fsanitize=address)
  endif()
endif()

if(MSVC)
  return()
endif()

option(WITH_TSAN OFF "Enable thread sanitizer")
if(WITH_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()

option(WITH_UBSAN OFF "Enable undefined behavior sanitizer")
if(WITH_UBSAN)
  add_compile_options(-fsanitize=undefined)
  add_link_options(-fsanitize=undefined)
endif()
option(WITH_MSAN OFF "Emable memory sanitizer")
if(WITH_MSAN)
  add_compile_options(-fsanitize=memory)
  add_link_options(-fsanitize=memory)
endif()

if(CMAKE_COMPILER_IS_GNUCC
   AND (WITH_ASAN
        OR WITH_UBSAN
        OR WITH_TSAN))
  add_compile_options(-fno-omit-frame-pointer -g)
endif()
