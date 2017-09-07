//
// leon_rm.h
// leon - Directory-major scratch filesystem cleanup
//
//
// The leon_rm() function performs a recursive removal of all
// contents of a directory (and the directory itself).
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
// $Id: leon_rm.h 479 2013-09-06 18:33:43Z frey $
//

#ifndef __LEON_RM_H__
#define __LEON_RM_H__

#include "leon_path.h"
#include "leon_log.h"

/*!
  @header leon_rm.h
  @discussion
    The leon_rm() and leon_rm_interactive() functions walk a path depth-first
    fashion and remove all files and directories within it before finally
    removing the path itself.  The functions include a runtime-configurable
    throttling mechanism for rate-limiting a program's calls to it.
    
    Throttling is effected by allowing a few seconds of wall time to pass
    while calls to leon_rm() and leon_rm_interactive() are counted.  Thereafter,
    the current rate R_i is calculated as call count divided by seconds of wall
    time, dt_i.  If R_i exceeds the target rate, R_t, then an average sleep
    period is projected over the next 100 calls to the functions:
    
         R_t = count / (dt_i + 100 ∆t)
    
          ∆t = ((count / R_t) - dt_i) / 100
    
    and the function calls usleep(∆t).
    
    This API is not thread safe.
*/

/*!
  @function leon_rm_ratelimit
  @discussion
    Returns the target limit, in calls-per-second, that leon_rm()/leon_rm_interactive()
    will attempt to meet.
  @result
    Returns 0.0 if the functions are not rate-limited or a value > 0.0.
*/
float leon_rm_ratelimit(void);

/*!
  @function leon_rm_setRatelimit
  @discussion
    Set the maximum calls-per-second that leon_rm()/leon_rm_interactive() will attempt
    to meet.  If rateLimit is less than the minimum rate specified in leon_ratelimits.h
    (LEON_MINIMUM_RATELIMIT) then calls to leon_rm()/leon_rm_interactive() will not be
    rate-limited.
*/
void leon_rm_setRatelimit(float rateLimit);

/*!
  @function leon_rm_profile
  @discussion
    If enough wall time has passed, logs a summary of the program's usage of
    leon_rm()/leon_rm_interactive() to stderr at the given verbosity level.  Otherwise,
    a message indicating that not enough wall time has passed is logged (again, at the
    given verbosity level).
*/
void leon_rm_profile(leon_verbosity_t verbosity);

/*!
  @function leon_rm
  @discussion
    Delete the directory or file at aPath.  Directories are always removed
    recursively.  Any failed call to the unlink() or rmdir() C library
    functions will cause the function to return false and set outErr to
    the value of errno at the time of failure.
    
    If dryRun is true, then only informative messages regarding the removal
    will be displayed and no files/directories will actually be removed from
    the filesystem.
    
    If enough wall time has passed to calculate a statistically-significant
    rate at which this function is being called and a limit has been set via
    leon_rm_setRatelimit(), this function will possibly call usleep() to
    throttle the rate.
  @result
    Returns true if aPath and any descendent directories/files were successfully
    removed.
*/
bool leon_rm(leon_path_ref aPath, bool dryRun, int *outErr);

/*!
  @typedef leon_rm_status_t
  @discussion
    Tri-state for the return status of the interactive form of leon_rm().  The
    kLeonRMStatusSucceeded and kLeonRMStatusFailed values are self-explanatory.
    The function returns kLeonRMStatusDeclined if the user, when prompted,
    declined removal of a filesystem object -- which does not necessarily
    constitute failure or an error.
*/
typedef enum {
  kLeonRMStatusSucceeded,
  kLeonRMStatusFailed,
  kLeonRMStatusDeclined
} leon_rm_status_t;

/*!
  @function leon_rm_interactive
  @discussion
    Delete the directory or file at aPath.  Directories are removed recursively if
    the isRecursive argument is true.  The user is prompted on stdout/stdin to
    consent or deny the removal of each and every filesystem object.
    
    The prompt is produced dynamically according to the kind of filesystem object
    being removed, and is prefixed with promptPrefix and a colon, e.g.:
    
        lrm: remove socket `larry`?
    
    Any failed call to the unlink() or rmdir() C library functions will cause the function
    to return kLeonRMStatusFailed and set outErr to the value of errno at the time of failure.
    
    If dryRun is true, then only informative messages regarding the removal
    will be displayed and no files/directories will actually be removed from
    the filesystem.
    
    If enough wall time has passed to calculate a statistically-significant
    rate at which this function is being called and a limit has been set via
    leon_rm_setRatelimit(), this function will possibly call usleep() to
    throttle the rate.
  @result
    Returns a value from the tri-state leon_rm_status_t.  Note that kLeonRMStatusDeclined
    does not constitute an error.
*/
leon_rm_status_t leon_rm_interactive(leon_path_ref aPath, const char* promptPrefix, bool isRecursive, bool dryRun, int *outErr);

/*!
  @function leon_rm_setByteTrackingPointer
  @discussion
    The leon_rm()/leon_rm_interactive() functions can be configured to collectively sum the
    byte sizes of all filesystem entities they remove.  Calling this function with byteCount
    of NULL disables that functionality (the default).  If byteCount is non-NULL, it must
    point to a variable of type off_t into which the functions should sum the byte sizes.
*/
void leon_rm_setByteTrackingPointer(off_t *byteCount);

#endif /* __LEON_RM_H__ */
