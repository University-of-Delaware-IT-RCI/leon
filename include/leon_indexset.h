//
// leon_indexset.h
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
// $Id: leon_indexset.h 479 2013-09-06 18:33:43Z frey $
//

#ifndef __LEON_INDEXSET_H__
#define __LEON_INDEXSET_H__

#include "leon.h"

/*!
  @header leon_indexset.h
  @discussion
    An indexset is an array of unsigned integers.
    
    This API is not thread safe.
*/

/*!
  @constant leon_undefIndex
  @discussion
    Value which is returned by leon_indexset functions to indicate
    an undefined index (e.g. end of enumeration for leon_indexset_nextIndex*).
*/
extern unsigned int   leon_undefIndex;

/*!
  @typedef leon_indexset_ref
  @discussion
    Type of an opaque reference to a leon_indexset pseudo-object.
*/
typedef struct _leon_indexset_t * leon_indexset_ref;

/*!
  @function leon_indexset_create
  @discussion
    Create a new indexset object containing no indices.
  @result
    Returns NULL on error, otherwise a reference to an indexset
    pseudo-object that should be deallocated using leon_indexset_destroy().
*/
leon_indexset_ref leon_indexset_create(void);

/*!
  @function leon_indexset_createWithRange
  @discussion
    Create a new indexset object containing the indices in the range
    low to high, inclusive.
  @result
    Returns NULL on error, otherwise a reference to an indexset
    pseudo-object that should be deallocated using leon_indexset_destroy().
*/
leon_indexset_ref leon_indexset_createWithRange(unsigned int low, unsigned int high);

/*!
  @function leon_indexset_destroy
  @discussion
    Deallocate anIndexSet.
*/
void leon_indexset_destroy(leon_indexset_ref anIndexSet);

/*!
  @function leon_indexset_count
  @discussion
    Returns the number of indices present in anIndexSet.
*/
unsigned int leon_indexset_count(leon_indexset_ref anIndexSet);

/*!
  @function leon_indexset_firstIndex
  @discussion
    Returns the lowest-valued index present in anIndexSet.
  @result
    Returns leon_undefIndex if the indexset is empty.
*/
unsigned int leon_indexset_firstIndex(leon_indexset_ref anIndexSet);

/*!
  @function leon_indexset_lastIndex
  @discussion
    Returns the highest-valued index present in anIndexSet.
  @result
    Returns leon_undefIndex if the indexset is empty.
*/
unsigned int leon_indexset_lastIndex(leon_indexset_ref anIndexSet);

/*!
  @function leon_indexset_nextIndexGreaterThan
  @discussion
    Returns the index in anIndexSet which has a value greater-than index.
  @result
    Returns leon_undefIndex if no indices are greater-than index.
*/
unsigned int leon_indexset_nextIndexGreaterThan(leon_indexset_ref anIndexSet, unsigned int index);

/*!
  @function leon_indexset_nextIndexLessThan
  @discussion
    Returns the index in anIndexSet which has a value less-than index.
  @result
    Returns leon_undefIndex if no indices are less-than index.
*/
unsigned int leon_indexset_nextIndexLessThan(leon_indexset_ref anIndexSet, unsigned int index);

/*!
  @function leon_indexset_containsIndex
  @discussion
    Returns true if anIndexSet contains index.
*/
bool leon_indexset_containsIndex(leon_indexset_ref anIndexSet, unsigned int index);

/*!
  @function leon_indexset_addIndex
  @discussion
    If not already present in anIndexSet, make the set contain index.
  @result
    Returns true if anIndexSet contains index.
*/
bool leon_indexset_addIndex(leon_indexset_ref anIndexSet, unsigned int index);

/*!
  @function leon_indexset_removeIndex
  @discussion
    If index is present in anIndexSet, remove it.
  @result
    Returns true if anIndexSet does not contain index.
*/
bool leon_indexset_removeIndex(leon_indexset_ref anIndexSet, unsigned int index);

#endif /* __LEON_INDEXSET_H__ */
