cmake_minimum_required (VERSION 2.6)
project (leon)
include_directories(BEFORE SYSTEM include)

#
# Our custom parameters:
#
set(LEON_RATELIMITS_USE_TIMEOFDAY ON CACHE BOOL "Use time of day deltas in rate limit computation")
set(LEON_NO_CODE_EMBEDDING OFF CACHE BOOL "Do not embed time-critical functions in utilities")

#
# Locate SQLite
#
find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES libsqlite3.so)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLITE3 DEFAULT_MSG SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR)
IF(SQLITE3_FOUND)
	SET(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
	SET(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
ELSE(SQLITE3_FOUND)
	SET(SQLITE3_LIBRARIES)
	SET(SQLITE3_INCLUDE_DIRS)
ENDIF(SQLITE3_FOUND)
MARK_AS_ADVANCED(SQLITE3_INCLUDE_DIRS SQLITE3_LIBRARIES)

#
# Augment the compile options with our global flags:
#
if(LEON_RATELIMITS_USE_TIMEOFDAY)
  add_definitions(-DLEON_RATELIMITS_USE_TIMEOFDAY)
endif(LEON_RATELIMITS_USE_TIMEOFDAY)

#
# Each of our sub-projects:
#
add_subdirectory(lib)
add_subdirectory(ldu)
add_subdirectory(lrm)
add_subdirectory(leon)
