//
// ldu.c
// leon - Directory-major scratch filesystem cleanup
//
//
// Copyright © 2013
// Dr. Jeffrey Frey
// University of Delware, IT-NSS
//
// 
// Where "ldu" stands for "leon disk usage", a mimic of the standard "du" utility
// that uses the rate-limited lstat() functionality of leon.
//
// $Id: ldu.c 478 2013-09-05 16:04:12Z frey $
//

#include "leon_path.h"
#include "leon_stat.h"
#include "leon_ratelimits.h"

#include <time.h>
#include <dirent.h>
#include <stdarg.h>
#include <ctype.h>

//

const uint32_t                      ldu_version = (1 << 24) | (0 << 16);

//

#ifndef LDU_NO_CODE_EMBEDDING
#warning Embedding leon_stat in ldu.c for IPA optimization
/* For the sake of IPA optimizations, some of the code is included directly into this source: */
#include "leon_stat.c"
#endif

//

bool
ldu_walk_dir(
  leon_path_ref     basePath,
  off_t             *totalBytes
)
{
  struct stat       fInfo;
  DIR               *dirHandle;
  struct dirent     *dirEntity;
  
  //
  // Check the entity itself:
  //
  if ( leon_stat(leon_path_cString(basePath), &fInfo) != 0 ) {
    leon_log(kLeonLogError, "Unable to stat() %s (errno = %d)", leon_path_cString(basePath), errno);
    return false;
  }
  *totalBytes += fInfo.st_size;
  if ( (fInfo.st_mode & S_IFMT) != S_IFDIR ) return true;
  
  //
  // If we can't open the directory, we can't process it:
  //
  if ( ! (dirHandle = opendir(leon_path_cString(basePath))) ) {
    leon_log(kLeonLogError, "Unable to open directory %s (errno = %d)", leon_path_cString(basePath), errno);
    return false;
  }
  leon_log(kLeonLogDebug1, "Entered directory %s", leon_path_cString(basePath));
  
  //
  // Walk the contents:
  //
  while ( (dirEntity = readdir(dirHandle)) ) {
    bool        isDir = false, isOkay = true;
    
    //
    // Ignore . and .. paths:
    //
    if ( (dirEntity->d_name[0] == '.') && (dirEntity->d_name[1] == '\0' || ((dirEntity->d_name[1] == '.') && (dirEntity->d_name[2] == '\0'))) ) continue;
    
    leon_path_push(basePath, dirEntity->d_name);
    
#ifdef _DIRENT_HAVE_D_TYPE
    if ( dirEntity->d_type == DT_DIR ) {
      isDir = true;
    } else {
      if ( leon_stat(leon_path_cString(basePath), &fInfo) == 0 ) {
        isDir = ((fInfo.st_mode & S_IFMT) == S_IFDIR) ? true : false;
      } else {
        isOkay = false;
      }
    }
#else
    if ( leon_stat(leon_path_cString(basePath), &fInfo) == 0 ) {
      isDir = ((fInfo.st_mode & S_IFMT) == S_IFDIR) ? true : false;
    } else {
      isOkay = false;
    }
#endif
    if ( isOkay ) {
      if ( isDir ) {
        leon_log(kLeonLogDebug1, "Stepping into subdirectory %s", leon_path_cString(basePath));
        if ( ! ldu_walk_dir(basePath, totalBytes) ) {
          closedir(dirHandle);
          return false;
        }
      } else {
        *totalBytes += fInfo.st_size;
      }
    } else {
      leon_log(kLeonLogError, "Unable to stat() %s (errno = %d)", leon_path_cString(basePath), errno);
      leon_path_pop(basePath);
      closedir(dirHandle);
      return false;
    }
    
    leon_path_pop(basePath);
  }
  closedir(dirHandle);
  
  leon_log(kLeonLogDebug1, "Exiting directory %s", leon_path_cString(basePath));
  
  return true;
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
      "\n"
      "  -k/--kilobytes           Display usage sums in kilobytes\n"
      "  -H/--human-readable      Display usage sums in a size-appropriate unit\n"
      "\n"
      "  -S/--stat-limit #.#      Rate limit on calls to stat(); floating-point value in\n"
      "                           units of calls / second\n"
      "  -R/--rate-report         Always show a final report of i/o rates\n"
      "\n"
      " $Id: ldu.c 478 2013-09-05 16:04:12Z frey $\n\n",
      exe
    );
}

