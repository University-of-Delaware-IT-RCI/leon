//
// leon_ratelimits.h
// leon - Directory-major scratch filesystem cleanup
//
//
// Helper pieces for implementing rate limits.
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
// $Id: leon_ratelimits.h 479 2013-09-06 18:33:43Z frey $
//

#ifndef __LEON_RATELIMITS_H__
#define __LEON_RATELIMITS_H__

#include "leon.h"

/*!
  @header leon_ratelimits.h
  @discussion
    Common pieces for the rate-limiting mechanism used in leon_rm.h and
    leon_stat.h.
    
    On systems that support the gettimeofday() functionality that function
    is preferred over the time() function due to its finer-grain
    representation of time.  The LEON_RATELIMITS_USE_TIMEOFDAY macro
    controls which variant of temporal representation and computation is
    used.
*/

#ifdef LEON_RATELIMITS_USE_TIMEOFDAY
#include <sys/time.h>

#define LEON_DELTA_TIMEVAL(a, b, result)           \
    (result).tv_sec = (a).tv_sec - (b).tv_sec;       \
    (result).tv_usec = (a).tv_usec - (b).tv_usec;    \
    if ((result).tv_usec < 0) {                        \
      --(result).tv_sec;                               \
      (result).tv_usec += 1000000;                     \
    }

/*!
  @function leon_delta_t
  @discussion
    Meant to be inlined, this function takes a starting time
    and the current system time and calculates the difference
    in seconds.  The difference is returned as a single-
    precision float.
*/
static float
leon_delta_t(
  struct timeval      startTime
)
{
  struct timeval      now, delta;
  
  if ( 0 == gettimeofday(&now, NULL) ) {
    LEON_DELTA_TIMEVAL(now, startTime, delta);
    return (float)delta.tv_sec + 1e-6 * (float)delta.tv_usec;
  }
  return 0.0f;
}
#else
static float
leon_delta_t(
  time_t    startTime
)
{
  return (float)(time(NULL) - startTime);
}
#endif

//

#ifndef LEON_RATELIMITS_LEADIN_SECONDS
/*!
  @defined LEON_RATELIMITS_LEADIN_SECONDS
  @discussion
    How much wall time must pass before we've gathered statistics enough to
    reliably calculate a call rate?
*/
#define LEON_RATELIMITS_LEADIN_SECONDS  (1.0f)
#endif

#ifndef LEON_MINIMUM_RATELIMIT
/*!
  @define LEON_MINIMUM_RATELIMIT
  @discussion
    The call rate should NOT be limited to this value or lower.  No reason
    to try to limit to e.g. 0.1 calls/second!
*/
#define LEON_MINIMUM_RATELIMIT          (1.0f)
#endif

#endif /* __LEON_RATELIMITS_H__ */
