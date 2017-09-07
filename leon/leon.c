//
// leon.c
// leon - Directory-major scratch filesystem cleanup
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
// $Id: leon.c 550 2015-03-04 21:40:34Z frey $
//

#include "leon_path.h"
#include "leon_hash.h"
#include "leon_indexset.h"
#include "leon_stat.h"
#include "leon_fstest.h"
#include "leon_rm.h"
#include "leon_worklog.h"
#include "leon_ratelimits.h"
#include "leon_log.h"

#include <time.h>
#include <dirent.h>
#include <stdarg.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>

//

const uint32_t                      leon_version = (1 << 24) | (0 << 16);

//

#ifndef LEON_NO_CODE_EMBEDDING
#warning Embedding leon_stat, leon_fstest, leon_rm in leon.c for IPA optimization
/* For the sake of IPA optimizations, some of the code is included directly into this source: */
#include "leon_stat.c"
#include "leon_fstest.c"
#include "leon_rm.c"
#endif

//

enum {
  kLeonThresholdFromNow,
  kLeonThresholdFromMidnight,
  kLeonThresholdFromNoon
};

//

static int                            leon_thresholdWhence = kLeonThresholdFromNow;
static long                           leon_thresholdDays = 30;
static bool                           leon_shouldDryRun = true;
static bool                           leon_shouldKeepGoing = false;
static leon_fstest_checkPathFunction  leon_checkPathFn = leon_fstest_checkPathMaxTimes;

//
#if 0
#pragma mark -
#endif
//

leon_result_t
leon_fstest_noSockets(
  const char*     path,
  struct stat*    pathInfo,
  const void*     context
)
{
  if ( (pathInfo->st_mode & S_IFMT) == S_IFSOCK ) return kLeonResultNo;
  return kLeonResultYes;
}

//

leon_result_t
leon_fstest_noPipes(
  const char*     path,
  struct stat*    pathInfo,
  const void*     context
)
{
  if ( (pathInfo->st_mode & S_IFMT) == S_IFIFO ) return kLeonResultNo;
  return kLeonResultYes;
}

//

leon_result_t
leon_fstest_noSocketsOrPipes(
  const char*     path,
  struct stat*    pathInfo,
  const void*     context
)
{
  switch ( pathInfo->st_mode & S_IFMT ) {
    case S_IFSOCK:
    case S_IFIFO:
      return kLeonResultNo;
  }
  return kLeonResultYes;
}

//

leon_result_t
leon_fstest_ownedByUid(
  const char*     path,
  struct stat*    pathInfo,
  const void*     context
)
{
  leon_indexset_ref   excludeUids = (leon_indexset_ref)context;
  
  return leon_indexset_containsIndex(excludeUids, pathInfo->st_uid) ? kLeonResultNo : kLeonResultYes;
}

//

leon_result_t
leon_fstest_ownedByGid(
  const char*     path,
  struct stat*    pathInfo,
  const void*     context
)
{
  leon_indexset_ref   excludeGids = (leon_indexset_ref)context;
  
  return leon_indexset_containsIndex(excludeGids, pathInfo->st_gid) ? kLeonResultNo : kLeonResultYes;
}

//

leon_result_t
leon_fstest_excludePaths(
  const char*     path,
  struct stat*    pathInfo,
  const void*     context
)
{
  leon_hash_ref       excludePaths = (leon_hash_ref)context;
  
  return leon_hash_containsKey(excludePaths, path) ? kLeonResultNo : kLeonResultYes;
}

//
#if 0
#pragma mark -
#endif
//

const char*
__leon_mv_dir_format(void)
{
  static bool     ready = false;
  static char     formatstr[64];
  
  if ( ! ready ) {
    time_t        now = time(NULL);
    struct tm     now_s;
    
    localtime_r(&now, &now_s);
    snprintf(formatstr, sizeof(formatstr), ".leon%04d%02d%02d%02d%02d-%%s", 1900 + now_s.tm_year, 1 + now_s.tm_mon, now_s.tm_mday, now_s.tm_hour, now_s.tm_min);
    ready = true;
  }
  return (const char*)formatstr;
}

