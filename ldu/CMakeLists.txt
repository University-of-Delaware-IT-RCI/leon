cmake_minimum_required (VERSION 2.6)
project (ldu)
add_executable(ldu ldu.c)
target_link_libraries(ldu leon)
include_directories(BEFORE ../lib)

#
# Augment the CPPFLAGS with our flags:
#
if(LEON_NO_CODE_EMBEDDING)
  add_definitions(-DLDU_NO_CODE_EMBEDDING)
endif(LEON_NO_CODE_EMBEDDING)

install (TARGETS ldu DESTINATION bin)

