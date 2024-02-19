# Sets variable with the given name to the git hash of the last commit, or empty
# string on any error
function(GitHash varname)
  set("${varname}"
      ""
      PARENT_SCOPE)
  if(NOT EXISTS ${PROJECT_SOURCE_DIR}/.git)
    return()
  endif()
  find_package(Git QUIET)
  if(NOT Git_FOUND)
    return()
  endif()
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE _git_hash
    RESULT_VARIABLE result)
  if(NOT (result EQUAL 0))
    return()
  endif()
  string(REGEX REPLACE "\n$" "" _git_hash "${_git_hash}")
  set(${varname}
      ${_git_hash}
      PARENT_SCOPE)
endfunction()
