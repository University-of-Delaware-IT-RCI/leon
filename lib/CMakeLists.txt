cmake_minimum_required (VERSION 2.6)
project (libleon)

#
# Our custom parameters:
#
set(LEON_BUILD_LIB_TESTS OFF CACHE BOOL "Build test programs that demonstrate hash and indexset libraries")

add_library(leon STATIC leon_fstest.c leon_hash.c leon_indexset.c leon_log.c leon_path.c leon_rm.c leon_stat.c leon_worklog.c)

if(LEON_BUILD_LIB_TESTS)
  add_executable(leon_hash_test leon_hash.c)
  target_compile_definitions(leon_hash_test PUBLIC -DLEON_HASH_MAIN)
  
  add_executable(leon_indexset_test leon_indexset.c)
  target_compile_definitions(leon_indexset_test PUBLIC -DLEON_INDEXSET_MAIN)
endif(LEON_BUILD_LIB_TESTS)

