//
// leon_stat.c
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
// $Id: leon_stat.c 470 2013-08-22 17:40:01Z frey $
//

#include "leon_stat.h"
#include "leon_ratelimits.h"

static bool   __leon_stat_ratelimitIsSet = false;
static float  __leon_stat_ratelimit = 0.0;

static bool   __leon_stat_inited = false;
#ifdef LEON_RATELIMITS_USE_TIMEOFDAY
#include <sys/time.h>

  static struct timeval __leon_stat_start = { 0 , 0 };

#else

  static time_t __leon_stat_start = 0;

#endif
static uint64_t __leon_stat_count = 0.0;

//

float
leon_stat_ratelimit(void)
{
  return __leon_stat_ratelimit;
}
void
leon_stat_setRatelimit(
  float     rateLimit
)
{
  if ( rateLimit >= LEON_MINIMUM_RATELIMIT ) {
    __leon_stat_ratelimitIsSet = true;
    __leon_stat_ratelimit = rateLimit;
  } else {
    __leon_stat_ratelimitIsSet = false;
    __leon_stat_ratelimit = 0.0f;
  }
}

//

float
leon_stat_rate(void)
{
  return (float)__leon_stat_count / leon_delta_t(__leon_stat_start);
}

//

void
leon_stat_profile(
  leon_verbosity_t  verbosity
)
{
  float             dt = leon_delta_t(__leon_stat_start);
  
  if ( __leon_stat_inited && (dt > LEON_RATELIMITS_LEADIN_SECONDS) ) {
#ifdef LEON_RATELIMITS_USE_TIMEOFDAY
    leon_log(
        verbosity,
        "leon_stat:  %llu calls over %.3f seconds (%.0f calls/sec)",
        (long long unsigned int)__leon_stat_count,
        dt,
        leon_stat_rate()
      );
#else
    leon_log(
        verbosity,
        "leon_stat:  %llu calls over %lld seconds (%.0f calls/sec)",
        (long long unsigned int)__leon_stat_count,
        (long long int)(dt),
        leon_stat_rate()
      );
#endif
  } else if ( __leon_stat_inited ) {
    unsigned long long    dt = LEON_RATELIMITS_LEADIN_SECONDS;
    
    leon_log(
        verbosity,
        "leon_stat:  no profiling data (statistics gathering requires %llu second%s)",
        dt,
        ( dt == 1 ? "" : "s" )
      );
  } else {
    leon_log(
        verbosity,
        "leon_stat:  no profiling data (no calls to leon_stat)",
        dt,
        ( dt == 1 ? "" : "s" )
      );
  }
}

//

int
leon_stat(
  const char*   path,
  struct stat   *pathInfo
)
{
  if ( ! __leon_stat_inited ) {
#ifdef LEON_RATELIMITS_USE_TIMEOFDAY
    if ( 0 != gettimeofday(&__leon_stat_start, NULL) ) {
      __leon_stat_start.tv_sec = time(NULL);
      __leon_stat_start.tv_usec = 0;
    }
#else
    __leon_stat_start = time(NULL);
#endif
    __leon_stat_inited = true;
  }
  
  // Check the rate?
  if ( __leon_stat_ratelimitIsSet ) {
    float         dt = leon_delta_t(__leon_stat_start);
    
    if ( dt > LEON_RATELIMITS_LEADIN_SECONDS ) {
      float     cur_rate = (float)__leon_stat_count / dt;
      
      leon_log(
            kLeonLogDebug2,
            "leon_stat:  rate = %.1f calls/sec",
            cur_rate
          );
      if ( cur_rate > __leon_stat_ratelimit ) {
        //
        // Spread the necessary ∆t to reach the target rate over the next 100 calls to leon_stat():
        //
        float     delta_t_us = ((float)__leon_stat_count / __leon_stat_ratelimit - dt) * (1e6) / 100.0f;
        
        if ( delta_t_us > 10.0 ) {
          leon_log(
              kLeonLogDebug1,
              "leon_stat:  Sleeping for %.0f microseconds",
              delta_t_us
            );
          usleep((useconds_t)delta_t_us);
        }
      }
    }
  }

#ifdef LEON_STAT_MAXINT_GUARD
  if ( __leon_stat_count == UINT64_MAX ) {
    //
    // We should never actually get here -- at 100000 calls/sec it would
    // take 292465359253 years to roll this 64-bit counter.
    //
    __leon_stat_start = time(NULL);
    __leon_stat_count = 1;
  } else {
    __leon_stat_count++;
  }
#else
  __leon_stat_count++;
#endif
  return lstat(path, pathInfo);
}

//

bool
leon_isDirectory(
  const char*     path
)
{
  struct stat     fInfo;
  
  if ( leon_stat(path, &fInfo) == 0 ) {
    if ( (fInfo.st_mode & S_IFMT) == S_IFDIR ) return true;
  }
  return false;
}
