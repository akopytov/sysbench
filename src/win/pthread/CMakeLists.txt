target_sources(sysbench
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_data.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread.h)
target_include_directories(sysbench
  PRIVATE SYSTEM
  ${CMAKE_CURRENT_SOURCE_DIR})