int
leon_mv_dir(
  leon_path_ref     basePath,
  leon_path_ref     origDirPath,
  const char*       dirName,
  leon_worklog_ref  worklog
)
{
  int             rc = 0;
  
  //
  // If the path already starts with ".leon############-" then we're just seeing something that failed
  // to be removed in the past:
  //
  if ( strncmp(dirName, ".leon", 5) == 0 ) {
    const char*   s = dirName + 5;
    int           c = 0;
    
    while ( *s && (c < 12) && isdigit(*s) ) { s++; c++; }
    if ( (c == 12) && (*s == '-') ) {
      leon_log(kLeonLogWarning, "Directory flagged by previous run: %s", leon_path_cString(origDirPath));
      return 0;
    }
  }
  leon_log(kLeonLogWarning, "Directory flagged for removal: %s", leon_path_cString(origDirPath));
  
  //
  // Manufacture the alternate name for the directory and try to rename it:
  //
  leon_path_pushFormat(basePath, __leon_mv_dir_format(), dirName);
  if ( ! leon_shouldDryRun ) {
    leon_log(kLeonLogDebug1, "RENAME(%s, %s)", leon_path_cString(origDirPath), leon_path_cString(basePath));
    rc = rename(leon_path_cString(origDirPath), leon_path_cString(basePath));
  } else {
    leon_log(kLeonLogNone, "Directory would be renamed %s", leon_path_cString(basePath));
    rc = 0;
  }
  if ( rc == 0 ) {
    leon_worklog_addPath(worklog, origDirPath, basePath);
  }
  leon_path_pop(basePath);
  
  return rc;
}

//

leon_result_t
leon_cleanup_dir(
  leon_path_ref     basePath,
  leon_worklog_ref  worklog
)
{
  leon_result_t   should_delete;
  struct stat     fInfo;
  DIR             *dirHandle = opendir(leon_path_cString(basePath));
  struct dirent   *dirEntity;
  leon_path_ref   basePathCopy = leon_path_copy(basePath);
  bool            foundSubdir = false;
  
  //
  // If we can't open the directory, we can't process it:
  //
  if ( ! dirHandle ) return kLeonResultUnknown;
  leon_log(kLeonLogDebug1, "Entered directory %s", leon_path_cString(basePath));
  
  //
  // Assume it can be removed:
  //
  should_delete = kLeonResultYes;
  
  //
  // Scan contents of the directory, looking for files that
  // will short-circuit its removal:
  //
  while ( (should_delete == kLeonResultYes) && (dirEntity = readdir(dirHandle)) ) {
    leon_result_t       tmpResult;
    
    //
    // Ignore . and .. paths:
    //
    if ( (dirEntity->d_name[0] == '.') && (dirEntity->d_name[1] == '\0' || ((dirEntity->d_name[1] == '.') && (dirEntity->d_name[2] == '\0'))) ) continue;
    
    //
    // Check the path:
    //
    leon_path_push(basePath, dirEntity->d_name);
    tmpResult = leon_checkPathFn(leon_path_cString(basePath), &fInfo);
    
    //
    // We change the removal status iff it was not a directory AND the check
    // said not to remove it:
    //
    if ( (fInfo.st_mode & S_IFMT) == S_IFDIR ) {
      foundSubdir = true;
    } else if ( tmpResult == kLeonResultNo ) {
      should_delete = kLeonResultNo;
      leon_log(kLeonLogInfo, "Directory removal short-circuited by file %s", leon_path_cString(basePath));
    }
    leon_path_pop(basePath);
  }
  
  //
  // Did we find any sub-directories to scan?
  //
  if ( foundSubdir ) {
    rewinddir(dirHandle);
    while ( (dirEntity = readdir(dirHandle)) ) {
      leon_result_t       subdir_result;
      int                 rc;
    
      //
      // Ignore . and .. paths:
      //
      if ( (dirEntity->d_name[0] == '.') && (dirEntity->d_name[1] == '\0' || ((dirEntity->d_name[1] == '.') && (dirEntity->d_name[2] == '\0'))) ) continue;
      
      leon_path_push(basePath, dirEntity->d_name);
#ifdef _DIRENT_HAVE_D_TYPE
      if ( dirEntity->d_type == DT_DIR || ((dirEntity->d_type == DT_UNKNOWN) && leon_isDirectory(leon_path_cString(basePath))) ) {
#else
      if ( leon_isDirectory(leon_path_cString(basePath)) ) {
#endif
        //
        // Call the cleanup function on the subdirectory no matter what, since we want to peruse
        // its contents and possibly delete it:
        //
        leon_log(kLeonLogDebug1, "Stepping into subdirectory %s", leon_path_cString(basePath));
        subdir_result = leon_cleanup_dir(basePath, worklog);
        if ( subdir_result == kLeonResultYes ) {
          if ( (rc = leon_mv_dir(basePathCopy, basePath, dirEntity->d_name, worklog)) != 0 ) {
            leon_log(kLeonLogError, "(errno = %d) Unable to rename removal target %s", errno, leon_path_cString(basePath));
            subdir_result = kLeonResultNo;
          }
        }
        //
        // If we're set to delete this directory and we find a sub-directory that should NOT be
        // deleted, then we can no longer delete this (parent) directory, either:
        //
        if ( subdir_result == kLeonResultNo ) should_delete = kLeonResultNo;
      }
#if 0
      // balance the _DIRENT_HAVE_D_TYPE conditional above, for the sake
      // of IDE's parsing
      }
#endif
      leon_path_pop(basePath);
    }
  }
  closedir(dirHandle);
  leon_path_destroy(basePathCopy);
  
  leon_log(kLeonLogDebug1, "Exiting directory %s", leon_path_cString(basePath));
  
  return should_delete;
}

