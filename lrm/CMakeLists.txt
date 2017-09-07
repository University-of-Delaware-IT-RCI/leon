cmake_minimum_required (VERSION 2.6)
project (lrm)
add_executable(lrm lrm.c)
target_link_libraries(lrm leon)
include_directories(BEFORE ../lib)

#
# Augment the CPPFLAGS with our flags:
#
if(LEON_NO_CODE_EMBEDDING)
  add_definitions(-DLRM_NO_CODE_EMBEDDING)
endif(LEON_NO_CODE_EMBEDDING)

install (TARGETS lrm DESTINATION bin)

