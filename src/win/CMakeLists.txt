target_sources(sysbench PRIVATE
 ${CMAKE_CURRENT_SOURCE_DIR}/sb_win_posix.h
 ${CMAKE_CURRENT_SOURCE_DIR}/sb_win_posix.c)
target_include_directories(sysbench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
