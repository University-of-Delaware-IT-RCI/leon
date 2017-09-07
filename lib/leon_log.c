//
// leon_log.c
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
// $Id: leon_log.c 468 2013-08-21 18:36:51Z frey $
//

#include "leon_log.h"

#include <time.h>
#include <stdarg.h>

const char* leon_log_level_strings[] = { "",
                                         "",
                                         " ERROR:",
                                         " WARNING:",
                                         " INFO:",
                                         " DEBUG:",
                                         " DEBUG+1:"
                                       };
                                       
leon_verbosity_t                      leon_verbosity = kLeonLogError;

//

char*
leon_timestamp(
  time_t    the_time,
  char*     buffer,
  size_t    bufferLen
)
{
  static char     timestamp[32];
  struct tm       the_time_s;
  
  if ( ! buffer || ! bufferLen ) {
    buffer = timestamp;
    bufferLen = sizeof(timestamp);
  }
  localtime_r(&the_time, &the_time_s);
  strftime(buffer, bufferLen, "%Y-%m-%d %H:%M:%S%z", &the_time_s);
  return buffer;
}

void
__leon_log(
  leon_verbosity_t  minimum_verbosity,
  const char*       format,
  ...
)
{
  if ( leon_verbosity >= minimum_verbosity ) {
    va_list       vargs;
    static char   timestamp[32];
    
    va_start(vargs, format);
    fprintf(stderr, "[%s]%s ", leon_timestamp(time(NULL), timestamp, sizeof(timestamp)), leon_log_level_strings[1 + minimum_verbosity]);
    vfprintf(stderr, format, vargs);
    fputc('\n', stderr);
    va_end(vargs);
  }
}
