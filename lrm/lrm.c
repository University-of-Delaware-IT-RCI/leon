//
// lrm.c
// leon - Directory-major scratch filesystem cleanup
//
//
// Copyright © 2013
// Dr. Jeffrey Frey
// University of Delware, IT-NSS
//
// 
// Where "lrm" stands for "leon remove", a mimic of the standard "rm" utility
// that uses the rate-limited lstat() and unlink()/rmdir() functionality of leon.
//
// $Id: lrm.c 470 2013-08-22 17:40:01Z frey $
//

#include "leon_path.h"
#include "leon_stat.h"
#include "leon_rm.h"
#include "leon_ratelimits.h"

#include <time.h>
#include <dirent.h>
#include <stdarg.h>

//

const uint32_t                      lrm_version = (1 << 24) | (0 << 16);

//

#ifndef LRM_NO_CODE_EMBEDDING
#warning Embedding leon_stat and leon_rm in lrm.c for IPA optimization
/* For the sake of IPA optimizations, some of the code is included directly into this source: */
#include "leon_stat.c"
#include "leon_rm.c"
#endif

//

typedef enum {
  kLeonRMInteractiveNever,
  kLeonRMInteractiveOnce,
  kLeonRMInteractiveAlways
} lrm_interactive_t;

//

bool
lrm_prompt(
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
  vprintf(format, vargs);
  va_end(vargs);
  printf("? ");
  fflush(stdout);
  
  // Wait on response:
  c = getchar ();
  yes = (c == 'y' || c == 'Y') ? true : false;
  while (c != '\n' && c != EOF) c = getchar ();
  return yes;
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
      "  --interactive{=WHEN}     Prompt the user for removal of items.  Values for WHEN\n"
      "                           are never, once (-I), or always (-i).  If WHEN is not\n"
      "                           specified, defaults to always\n"
      "  -i                       Shortcut for --interactive=always\n"
      "  -I                       Shortcut for --interactive=once; user is prompted one time\n"
      "                           only if a directory is being removed recursively or if more\n"
      "                           than three items are being removed\n"
      "  -r/--recursive           Remove directories and their contents recursively\n"
      "\n"
      "  -s/--summary             Display a summary of how much space was freed...\n"
      "    -k/--kilobytes         ...in kilobytes\n"
      "    -H/--human-readable    ...in a size-appropriate unit\n"
      "\n"
      "  -S/--stat-limit #.#      Rate limit on calls to stat(); floating-point value in\n"
      "                           units of calls / second\n"
      "  -U/--unlink-limit #.#    Rate limit on calls to unlink() and rmdir(); floating-\n"
      "                           point value in units of calls / second\n"
      "  -R/--rate-report         Always show a final report of i/o rates\n"
      "\n"
      " $Id: lrm.c 470 2013-08-22 17:40:01Z frey $\n\n",
      exe
    );
}

void
version(
  const char*   exe
)
{
  char*         svn_revision = "$Revision: 470 $";
  long int      rev = 0;
  
  while ( *svn_revision && ! isdigit(*svn_revision) ) svn_revision++;
  if ( *svn_revision ) {
    rev = strtol(svn_revision, NULL, 10);
  }
  
  printf(
      "%s %u.%u.%u (%ld)\n"
      "  $HeadURL: https://metal1.nss.udel.edu:8443/svn/default/Utilities/Linux/leon/trunk/lrm.c $\n"
      "  $Author: frey $\n"
      "  $Date: 2013-08-22 13:40:01 -0400 (Thu, 22 Aug 2013) $\n\n",
      exe,
      (lrm_version & 0xFF000000) >> 24,
      (lrm_version & 0x00FF0000) >> 16,
      (lrm_version & 0x0000FFFF),
      rev
    );
}

//

#include <getopt.h>

enum
{
  CLI_OPTION_INTERACTIVE = CHAR_MAX + 1
};

static struct option cli_options[] = {
        { "help",               no_argument,        NULL,             'h' },
        { "version",            no_argument,        NULL,             'V' },
        { "quiet",              no_argument,        NULL,             'q' },
        { "verbose",            no_argument,        NULL,             'v' },
        { "interactive",        optional_argument,  NULL,              CLI_OPTION_INTERACTIVE },
        { "recursive",          no_argument,        NULL,             'r' },
        { "summary",            no_argument,        NULL,             's' },
        { "kilobytes",          no_argument,        NULL,             'k' },
        { "human-readable",     no_argument,        NULL,             'H' },
        { "stat-limit",         required_argument,  NULL,             'S' },
        { "unlink-limit",       required_argument,  NULL,             'U' },
        { "rate-report",        no_argument,        NULL,             'R' },
        { NULL,                 no_argument,        NULL,             'i' },
        { NULL,                 no_argument,        NULL,             'I' },
        { NULL,                 0,                  NULL,              0  }
      };

