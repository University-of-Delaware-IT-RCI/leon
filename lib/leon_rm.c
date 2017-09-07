//
// leon_rm.c
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
// $Id: leon_rm.c 479 2013-09-06 18:33:43Z frey $
//

#include "leon_rm.h"
#include "leon_stat.h"
#include "leon_ratelimits.h"
#include <dirent.h>
#include <stdarg.h>

static bool   __leon_rm_ratelimitIsSet = false;
static float  __leon_rm_ratelimit = 0.0;
static off_t  *__leon_rm_totalBytes = NULL;

//

static bool   __leon_rm_inited = false;
#ifdef LEON_RATELIMITS_USE_TIMEOFDAY

  static struct timeval __leon_rm_start = { 0 , 0 };

#else

  static time_t __leon_rm_start = 0;

#endif
static uint64_t __leon_rm_count = 0;

//

float
leon_rm_ratelimit(void)
{
  return __leon_rm_ratelimit;
}
void
leon_rm_setRatelimit(
  float     rateLimit
)
{
  if ( rateLimit >= LEON_MINIMUM_RATELIMIT ) {
    __leon_rm_ratelimitIsSet = true;
    __leon_rm_ratelimit = rateLimit;
  } else {
    __leon_rm_ratelimitIsSet = false;
    __leon_rm_ratelimit = 0.0f;
  }
}

//

float
leon_rm_rate(void)
{
  return (float)__leon_rm_count / leon_delta_t(__leon_rm_start);
}

//

void
leon_rm_profile(
  leon_verbosity_t  verbosity
)
{
  float             dt = leon_delta_t(__leon_rm_start);
  
  if ( __leon_rm_inited && (dt > LEON_RATELIMITS_LEADIN_SECONDS) ) {
#ifdef LEON_RATELIMITS_USE_TIMEOFDAY
    leon_log(
        verbosity,
        "leon_rm:  %llu calls over %.3f seconds (%.0f calls/sec)",
        (long long unsigned int)__leon_rm_count,
        dt,
        leon_rm_rate()
      );
#else
    leon_log(
        verbosity,
        "leon_rm:  %llu calls over %lld seconds (%.0f calls/sec)",
        (long long unsigned int)__leon_rm_count,
        (long long int)(dt),
        leon_rm_rate()
      );
#endif
  } else if ( __leon_rm_inited ) {
    unsigned long long    dt = LEON_RATELIMITS_LEADIN_SECONDS;
    
    leon_log(
        verbosity,
        "leon_rm:  no profiling data (statistics gathering requires %llu second%s)",
        dt,
        ( dt == 1 ? "" : "s" )
      );
  } else {
    leon_log(
        verbosity,
        "leon_rm:  no profiling data (no calls to leon_rm)",
        dt,
        ( dt == 1 ? "" : "s" )
      );
  }
}

//

int
__leon_rm_entity(
  const char*     filepath,
  bool            isDirectory
)
{
  if ( ! __leon_rm_inited ) {
#ifdef LEON_RATELIMITS_USE_TIMEOFDAY
    if ( 0 != gettimeofday(&__leon_rm_start, NULL) ) {
      __leon_rm_start.tv_sec = time(NULL);
      __leon_rm_start.tv_usec = 0;
    }
#else
    __leon_rm_start = time(NULL);
#endif
    __leon_rm_inited = true;
  }

  // Check the rate?
  if ( __leon_rm_ratelimitIsSet ) {
    float         dt = leon_delta_t(__leon_rm_start);
    
    if ( dt > LEON_RATELIMITS_LEADIN_SECONDS ) {
      float     cur_rate = (float)__leon_rm_count / dt;
      
      leon_log(
            kLeonLogDebug2,
            "__leon_rm_entity:  rate = %.1f calls/sec",
            cur_rate
          );
      if ( cur_rate > __leon_rm_ratelimit ) {
        //
        // Spread the necessary ∆t to reach the target rate over the next 100 calls to leon_stat():
        //
        float     delta_t_us = ((float)__leon_rm_count / __leon_rm_ratelimit - dt) * (1e6) / 100.0f;
        
        if ( delta_t_us > 10.0 ) {
          leon_log(
              kLeonLogDebug1,
              "__leon_rm_entity:  Sleeping for %.0f microseconds",
              delta_t_us
            );
          usleep((useconds_t)delta_t_us);
        }
      }
    }
  }
  
#ifdef LEON_RM_MAXINT_GUARD
  if ( __leon_rm_count == UINT64_MAX ) {
    //
    // We should never actually get here -- at 100000 calls/sec it would
    // take 292465359253 years to roll this 64-bit counter.
    //
    __leon_rm_start = time(NULL);
    __leon_rm_count = 1;
  } else {
    __leon_rm_count++;
  }
#else
  __leon_rm_count++;
#endif
  return ( isDirectory ? rmdir(filepath) : unlink(filepath) );
}

