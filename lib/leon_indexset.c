//
// leon_indexset.c
// leon - Directory-major scratch filesystem cleanup
//
//
// The leon_indexset pseudo-class provides an array of integer values.
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
// $Id: leon_indexset.c 463 2013-08-02 16:51:57Z frey $
//

#include "leon_indexset.h"
#include <limits.h>

unsigned int   leon_undefIndex = UINT_MAX;

//

typedef struct _leon_indexset_t {
  unsigned int      min, max;
  
  unsigned int      count, capacity;
  unsigned int      *values;
  
  unsigned int      enumIdx, enumLastVal;
} leon_indexset_t;

//

bool
__leon_indexset_grow(
  leon_indexset_t   *anIndexSet
)
{
  unsigned int      capacity = 1 + anIndexSet->capacity / 32;
  unsigned int      *values;
  
  capacity *= 32;
  if ( anIndexSet->values ) {
    values = (unsigned int*)realloc(anIndexSet->values, capacity * sizeof(unsigned int));
  } else {
    values = (unsigned int*)malloc(capacity * sizeof(unsigned int));
  }
  if ( values ) {
    anIndexSet->capacity = capacity;
    anIndexSet->values = values;
    return true;
  }
  return false;
}

//

leon_indexset_ref
leon_indexset_create(void)
{
  return leon_indexset_createWithRange(0, UINT_MAX - 1);
}

//

leon_indexset_ref
leon_indexset_createWithRange(
  unsigned int  low,
  unsigned int  high
)
{
  leon_indexset_t*    newSet = (leon_indexset_t*)calloc(1, sizeof(leon_indexset_t));
  
  if ( newSet ) {
    newSet->min = low;
    newSet->max = (high > low) ? ((high == UINT_MAX) ? high - 1 : high) : low;
  }
  return newSet;
}

//

void
leon_indexset_destroy(
  leon_indexset_ref   anIndexSet
)
{
  if ( anIndexSet->values ) free((void*)anIndexSet->values);
  free((void*)anIndexSet);
}

//

unsigned int
leon_indexset_count(
  leon_indexset_ref   anIndexSet
)
{
  return anIndexSet->count;
}

//

unsigned int
leon_indexset_firstIndex(
  leon_indexset_ref     anIndexSet
)
{
  if ( anIndexSet->count > 0 ) {
    anIndexSet->enumIdx = 0;
    return (anIndexSet->enumLastVal = anIndexSet->values[0]);
  }
  return leon_undefIndex;
}

unsigned int
leon_indexset_lastIndex(
  leon_indexset_ref     anIndexSet
)
{
  if ( anIndexSet->count > 0 ) {
    anIndexSet->enumIdx = anIndexSet->count - 1;
    return (anIndexSet->enumLastVal = anIndexSet->values[anIndexSet->enumIdx]);
  }
  return leon_undefIndex;
}

unsigned int
leon_indexset_nextIndexGreaterThan(
  leon_indexset_ref     anIndexSet,
  unsigned int          index
)
{
  unsigned int          i = 0;
  
  if ( anIndexSet->enumIdx != leon_undefIndex ) {
    if ( anIndexSet->enumLastVal == index ) {
      if ( ++anIndexSet->enumIdx < anIndexSet->count ) {
        return (anIndexSet->enumLastVal = anIndexSet->values[anIndexSet->enumIdx]);
      }
      anIndexSet->enumIdx = leon_undefIndex;
      return leon_undefIndex;
    }
  }
  
  // Long scan:
  while ( i < anIndexSet->count ) {
    if ( anIndexSet->values[i] > index ) {
      anIndexSet->enumIdx = i;
      return (anIndexSet->enumLastVal = anIndexSet->values[i]);
    }
    i++;
  }
  return leon_undefIndex;
}

unsigned int
leon_indexset_nextIndexLessThan(
  leon_indexset_ref     anIndexSet,
  unsigned int          index
)
{
  unsigned int          i = anIndexSet->count;
  
  if ( anIndexSet->enumIdx != leon_undefIndex ) {
    if ( anIndexSet->enumLastVal == index ) {
      if ( anIndexSet->enumIdx-- > 0 ) {
        return (anIndexSet->enumLastVal = anIndexSet->values[anIndexSet->enumIdx]);
      }
      anIndexSet->enumIdx = leon_undefIndex;
      return leon_undefIndex;
    }
  }
  
  // Long scan:
  while ( i-- > 0 ) {
    if ( anIndexSet->values[i] < index ) {
      anIndexSet->enumIdx = i;
      return (anIndexSet->enumLastVal = anIndexSet->values[i]);
    }
  }
  return leon_undefIndex;
}