//

#include <signal.h>

void
lrm_USR1_handler(
  int     signum
)
{
  leon_stat_profile(kLeonLogSilent);
  leon_rm_profile(kLeonLogSilent);
}

//

void
lrm_printSum(
  bool            isHumanReadable,
  bool            isKilobytesOnly,
  off_t           totalBytes
)
{
  double          bytes = (double)totalBytes;
  const char*     unit = "bytes";
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
      useDecimal ? "%.2lf %s" : "%.0lf %s",
      bytes,
      unit
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
  bool                          showSummary = false;
  bool                          isRecursive = false;
  lrm_interactive_t             interactivity = kLeonRMInteractiveNever;
  off_t                         totalBytes = 0;
  
  if ( argc == 1 ) {
    usage(exe);
    return EINVAL;
  }
  
  //
  // USR1 will display stats:
  //
  signal(SIGUSR1, lrm_USR1_handler);
  
  //
  // Process any command-line arguments:
  //
  while ( (opt_ch = getopt_long(argc, (char* const*)argv, "hVqviIrskHS:U:R", cli_options, NULL)) != -1 ) {
    
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
        
      case CLI_OPTION_INTERACTIVE:
        if ( optarg && *optarg ) {
          if ( ! strcmp(optarg, "always") || ! strcmp(optarg, "yes") ) {
            interactivity = kLeonRMInteractiveAlways;
          }
          else if ( ! strcmp(optarg, "never") || ! strcmp(optarg, "no") || ! strcmp(optarg, "none") ) {
            interactivity = kLeonRMInteractiveNever;
          }
          else if ( ! strcmp(optarg, "once") ) {
            interactivity = kLeonRMInteractiveOnce;
          }
          else {
            fprintf(
                stderr,
                "%s: invalid argument `%s' for `--interactive'\n"
                "Valid arguments are:\n"
                "  - `never', `no', `none'\n"
                "  - `once'\n"
                "  - `always', `yes'\n"
                "Try `%s --help' for more information.\n",
                exe,
                optarg,
                exe
              );
            return EINVAL;
          }
        } else {
          interactivity = kLeonRMInteractiveAlways;
        }
        break;
      case 'i':
        interactivity = kLeonRMInteractiveAlways;
        break;
      case 'I':
        interactivity = kLeonRMInteractiveOnce;
        break;
      
      case 'r':
        isRecursive = true;
        break;
      
      case 's':
        showSummary = true;
        leon_rm_setByteTrackingPointer(&totalBytes);
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
  // If we're supposed to prompt once and we have more than three arguments, go ahead and do the prompting:
  //
  if ( interactivity == kLeonRMInteractiveOnce ) {
    bool      promptResult = true;
    
    if ( isRecursive ) {
      promptResult = lrm_prompt(exe, "remove all arguments recursively");
    }
    else if ( argc - argn + 1 >= 3 ) {
      promptResult = lrm_prompt(exe, "remove all arguments");
    }
    if ( ! promptResult ) return 0;
  }
  
  //
  // For each path, do the walk-and-remove:
  //
  while ( ! rc && (argn < argc) ) {
    const char*     canonicalPath = realpath(argv[argn], NULL);
    
    if ( canonicalPath ) {
      leon_path_ref       basePath = leon_path_createWithCString(canonicalPath);
      
      if ( basePath ) {
        if ( interactivity == kLeonRMInteractiveAlways ) {
          leon_rm_status_t  rmStatus;
          
          rmStatus = leon_rm_interactive(basePath, exe, isRecursive, false, &rc);
        } else {
          leon_rm(basePath, false, &rc);
        }
        leon_path_destroy(basePath);
      }
      free((void*)canonicalPath);
    }
    argn++;
  }
  if ( showSummary ) {
    printf("%s: removed ", exe);
    lrm_printSum(showHumanReadable, showKilobytesOnly, totalBytes);
    printf("\n");
    fflush(stdout);
  }
  leon_stat_profile((showRateReport ? kLeonLogSilent : kLeonLogDebug1));
  leon_rm_profile((showRateReport ? kLeonLogSilent : kLeonLogDebug1));
  
  return rc;
}