//

bool
leon_rm(
  leon_path_ref     aPath,
  bool              dryRun,
  int               *outErr
)
{
  struct stat       fInfo;
  
  // Is aPath a directory?
  if ( leon_stat(leon_path_cString(aPath), &fInfo) == 0 ) {
    if ( (fInfo.st_mode & S_IFMT) == S_IFDIR ) {
      DIR*          dirHandle = opendir(leon_path_cString(aPath));
      
      if ( dirHandle ) {
        struct dirent *dirEntity;
        
        leon_log(kLeonLogDebug2, "leon_rm: Entering directory %s", leon_path_cString(aPath));
        
        // Remove everything inside the directory:
        while ( (dirEntity = readdir(dirHandle)) ) {
          bool        isDir = false, isOkay = true;
          
          // Don't look at . or ..
          if ( (dirEntity->d_name[0] == '.') && (dirEntity->d_name[1] == '\0' || ((dirEntity->d_name[1] == '.') && (dirEntity->d_name[2] == '\0'))) ) continue;
          
          // Construct the path to the in-scope entity:
          leon_path_push(aPath, dirEntity->d_name);
          
          // What kind of filesystem entity is it?
#ifdef _DIRENT_HAVE_D_TYPE
          if ( dirEntity->d_type == DT_DIR ) {
            isDir = true;
          } else {
            if ( leon_stat(leon_path_cString(aPath), &fInfo) == 0 ) {
              isDir = ((fInfo.st_mode & S_IFMT) == S_IFDIR) ? true : false;
            } else {
              isOkay = false;
            }
          }
#else
          if ( leon_stat(leon_path_cString(aPath), &fInfo) == 0 ) {
            isDir = ((fInfo.st_mode & S_IFMT) == S_IFDIR) ? true : false;
          } else {
            isOkay = false;
          }
#endif
          if ( isOkay ) {
            if ( isDir ) {
              if ( ! leon_rm(aPath, dryRun, outErr) ) {
                closedir(dirHandle);
                return false;
              }
            } else {
              if ( ! dryRun ) {
                if ( (__leon_rm_entity(leon_path_cString(aPath), false) != 0) && (errno != ENOENT) ) {
                  *outErr = errno;
                  leon_log(kLeonLogError, "Unable to unlink(%s) (errno = %d)", leon_path_cString(aPath), errno);
                  leon_path_pop(aPath);
                  closedir(dirHandle);
                  return false;
                } else if ( __leon_rm_totalBytes ) {
                  *__leon_rm_totalBytes += fInfo.st_size;
                }
              }
            }
          } else {
            leon_log(kLeonLogInfo, "Unable to stat(%s) (errno = %d)", leon_path_cString(aPath), errno);
          }
          leon_path_pop(aPath);
        }
        closedir(dirHandle);
      } else {
        *outErr = errno;
        leon_log(kLeonLogError, "Unable to scan directory %s (errno = %d)", leon_path_cString(aPath), errno);
      }
      // Remove the directory itself:
      if ( ! dryRun ) {
        leon_log(kLeonLogDebug2, "leon_rm: Removing directory %s", leon_path_cString(aPath));
        if ( __leon_rm_totalBytes ) leon_stat(leon_path_cString(aPath), &fInfo);
        if ( (__leon_rm_entity(leon_path_cString(aPath), true) != 0) && (errno != ENOENT) ) {
          *outErr = errno;
          leon_log(kLeonLogError, "Unable to rmdir(%s) (errno = %d)", leon_path_cString(aPath), errno);
          return false;
        } else if ( __leon_rm_totalBytes ) {
          *__leon_rm_totalBytes += fInfo.st_size;
        }
      } else {
        leon_log(kLeonLogNone, "Would rmdir(%s)", leon_path_cString(aPath));
      }
      leon_log(kLeonLogDebug2, "leon_rm: Exiting directory %s", leon_path_cString(aPath));
      return true;
    } else {
      if ( dryRun ) {
        leon_log(kLeonLogNone, "Would unlink(%s)", leon_path_cString(aPath));
      } else {
        if ( (__leon_rm_entity(leon_path_cString(aPath), false) != 0) && (errno != ENOENT) ) {
          *outErr = errno;
          leon_log(kLeonLogError, "Unable to unlink(%s) (errno = %d)", leon_path_cString(aPath), errno);
        } else {
          if ( __leon_rm_totalBytes ) *__leon_rm_totalBytes += fInfo.st_size;
          return true;
        }
      }
    }
  } else {
    *outErr = errno;
    leon_log(kLeonLogError, "Unable to stat(%s) (errno = %d)", leon_path_cString(aPath), errno);
  }
  return false;
}

