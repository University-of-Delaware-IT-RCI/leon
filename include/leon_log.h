//
// leon_log.h
// leon - Directory-major scratch filesystem cleanup
//
//
// Copyright © 2013
// Dr. Jeffrey Frey
// University of Delware, IT-NSS
//
// 
// Logging functionality.
//
// $Id: leon_log.h 479 2013-09-06 18:33:43Z frey $
//

#ifndef __LEON_LOG_H__
#define __LEON_LOG_H__

#include "leon.h"

/*!
  @typedef leon_verbosity_t
  @discussion
    A'la syslog, we allow for discrete verbosity levels that
    control how much information gets output to stderr.
    
    Messages are typically logged to levels above "silent" making
    that level discard just about all output.  Typical programs start
    with leon_verbosity set to the "error" level, with the "-v" flag
    incrementing leon_verbosity and "-q" decrementing it.
*/
typedef enum {
  kLeonLogSilent    = -1,
  kLeonLogNone      =  0,
  kLeonLogError,
  kLeonLogWarning,
  kLeonLogInfo,
  kLeonLogDebug1,
  kLeonLogDebug2,
  kLeonLogMax
} leon_verbosity_t;

/*!
  @constant leon_verbosity
  @discussion
    Current logging level for the program.  All logged messages with a verbosity
    greater than this level will be discarded; all verbosities less than or
    equal to this level will be written to stderr.
*/
extern leon_verbosity_t     leon_verbosity;

/*!
  @function leon_timestamp
  @discussion
    Generate a textual timestamp a'la "2013-09-06 10:47:04-0500" and return a
    pointer to the resulting C string.  If buffer is non-NULL and bufferLen is
    greater than zero, as much of the string as possible is copied to that
    buffer and it is returned.  Otherwise, a shared buffer is used which will
    be overwritten by the next call to this function.
    
    If bufferLen is less than 25 the string will be truncated.
  @result
    Returns NULL on error.  Otherwise, a pointer to a NUL-terminated C string
    containing the textual form of the_time is returned.
*/
char* leon_timestamp(time_t the_time, char* buffer, size_t bufferLen);

/*!
  @function __leon_log
  @discussion
    Programs should not call this function directly.  Instead, the leon_log macro
    should be used, such that inlining of the verbosity check is always performed in
    order to avoid unnecessary function calls.
*/
void __leon_log(leon_verbosity_t minimum_verbosity, const char* format, ...);

/*!
  @defined leon_log
  @discussion
    A'la syslog() the leon_log() function takes a verbosity level of the message, a
    printf()-style format string, and arguments to be converted by the format string.
    The resulting text is written to the stderr stream.
    
    Log lines are prefixed with a timestamp and the logging level label (e.g. ERROR,
    WARNING, DEBUG).
*/
#define leon_log(LEON_LOG_MACRO_MINIMUM_VERBOSITY, LEON_LOG_MACRO_FORMAT, ...) \
  do { if ( leon_verbosity >= LEON_LOG_MACRO_MINIMUM_VERBOSITY ) __leon_log(LEON_LOG_MACRO_MINIMUM_VERBOSITY, LEON_LOG_MACRO_FORMAT, ##__VA_ARGS__); } while (0);

#endif /* __LEON_LOG_H__ */
