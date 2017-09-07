cmake_minimum_required (VERSION 2.6)
project (leon)
add_executable(leon-exe leon.c)
set_target_properties(leon-exe PROPERTIES OUTPUT_NAME leon)
target_link_libraries(leon-exe leon ${SQLITE3_LIBRARIES})
include_directories(BEFORE ../lib ${SQLITE3_INCLUDE_DIRS})

#
# Augment the CPPFLAGS with our flags:
#
if(LEON_NO_CODE_EMBEDDING)
  add_definitions(-DLEON_NO_CODE_EMBEDDING)
endif(LEON_NO_CODE_EMBEDDING)

install (TARGETS leon-exe DESTINATION bin)