//

bool
__leon_rm_interactivePrompt(
  const char*     exe,
  const char*     format,
  ...
)
{
  va_list         vargs;
  int             c;
  bool            yes;
  
  printf("%s: ", exe);
  va_start(vargs, format);
  vfprintf(stdout, format, vargs);
  va_end(vargs);
  printf("? ");
  fflush(stdout);
  
  // Wait on response:
  c = fgetc(stdin);
  yes = (c == 'y' || c == 'Y') ? true : false;
  while (c != '\n' && c != EOF) c = fgetc(stdin);
  
  return yes;
}

//

const char*    __leon_rm_filetype_descriptions[1 + (S_IFMT >> 16)];
bool           __leon_rm_filetype_descriptions_ready = false;

const char*
__leon_rm_filetype_description(
  mode_t      mode
)
{
  
  const char*           typeStr;
  
  if ( ! __leon_rm_filetype_descriptions_ready ) {
    memset(__leon_rm_filetype_descriptions, sizeof(__leon_rm_filetype_descriptions), 0);
    
    __leon_rm_filetype_descriptions[(S_IFIFO & S_IFMT) >> 12] = "fifo";
    __leon_rm_filetype_descriptions[(S_IFCHR & S_IFMT) >> 12] = "character device";
    __leon_rm_filetype_descriptions[(S_IFDIR & S_IFMT) >> 12] = "directory";
    __leon_rm_filetype_descriptions[(S_IFBLK & S_IFMT) >> 12] = "block device";
    __leon_rm_filetype_descriptions[(S_IFREG & S_IFMT) >> 12] = "regular file";
    __leon_rm_filetype_descriptions[(S_IFLNK & S_IFMT) >> 12] = "symbolic link";
    __leon_rm_filetype_descriptions[(S_IFSOCK & S_IFMT) >> 12] = "socket";
    __leon_rm_filetype_descriptions_ready = true;
  }
  typeStr = __leon_rm_filetype_descriptions[ (mode & S_IFMT) >> 12 ];
  return ( typeStr ? typeStr : "unknown file type" );
}

//