void
version(
  const char*   exe
)
{
  char*         svn_revision = "$Revision: 478 $";
  long int      rev = 0;
  
  while ( *svn_revision && ! isdigit(*svn_revision) ) svn_revision++;
  if ( *svn_revision ) {
    rev = strtol(svn_revision, NULL, 10);
  }
  
  printf(
      "%s %u.%u.%u (%ld)\n"
      "  $HeadURL: https://metal1.nss.udel.edu:8443/svn/default/Utilities/Linux/leon/trunk/ldu.c $\n"
      "  $Author: frey $\n"
      "  $Date: 2013-09-05 12:04:12 -0400 (Thu, 05 Sep 2013) $\n\n",
      exe,
      (ldu_version & 0xFF000000) >> 24,
      (ldu_version & 0x00FF0000) >> 16,
      (ldu_version & 0x0000FFFF),
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
        { "kilobytes",          no_argument,        NULL,             'k' },
        { "human-readable",     no_argument,        NULL,             'H' },
        { "rate-report",        no_argument,        NULL,             'R' },
        { "stat-limit",         required_argument,  NULL,             'S' },
        { NULL,                 0,                  NULL,              0  }
      };

//

#include <signal.h>

void
ldu_USR1_handler(
  int     signum
)
{
  leon_stat_profile(kLeonLogSilent);
}

//

void
ldu_printSum(
  const char*     path,
  bool            isHumanReadable,
  bool            isKilobytesOnly,
  off_t           totalBytes
)
{
  double          bytes = (double)totalBytes;
  const char*     unit = "";
  bool            useDecimal = false;
  
  if ( isHumanReadable ) {
    if ( isKilobytesOnly ) {
      useDecimal = true;
      bytes /= 1024.0;
      unit = "kiB";
    } else if ( bytes > 1024.0 ) {
      useDecimal = true;
      bytes /= 1024.0;
      unit = "kiB";
      if ( bytes > 1024.0 ) {
        bytes /= 1024.0;
        unit = "MiB";
        if ( bytes > 1024.0 ) {
          bytes /= 1024.0;
          unit = "GiB";
          if ( bytes > 1024.0 ) {
            bytes /= 1024.0;
            unit = "TiB";
          }
        }
      }
    }
  }
  printf(
      useDecimal ? "%.2lf %s\t%s\n" : "%.0lf %s\t%s\n",
      bytes,
      unit,
      path
    );
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
  bool                          showRateReport = false;
  bool                          showHumanReadable = false;
  bool                          showKilobytesOnly = false;
  
  if ( argc == 1 ) {
    usage(exe);
    return EINVAL;
  }
  
  //
  // USR1 will display stats:
  //
  signal(SIGUSR1, ldu_USR1_handler);
  
  //
  // Process any command-line arguments:
  //
  while ( (opt_ch = getopt_long(argc, (char* const*)argv, "hVqvkHRS:", cli_options, NULL)) != -1 ) {
    
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
      
      case 'k':
        showKilobytesOnly = true;
      case 'H':
        showHumanReadable = true;
        break;
      
      case 'R':
        showRateReport = true;
        break;
      
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
  
  //
  // For each path, do the scan:
  //
  while ( ! rc && (argn < argc) ) {
    const char*     canonicalPath = realpath(argv[argn], NULL);
    
    if ( canonicalPath ) {
      leon_path_ref       basePath = leon_path_createWithCString(canonicalPath);
      
      if ( basePath ) {
        off_t             totalBytes = 0;
        
        if ( ldu_walk_dir(basePath, &totalBytes) ) {
          ldu_printSum(canonicalPath, showHumanReadable, showKilobytesOnly, totalBytes);
        }
        leon_path_destroy(basePath);
      }
      free((void*)canonicalPath);
    }
    argn++;
  }
  leon_stat_profile((showRateReport ? kLeonLogSilent : kLeonLogDebug1));
  
  return rc;
}




