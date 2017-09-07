//
// leon.h
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
// $Id: leon.h 479 2013-09-06 18:33:43Z frey $
//

#ifndef __LEON_H__
#define __LEON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

/*!
  @typedef leon_result_t
  @discussion
    In some cases we need a tri-state result, not a simple boolean.
*/
typedef enum {
  kLeonResultUnknown    = -1,
  kLeonResultNo         =  0,
  kLeonResultYes        =  1
} leon_result_t;

#endif /* __LEON_H__ */
