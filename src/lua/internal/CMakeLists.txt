# Create a C header file with the contents of a Lua script
function(create_lua_h src varname dest)
  file(READ ${src} LUA_SRC)
  string(REPLACE "\\" "\\\\" LUA_SRC "${LUA_SRC}")
  string(REPLACE "\"" "\\\"" LUA_SRC "${LUA_SRC}")
  string(REPLACE "\n" "\\n\"\n\"" LUA_SRC "${LUA_SRC}")
  set(filecontent "unsigned char ${varname}[] =\n\"${LUA_SRC}\";\nsize_t ${varname}_len=sizeof(${varname})-1;\n")
  file(WRITE ${dest}.tmp "${filecontent}")
  configure_file(${dest}.tmp ${dest} COPYONLY)
  file(REMOVE ${dest}.tmp)
endfunction()

set(
  LUA_INTERNAL_FILES
  sysbench.cmdline.lua
  sysbench.histogram.lua
  sysbench.lua
  sysbench.rand.lua
  sysbench.sql.lua
)

# Rerun cmake if any of these files changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${LUA_INTERNAL_FILES})

foreach(f ${LUA_INTERNAL_FILES})
  string(REPLACE "." "_" name ${f})
  create_lua_h(${f} ${name} ${CMAKE_CURRENT_BINARY_DIR}/${f}.h)
endforeach()

if(WIN32)
  # prepare list of functions to be exported from sysbench.exe
  set(sysbench_def ${CMAKE_CURRENT_BINARY_DIR}/sysbench.def)
  execute_process(
    COMMAND
      powershell.exe -noprofile -ExecutionPolicy Bypass
      -File ${PROJECT_SOURCE_DIR}/src/win/extract_c_functions.ps1
      ${LUA_INTERNAL_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_FILE ${sysbench_def}.tmp
    COMMAND_ERROR_IS_FATAL ANY)
  configure_file(${sysbench_def}.tmp ${sysbench_def} COPYONLY)
  file(REMOVE ${sysbench_def}.tmp)
  target_sources(sysbench PRIVATE ${sysbench_def})
endif()