//
#if 0
#pragma mark -
#endif
//

void
usage(
  const char*   exe
)
{
  printf(
      "usage:\n\n"
      "  %s {options} <path> {<path> ..}\n\n"
      " options:\n\n"
      "  -h/--help                This information\n"
      "  -V/--version             Version information\n"
      "  -q/--quiet               Minimal output, please\n"
      "  -v/--verbose             Increase the level of output to stderr as the program\n"
      "                           walks the filesystem; may be used multiple times\n"
      "  -D/--do-it               Without this flag, leon will just print info and not\n"
      "                           actually rename/remove anything; the work log will be\n"
      "                           generated no matter what\n"
      "  -k/--keep-going          Ignore errors for <path> and continue processing the\n"
      "                           next <path>\n"
      "\n"
      "  -d/--days <#>            Files/directories that have been modified as recently\n"
      "                           as this many days will not be removed (default: %ld)\n"
      "  -m/--midnight            Calculate the threshold starting from midnight today\n"
      "  -n/--noon                Calculate the threshold starting from noon today\n"
      "  -A/--atime               Temporal tests should be by the 'atime' only\n"
      "  -M/--mtime               Temporal tests should be by the 'mtime' only\n"
      "\n"
      "  -r/--include-root        By default, items owned by root will not be considered\n"
      "                           for removal; this flag overrides that behavior\n"
      "  -s/--ignore-sockets      Socket files should be ignored\n"
      "  -p/--ignore-pipes        Pipes (FIFOs) should be ignored\n"
      "  -e/--exclude-path <path> Do not remove <path> or any of its contents\n"
      "  -E/--exclude-user <uid>  Do not remove directories owned by the given user; if <uid>\n"
      "                           is not an integer it is assumed to be a uname\n"
      "  -G/--exclude-group <gid> Do not remove directories owned by the given group; if <gid>\n"
      "                           is not an integer it is assumed to be a gname\n"
      "\n"
      "  -S/--stat-limit #.#      Rate limit on calls to stat(); floating-point value in\n"
      "                           units of calls / second\n"
      "  -U/--unlink-limit #.#    Rate limit on calls to unlink() and rmdir(); floating-\n"
      "                           point value in units of calls / second\n"
      "  -R/--rate-report         Always show a final report of i/o rates\n"
      "\n"
      "  -o/--work-log-only       Halt after producing the work log (do not remove the\n"
      "                           target directories from the filesystem)\n"
      "  -w/--work-log <path>     Store the work log at the given path\n"
      "  -K/--keep-work-log       Do not delete the work log when the program exits\n"
      "  -F/--allow-files         Allow files to be specified in the argument list as well as\n"
      "                           directories.\n"
      "\n"
      " $Id: leon.c 550 2015-03-04 21:40:34Z frey $\n\n",
      exe,
      leon_thresholdDays
    );
}

