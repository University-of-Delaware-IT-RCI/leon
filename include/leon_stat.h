//
// leon_stat.h
// leon - Directory-major scratch filesystem cleanup
//
//
// The leon_stat() function is a stand-in for lstat() that includes
// a throttling mechanism for rate-limiting a program's calls to
// it.
//
//
// Copyright © 2013
// Dr. Jeffrey Frey
// University of Delware, IT-NSS
//
// 
// The program name is a reference to the "cleaner" named Leon in the
// movie, "The Professional."
//
// $Id: leon_stat.h 479 2013-09-06 18:33:43Z frey $
//

#ifndef __LEON_STAT_H__
#define __LEON_STAT_H__

#include "leon.h"
#include "leon_log.h"
#include <sys/types.h>
#include <sys/stat.h>

/*!
  @header leon_stat.h
  @discussion
    The leon_stat() function is a stand-in for the C library's lstat()
    function that includes a runtime-configurable throttling mechanism
    for rate-limiting a program's calls to it.
    
    Throttling is effected by allowing a few seconds of wall time to pass
    while calls to leon_stat() are counted.  Thereafter, the current rate
    R_i is calculated as call count divided by seconds of wall time, dt_i.
    If R_i exceeds the target rate, R_t, then an average sleep period is
    projected over the next 100 calls to the function:
    
         R_t = count / (dt_i + 100 ∆t)
    
          ∆t = ((count / R_t) - dt_i) / 100
    
    and the function calls usleep(∆t).
    
    This API is not thread safe.
*/

/*!
  @function leon_stat_ratelimit
  @discussion
    Returns the target limit, in calls-per-second, that leon_stat() will
    attempt to meet.
  @result
    Returns 0.0 if the leon_stat() is not rate-limited or a value > 0.0.
*/
float leon_stat_ratelimit(void);

/*!
  @function leon_stat_setRatelimit
  @discussion
    Set the maximum calls-per-second that leon_stat() will attempt to meet.
    If rateLimit is less than the minimum rate specified in leon_ratelimits.h
    (LEON_MINIMUM_RATELIMIT) then calls to leon_stat() will not be
    rate-limited.
*/
void leon_stat_setRatelimit(float rateLimit);

/*!
  @function leon_stat_profile
  @discussion
    If enough wall time has passed, logs a summary of the program's usage of
    leon_stat() to stderr at the given verbosity level.  Otherwise, a message
    indicating that not enough wall time has passed is logged (again, at the
    given verbosity level).
*/
void leon_stat_profile(leon_verbosity_t  verbosity);

/*!
  @function leon_stat
  @discussion
    A wrapper to the C library's lstat() function that (possibly) limits the
    rate at which that function is being called.  See your OS's documentation
    for lstat() if you need to know what the path and pathInfo arguments
    represent.
    
    If enough wall time has passed to calculate a statistically-significant
    rate at which this function is being called and a limit has been set via
    leon_stat_setRatelimit(), this function will possibly call usleep() to
    throttle the rate.
  @result
    Returns the result of the call to lstat(), so see your OS's documentation
    for lstat().
*/
int leon_stat(const char* path, struct stat *pathInfo);

/*!
  @function leon_isDirectory
  @discussion
    Calls leon_stat(path,..) and returns true if the path exists and is a
    directory.
*/
bool leon_isDirectory(const char* path);

#endif /* __LEON_STAT_H__ */