leon_rm_status_t
leon_rm_interactive(
  leon_path_ref     aPath,
  const char*       promptPrefix,
  bool              isRecursive,
  bool              dryRun,
  int               *outErr
)
{
  struct stat       fInfo;
  
  // Is aPath a directory?
  if ( leon_stat(leon_path_cString(aPath), &fInfo) == 0 ) {
    if ( (fInfo.st_mode & S_IFMT) == S_IFDIR ) {
      if ( isRecursive ) {
        DIR*                dirHandle = opendir(leon_path_cString(aPath));
        leon_rm_status_t    dirStatus = kLeonRMStatusFailed;
        
        if ( dirHandle ) {
          struct dirent       *dirEntity;
          
          dirStatus = kLeonRMStatusSucceeded;
          leon_log(kLeonLogDebug2, "leon_rm_interactive: Entering directory %s", leon_path_cString(aPath));
          
          // Remove everything inside the directory:
          while ( (dirStatus != kLeonRMStatusFailed) && (dirEntity = readdir(dirHandle)) ) {
            bool        isDir = false, isOkay = true;
            
            // Don't look at . or ..
            if ( (dirEntity->d_name[0] == '.') && (dirEntity->d_name[1] == '\0' || ((dirEntity->d_name[1] == '.') && (dirEntity->d_name[2] == '\0'))) ) continue;
            
            // Construct the path to the in-scope entity:
            leon_path_push(aPath, dirEntity->d_name);
            
            // What kind of filesystem entity is it?
#ifdef _DIRENT_HAVE_D_TYPE
            if ( dirEntity->d_type == DT_DIR ) {
              isDir = true;
            } else {
              if ( leon_stat(leon_path_cString(aPath), &fInfo) == 0 ) {
                isDir = ((fInfo.st_mode & S_IFMT) == S_IFDIR) ? true : false;
              } else {
                isOkay = false;
              }
            }
#else
            if ( leon_stat(leon_path_cString(aPath), &fInfo) == 0 ) {
              isDir = ((fInfo.st_mode & S_IFMT) == S_IFDIR) ? true : false;
            } else {
              isOkay = false;
            }
#endif
            if ( isOkay ) {
              if ( isDir ) {
                dirStatus = leon_rm_interactive(aPath, promptPrefix, isRecursive, dryRun, outErr);
              } else {
                if ( dryRun ) {
                  leon_log(kLeonLogNone, "Would unlink(%s)", leon_path_cString(aPath));
                } else {
                  // Prompt:
                  if ( __leon_rm_interactivePrompt(promptPrefix, "remove %s `%s'", __leon_rm_filetype_description(fInfo.st_mode), dirEntity->d_name) ) {
                    if ( (__leon_rm_entity(leon_path_cString(aPath), false) != 0) && (errno != ENOENT) ) {
                      *outErr = errno;
                      leon_log(kLeonLogError, "Unable to unlink(%s) (errno = %d)", leon_path_cString(aPath), errno);
                      dirStatus = kLeonRMStatusFailed;
                    } else if ( __leon_rm_totalBytes ) {
                      *__leon_rm_totalBytes += fInfo.st_size;
                    }
                  } else {
                    dirStatus = kLeonRMStatusDeclined;
                  }
                }
              }
            } else {
              leon_log(kLeonLogInfo, "Unable to stat(%s) (errno = %d)", leon_path_cString(aPath), errno);
            }
            leon_path_pop(aPath);
          }
          closedir(dirHandle);
        } else {
          *outErr = errno;
          leon_log(kLeonLogError, "Unable to scan directory %s (errno = %d)", leon_path_cString(aPath), errno);
        }
        // Remove the directory itself:
        if ( dryRun ) {
          leon_log(kLeonLogNone, "Would rmdir(%s)", leon_path_cString(aPath));
        } else if ( dirStatus == kLeonRMStatusSucceeded ) {
          // Prompt:
          if ( __leon_rm_interactivePrompt(promptPrefix, "remove directory `%s'", leon_path_lastComponent(aPath)) ) {
            leon_log(kLeonLogDebug2, "leon_rm_interactive: Removing directory %s", leon_path_cString(aPath));
            if ( __leon_rm_totalBytes ) leon_stat(leon_path_cString(aPath), &fInfo);
            if ( (__leon_rm_entity(leon_path_cString(aPath), true) != 0) && (errno != ENOENT) ) {
              *outErr = errno;
              leon_log(kLeonLogError, "Unable to rmdir(%s) (errno = %d)", leon_path_cString(aPath), errno);
              dirStatus = kLeonRMStatusFailed;
            } else if ( __leon_rm_totalBytes ) {
              *__leon_rm_totalBytes += fInfo.st_size;
            }
          } else {
            dirStatus = kLeonRMStatusDeclined;
          }
        }
        leon_log(kLeonLogDebug2, "leon_rm_interactive: Exiting directory %s", leon_path_cString(aPath));
        return dirStatus;
      } else {
        fprintf(stderr, "%s: cannot remove `%s': Is a directory\n", promptPrefix, leon_path_lastComponent(aPath));
        return kLeonRMStatusFailed;
      }
    } else {
      if ( dryRun ) {
        leon_log(kLeonLogNone, "Would unlink(%s)", leon_path_cString(aPath));
      } else {
        // Prompt:
        if ( __leon_rm_interactivePrompt(promptPrefix, "remove %s `%s'", __leon_rm_filetype_description(fInfo.st_mode), leon_path_lastComponent(aPath)) ) {
          if ( (__leon_rm_entity(leon_path_cString(aPath), false) != 0) && (errno != ENOENT) ) {
            *outErr = errno;
            leon_log(kLeonLogError, "Unable to unlink(%s) (errno = %d)", leon_path_cString(aPath), errno);
          } else {
            if ( __leon_rm_totalBytes ) *__leon_rm_totalBytes += fInfo.st_size;
            return kLeonRMStatusSucceeded;
          }
        } else {
          return kLeonRMStatusDeclined;
        }
      }
    }
  } else {
    *outErr = errno;
    leon_log(kLeonLogError, "Unable to stat(%s) (errno = %d)", leon_path_cString(aPath), errno);
  }
  return kLeonRMStatusFailed;
}

//

void
leon_rm_setByteTrackingPointer(
  off_t     *byteCount
)
{
  if ( (__leon_rm_totalBytes = byteCount) ) {
    *byteCount = 0;
  }
}