void
version(
  const char*   exe
)
{
  char*         svn_revision = "$Revision: 550 $";
  long int      rev = 0;
  
  while ( *svn_revision && ! isdigit(*svn_revision) ) svn_revision++;
  if ( *svn_revision ) {
    rev = strtol(svn_revision, NULL, 10);
  }
  
  printf(
      "%s %u.%u.%u (%ld)\n"
      "  $HeadURL: https://shuebox.nss.udel.edu/nss-code/default/Utilities/Linux/leon/trunk/leon.c $\n"
      "  $Author: frey $\n"
      "  $Date: 2015-03-04 16:40:34 -0500 (Wed, 04 Mar 2015) $\n\n",
      exe,
      (leon_version & 0xFF000000) >> 24,
      (leon_version & 0x00FF0000) >> 16,
      (leon_version & 0x0000FFFF),
      rev
    );
}

//

#include <getopt.h>

static struct option cli_options[] = {
        { "help",               no_argument,        NULL,             'h' },
        { "version",            no_argument,        NULL,             'V' },
        { "quiet",              no_argument,        NULL,             'q' },
        { "verbose",            no_argument,        NULL,             'v' },
        { "days",               required_argument,  NULL,             'd' },
        { "include-root",       no_argument,        NULL,             'r' },
        { "do-it",              no_argument,        NULL,             'D' },
        { "keep-going",         no_argument,        NULL,             'k' },
        { "atime",              no_argument,        NULL,             'A' },
        { "mtime",              no_argument,        NULL,             'M' },
        { "midnight",           no_argument,        NULL,             'm' },
        { "noon",               no_argument,        NULL,             'n' },
        { "ignore-sockets",     no_argument,        NULL,             's' },
        { "ignore-pipes",       no_argument,        NULL,             'p' },
        { "stat-limit",         required_argument,  NULL,             'S' },
        { "unlink-limit",       required_argument,  NULL,             'U' },
        { "rate-report",        no_argument,        NULL,             'R' },
        { "work-log",           required_argument,  NULL,             'w' },
        { "keep-work-log",      no_argument,        NULL,             'K' },
        { "work-log-only",      no_argument,        NULL,             'o' },
        { "exclude-path",       required_argument,  NULL,             'e' },
        { "exclude-user",       required_argument,  NULL,             'E' },
        { "exclude-group",      required_argument,  NULL,             'G' },
        { "allow-files",        no_argument,        NULL,             'F' },
        { NULL,                 0,                  NULL,              0  }
      };

//

#include <signal.h>

void
leon_USR1_handler(
  int     signum
)
{
  leon_stat_profile(kLeonLogSilent);
  leon_rm_profile(kLeonLogSilent);
}

//