//

int
__leon_indexset_bsearch(
  const void*   a,
  const void*   b
)
{
  unsigned int  A = *((unsigned int*)a);
  unsigned int  B = *((unsigned int*)b);
  
  if ( A < B ) return -1;
  if ( A == B ) return 0;
  return 1;
}

//

bool
leon_indexset_containsIndex(
  leon_indexset_ref     anIndexSet,
  unsigned int          index
)
{
  if ( (index >= anIndexSet->min) && (index <= anIndexSet->max) && anIndexSet->count ) {
    unsigned int*       found = bsearch(&index, anIndexSet->values, anIndexSet->count, sizeof(unsigned int), __leon_indexset_bsearch);
    
    if ( found ) return true;
  }
  return false;
}

//

bool
leon_indexset_addIndex(
  leon_indexset_ref     anIndexSet,
  unsigned int          index
)
{
  if ( (index >= anIndexSet->min) && (index <= anIndexSet->max) ) {
    unsigned int        i = 0;
    
    while ( i < anIndexSet->count ) {
      unsigned int      v = anIndexSet->values[i];
      
      if ( v == index ) return true;
      if ( v > index ) break;
      i++;
    }
    if ( anIndexSet->count == anIndexSet->capacity ) {
      if ( ! __leon_indexset_grow(anIndexSet) ) return false;
    }
    if ( i < anIndexSet->count ) memmove(anIndexSet->values + i + 1, anIndexSet->values + i, (anIndexSet->count - i) * sizeof(unsigned int));
    anIndexSet->values[i] = index;
    anIndexSet->count++;
    anIndexSet->enumIdx = leon_undefIndex;
  }
  return false;
}

//

bool
leon_indexset_removeIndex(
  leon_indexset_ref     anIndexSet,
  unsigned int          index
)
{
  if ( (index >= anIndexSet->min) && (index <= anIndexSet->max) && anIndexSet->count ) {
    unsigned int*       found = bsearch(&index, anIndexSet->values, anIndexSet->count, sizeof(unsigned int), __leon_indexset_bsearch);
    
    if ( found ) {
      unsigned int      i = found - anIndexSet->values;
      
      if ( i + 1 < anIndexSet->count ) memmove(anIndexSet->values + i, anIndexSet->values + i + 1, (anIndexSet->count - i) * sizeof(unsigned int));
      anIndexSet->count--;
    }
    anIndexSet->enumIdx = leon_undefIndex;
  }
  return false;
}

//

void
leon_indexset_description(
  leon_indexset_ref     anIndexSet,
  FILE*                 stream
)
{
  unsigned int          i = 0;
  
  fprintf(stream, "leon_indexset@%p ( [%x,%x] %u / %u ) { ", anIndexSet, anIndexSet->min, anIndexSet->max, anIndexSet->count, anIndexSet->capacity);
  while ( i < anIndexSet->count ) {
    fprintf(stream, "%s%u ", ( i > 0 ? "," : "" ), anIndexSet->values[i]);
    i++;
  }
  fprintf(stream, "}\n");
}

//
#if 0
#pragma mark -
#endif
//

#ifdef LEON_INDEXSET_MAIN

int
main()
{
  leon_indexset_ref   mySet = leon_indexset_createWithRange(10,1024);
  
  if ( mySet ) {
    leon_indexset_description(mySet, stdout);
    
    leon_indexset_addIndex(mySet, 0);
    leon_indexset_addIndex(mySet, 192);
    leon_indexset_addIndex(mySet, 54);
    leon_indexset_addIndex(mySet, 1920);
    leon_indexset_addIndex(mySet, 512);
    leon_indexset_addIndex(mySet, 54);
    leon_indexset_addIndex(mySet, 1000);
    leon_indexset_description(mySet, stdout);
    
    printf("5? %d\n", leon_indexset_containsIndex(mySet, 5));
    printf("513? %d\n", leon_indexset_containsIndex(mySet, 513));
    printf("432? %d\n", leon_indexset_containsIndex(mySet, 432));
    printf("54? %d\n", leon_indexset_containsIndex(mySet, 54));
    
    leon_indexset_removeIndex(mySet, 510);
    leon_indexset_removeIndex(mySet, 512);
    leon_indexset_description(mySet, stdout);
    
    leon_indexset_destroy(mySet);
  }
  return 0;
}

#endif