int
main(
  int             argc,
  const char*     argv[]
)
{
  int                           rc = 0;
  int                           argn = 1;
  int                           opt_ch;
  const char*                   exe = argv[0];
  bool                          ignoreSockets = false;
  bool                          ignorePipes = false;
  bool                          showRateReport = false;
  leon_path_ref                 workLogPath = NULL;
  leon_hash_ref                 excludePaths = NULL;
  leon_indexset_ref             excludeUids = NULL;
  leon_indexset_ref             excludeGids = NULL;
  bool                          keepWorkLog = false;
  bool                          shouldSuffixWorkLogs = false;
  bool                          workLogOnly = false;
  bool                          allowFiles = false;
  int                           directoryNum = 1;
  
  if ( argc == 1 ) {
    usage(exe);
    return EINVAL;
  }
  
  //
  // USR1 will display stats:
  //
  signal(SIGUSR1, leon_USR1_handler);
  
  //
  // Process any command-line arguments:
  //
  while ( (opt_ch = getopt_long(argc, (char* const*)argv, "hVqvd:rDkAMmnspS:U:Rrw:Koe:E:G:F", cli_options, NULL)) != -1 ) {
    
    switch ( opt_ch ) {
    
      case 'h':
        usage(exe);
        return 0;
    
      case 'V':
        version(exe);
        return 0;
    
      case 'q':
        if ( leon_verbosity > kLeonLogSilent ) leon_verbosity--;
        break;
      
      case 'v':
        if ( leon_verbosity + 1 < kLeonLogMax ) leon_verbosity++;
        break;
      
      case 'r':
        leon_fstest_excludeRoot = false;
        break;
      
      case 'D':
        leon_shouldDryRun = false;
        break;
      
      case 'k':
        leon_shouldKeepGoing = true;
        break;
      
      case 'A':
        leon_checkPathFn = leon_fstest_checkPathAccessTimes;
        break;
      
      case 'M':
        leon_checkPathFn = leon_fstest_checkPathModificationTimes;
        break;
      
      case 'm':
        leon_thresholdWhence = kLeonThresholdFromMidnight;
        break;
      
      case 'n':
        leon_thresholdWhence = kLeonThresholdFromNoon;
        break;
      
      case 's':
        ignoreSockets = true;
        break;
      
      case 'p':
        ignorePipes = true;
        break;
      
      case 'd': {
        char*         end = NULL;
        long int      tmp_days = strtol(optarg, &end, 10);
        
        if ( (tmp_days >= 0) && (end > optarg) ) {
          leon_thresholdDays = tmp_days;
        } else {
          fprintf(stderr, "ERROR:  Invalid value provided to -d/--days option:  %s\n", optarg);
          return EINVAL;
        }
        break;
      }
      
      case 'S': {
        char*         end = NULL;
        float         tmp_limit = strtof(optarg, &end);
        
        if ( (tmp_limit >= LEON_MINIMUM_RATELIMIT) && (end > optarg) ) {
          leon_stat_setRatelimit(tmp_limit);
        } else {
          fprintf(stderr, "ERROR:  Invalid value provided to -S/--stat-limit option:  %s\n", optarg);
          return EINVAL;
        }
        break;
      }
      
      case 'U': {
        char*         end = NULL;
        float         tmp_limit = strtof(optarg, &end);
        
        if ( (tmp_limit >= LEON_MINIMUM_RATELIMIT) && (end > optarg) ) {
          leon_rm_setRatelimit(tmp_limit);
        } else {
          fprintf(stderr, "ERROR:  Invalid value provided to -U/--unlink-limit option:  %s\n", optarg);
          return EINVAL;
        }
        break;
      }
      
      case 'R':
        showRateReport = true;
        break;
      
      case 'w': {
        if ( workLogPath ) leon_path_destroy(workLogPath);
        workLogPath = leon_path_createWithCString(optarg);
        break;
      }
      
      case 'K':
        keepWorkLog = true;
        break;
      
      case 'o':
        workLogOnly = true;
        break;
      
      case 'e': {
        if ( optarg && *optarg ) {
          const char*     canonicalPath = realpath(optarg, NULL);
          
          if ( canonicalPath ) {
            if ( excludePaths == NULL ) {
              if ( (excludePaths = leon_hash_create(0, &leon_hash_key_cString_callbacks, NULL)) == NULL ) {
                fprintf(stderr, "ERROR:  Unable to setup path exclusions.\n");
                return ENOMEM;
              }
            }
            leon_hash_setValueForKey(excludePaths, canonicalPath, (leon_hash_value_t)1);
            free((void*)canonicalPath);
          }
        }
        break;
      }
      
      case 'E': {
        if ( optarg && *optarg ) {
          char*             end;
          unsigned int      uidNum = (unsigned int)strtoll(optarg, &end, 10);
          
          if ( ! end || (*end != '\0') ) {
            struct passwd*  uInfo = getpwnam(optarg);
            
            if ( uInfo ) {
              uidNum = uInfo->pw_uid;
            } else {
              fprintf(stderr, "ERROR:  no such user: %s\n", optarg);
              return EINVAL;
            }
          }
          if ( uidNum > 0 ) {
            if ( excludeUids == NULL ) {
              if ( (excludeUids = leon_indexset_create()) == NULL ) {
                fprintf(stderr, "ERROR:  Unable to setup uid exclusions.\n");
                return ENOMEM;
              }
            }
            leon_indexset_addIndex(excludeUids, uidNum);
          } else {
            fprintf(stderr, "ERROR:  Negative uid numbers not allowed, sorry!\n");
            return EINVAL;
          }
        }
        break;
      }
      
      case 'G': {
        if ( optarg && *optarg ) {
          char*             end;
          unsigned int      gidNum = (unsigned int)strtoll(optarg, &end, 10);
          
          if ( ! end || (*end != '\0') ) {
            struct group*   gInfo = getgrnam(optarg);
            
            if ( gInfo ) {
              gidNum = gInfo->gr_gid;
            } else {
              fprintf(stderr, "ERROR:  no such group: %s\n", optarg);
              return EINVAL;
            }
          }
          if ( gidNum > 0 ) {
            if ( excludeGids == NULL ) {
              if ( (excludeGids = leon_indexset_create()) == NULL ) {
                fprintf(stderr, "ERROR:  Unable to setup gid exclusions.\n");
                return ENOMEM;
              }
            }
            leon_indexset_addIndex(excludeGids, gidNum);
          } else {
            fprintf(stderr, "ERROR:  Negative gid numbers not allowed, sorry!\n");
            return EINVAL;
          }
        }
        break;
      }
      
      case 'F':
        allowFiles = true;
        break;
    
    }
    
  }
  
  //
  // Calculate the cutoff time -- files modified ealier than this time will be considered
  // "old"
  //
  switch ( leon_thresholdWhence ) {
  
    case kLeonThresholdFromNow: {
      leon_fstest_temporalThreshold = time(NULL) - leon_thresholdDays * 24L * 60L * 60L;
      break;
    }
    
    case kLeonThresholdFromMidnight: {
      struct tm       threshold_s;
      time_t          now = time(NULL);
      
      localtime_r(&now, &threshold_s);
      threshold_s.tm_sec = threshold_s.tm_min = threshold_s.tm_hour = 0;
      leon_fstest_temporalThreshold = mktime(&threshold_s) - leon_thresholdDays * 24L * 60L * 60L;
      break;
    }
    
    case kLeonThresholdFromNoon: {
      struct tm       threshold_s;
      time_t          now = time(NULL);
      
      localtime_r(&now, &threshold_s);
      threshold_s.tm_sec = threshold_s.tm_min = 0;
      threshold_s.tm_hour = 12;
      leon_fstest_temporalThreshold = mktime(&threshold_s) - leon_thresholdDays * 24L * 60L * 60L;
      break;
    }
  }
  
  //
  // Skip past the arguments to get to the path(s):
  //
  argn = optind;
  if ( argn == argc ) {
    usage(exe);
    return EINVAL;
  }
  if ( workLogPath && (argc - argn > 1) ) shouldSuffixWorkLogs = true;
  
  //
  // Startup info:
  //
  if ( leon_shouldDryRun ) leon_log(kLeonLogInfo, "This will be a dry run only -- no files/directories will be deleted");
  if ( ! leon_fstest_excludeRoot ) leon_log(kLeonLogInfo, "Directories and files owned by root (uid = 0) will also be removed");
  if ( ignoreSockets ) {
    if ( ignorePipes ) {
      leon_log(kLeonLogInfo, "Socket and FIFO files will not short-circuit directory removal");
    } else {
      leon_log(kLeonLogInfo, "Socket files will not short-circuit directory removal (FIFO files will)");
      leon_fstest_registerCallback("isFIFO", leon_fstest_noPipes, NULL);
    }
  } else if ( ignorePipes ) {
    leon_log(kLeonLogInfo, "FIFO files will not short-circuit directory removal (socket files will)");
    leon_fstest_registerCallback("isSocket", leon_fstest_noSockets, NULL);
  } else {
    leon_log(kLeonLogInfo, "Socket and FIFO files will short-circuit directory removal");
    leon_fstest_registerCallback("isPipeOrSocket", leon_fstest_noSocketsOrPipes, NULL);
  }
  if ( excludePaths ) {
    leon_hash_enum_t    pathEnum;
    
    leon_hash_enum_init(excludePaths, &pathEnum);
    while ( ! leon_hash_enum_isComplete(&pathEnum) ) {
      leon_hash_key_t   path = leon_hash_enum_nextKey(&pathEnum);
      leon_log(kLeonLogInfo, "Path excluded from cleanup:  %s", path);
    }
    leon_fstest_registerCallback("pathExclusions", leon_fstest_excludePaths, excludePaths);
  }
  if ( excludeUids ) {
    unsigned int      uidNum = leon_indexset_firstIndex(excludeUids);
    
    while ( uidNum != leon_undefIndex ) {
      leon_log(kLeonLogInfo, "UID excluded from cleanup:  %u", uidNum);
      uidNum = leon_indexset_nextIndexGreaterThan(excludeUids, uidNum);
    }
    leon_fstest_registerCallback("userExclusions", leon_fstest_ownedByUid, excludeUids);
  }
  if ( excludeGids ) {
    unsigned int      gidNum = leon_indexset_firstIndex(excludeGids);
    
    while ( gidNum != leon_undefIndex ) {
      leon_log(kLeonLogInfo, "GID excluded from cleanup:  %u", gidNum);
      gidNum = leon_indexset_nextIndexGreaterThan(excludeGids, gidNum);
    }
    leon_fstest_registerCallback("groupExclusions", leon_fstest_ownedByGid, excludeGids);
  }
  leon_log(kLeonLogInfo, "Temporal threshold of %ld day%s (%s)", leon_thresholdDays, ( leon_thresholdDays != 1 ? "s" : "" ), leon_timestamp(leon_fstest_temporalThreshold, NULL, 0));
  leon_fstest_description();
  
  //
  // For each path, do the scan:
  //
  while ( (! (rc && ! leon_shouldKeepGoing)) && (argn < argc) ) {
    const char*     canonicalPath = realpath(argv[argn], NULL);
    
    if ( canonicalPath ) {
      if ( ! leon_isDirectory(canonicalPath) ) {
        if ( allowFiles ) {
          struct stat         fInfo;
          leon_result_t       tmpResult = leon_checkPathFn(canonicalPath, &fInfo);
          
          if ( tmpResult == kLeonResultYes ) {
            if ( leon_shouldDryRun ) {
              leon_log(kLeonLogNone, "File would be removed: %s", canonicalPath);
            } else if ( unlink(canonicalPath) == 0 ) {
              leon_log(kLeonLogInfo, "Removed %s", canonicalPath);
            } else {
              leon_log(kLeonLogError, "Unable to remove %s (errno = %d)", canonicalPath, errno);
            }
          }
        } else {
          leon_log(kLeonLogError, "%s is not a directory", canonicalPath);
          rc = EINVAL;
        }
      } else {
        leon_path_ref       basePath = leon_path_createWithCString(canonicalPath);
        
        if ( basePath ) {
          leon_worklog_ref  curWorkLog = NULL;
          leon_result_t     cleanupResult = kLeonResultUnknown;
          
          //
          // Setup the work log:
          //
          if ( workLogPath ) {
            leon_path_ref     curWorkLogPath = leon_path_copy(workLogPath);
            
            if ( shouldSuffixWorkLogs ) leon_path_appendFormat(curWorkLogPath, ".%d", directoryNum);
            leon_log(kLeonLogDebug1, "Creating work log at path %s", leon_path_cString(curWorkLogPath));
            curWorkLog = leon_worklog_createWithFile(curWorkLogPath);
            leon_path_destroy(curWorkLogPath);
          } else {
            leon_log(kLeonLogDebug1, "Creating in-memory work log");
            curWorkLog = leon_worklog_create();
          }
          if ( ! curWorkLog ) {
            leon_log(kLeonLogError, "Unable to create work log for job.");
            if ( ! leon_shouldKeepGoing ) return EPERM;
            leon_path_destroy(basePath);
            continue;
          }
          
          //
          // Scan the directory...if we're not supposed to exclude it!
          //
          if ( excludePaths && leon_hash_containsKey(excludePaths, leon_path_cString(basePath)) ) {
            leon_log(kLeonLogError, "The directory %s is set to be excluded!", canonicalPath);
          } else {
            leon_log(kLeonLogInfo, "Scanning %s", canonicalPath);
            cleanupResult = leon_cleanup_dir(basePath, curWorkLog);
            leon_worklog_scanComplete(curWorkLog, false);
            if ( cleanupResult != kLeonResultUnknown ) {
              if ( ! workLogOnly ) {
                // Process the work log:
                leon_log(kLeonLogInfo, "Processing work log...");
                while ( leon_worklog_getPath(curWorkLog, &basePath) ) {
                  int     errCode;
                  
                  if ( leon_shouldDryRun ) {
                    leon_log(kLeonLogNone, "Directory would be removed: %s", leon_path_cString(basePath));
                  } else {
                    leon_log(kLeonLogInfo, "Removing directory %s", leon_path_cString(basePath));
                    leon_rm(basePath, leon_shouldDryRun, &errCode);
                  }
                }
              }
            }
          }
          leon_worklog_destroy(curWorkLog, keepWorkLog);
          leon_path_destroy(basePath);
        }
      }
      free((void*)canonicalPath);
    }
    argn++;
    directoryNum++;
  }
  
  leon_stat_profile((showRateReport ? kLeonLogSilent : kLeonLogDebug1));
  leon_rm_profile((showRateReport ? kLeonLogSilent : kLeonLogDebug1));
  
  return rc;
}




