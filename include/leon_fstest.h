//
// leon_fstest.h
// leon - Directory-major scratch filesystem cleanup
//
//
// A framework for creating an arbitrary chain of callback functions
// to check a filesystem path for removal.
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
// $Id: leon_fstest.h 479 2013-09-06 18:33:43Z frey $
//

#ifndef __LEON_FSTEST_H__
#define __LEON_FSTEST_H__

#include "leon_stat.h"

/*!
  @header leon_fstest.h
  @discussion
    Beyond the simple test of a filesystem object's age, there are myriad other factors
    that influence its eligibility for removal.  All such tests should be implemented
    as functions matching the leon_fstest_callback prototype defined herein.  Tests
    can then be registered with this API in the sequence they should be applied.  Calling
    one of the leon_fstest_checkPath*() functions on a path then uses the basic age test
    plus all necessary registered callbacks to disqualify the path for removal.  If all
    tests are passed, the file is eligible for removal.
*/

/*!
  @constant leon_fstest_excludeRoot
  @discussion
    This global should be set to true if the filesystem tests should mark all filesystem
    entities owned by root as ineligible for deletion.  By default this global is set to
    true.
*/
extern bool       leon_fstest_excludeRoot;

/*!
  @constant leon_fstest_temporalThreshold
  @discussion
    This global should be set to the number of seconds that represent the minimal age
    a filesystem entity must be to be considered eligible for deletion.
*/
extern time_t     leon_fstest_temporalThreshold;

/*!
  @typedef leon_fstest_callback
  @discussion
    Type of a function that tests the filesystem object at path (with properties present
    in the pathInfo struct) for being eligible to be deleted.  Should return kLeonResultYes
    if the object can be removed.
    
    The context is provided when a callback is registered using leon_fstest_registerCallback().
*/
typedef leon_result_t (*leon_fstest_callback)(const char* path, struct stat* pathInfo, const void* context);

/*!
  @function leon_fstest_description
  @discussion
    Display a summary of the filesystem test callbacks that have been registered.
*/
void leon_fstest_description(void);

/*!
  @function leon_fstest_registerCallback
  @discussion
    If a filesystem test has already been registered with aName, replace it in situ with
    aCallback and context.
    
    Otherwise, push the new filesystem test aName to the end of the test chain to use
    aCallback and context.
*/
void leon_fstest_registerCallback(const char* aName, leon_fstest_callback aCallback, const void* context);

/*!
  @function leon_fstest_unregisterCallback
  @discussion
    If a filesystem test has been registered with aName, remove it from the test chain.
*/
void leon_fstest_unregisterCallback(const char* aName);

/*!
  @typedef leon_fstest_checkPathFunction
  @discussion
    There are multiple ways that the age of a filesystem object can be established; they are
    abstracted into three like-invoked functions of this type.
*/
typedef leon_result_t (*leon_fstest_checkPathFunction)(const char* path, struct stat* pathInfo);

/*!
  @function leon_fstest_checkPathModificationTimes
  @discussion
    Use filesystem objects' last-modification timestamp to calculate their age.  If the age is older
    than leon_fstest_temporalThreshold, use the test chain to further attempt to disqualify the
    entity's eligibility.
  @result
    Returns kLeonResultYes if the filesystem object at path is eligible for removal.
*/
leon_result_t leon_fstest_checkPathModificationTimes(const char* path, struct stat* pathInfo);

/*!
  @function leon_fstest_checkPathAccessTimes
  @discussion
    Use filesystem objects' last-accessed timestamp to calculate their age.  If the age is older
    than leon_fstest_temporalThreshold, use the test chain to further attempt to disqualify the
    entity's eligibility.
  @result
    Returns kLeonResultYes if the filesystem object at path is eligible for removal.
*/
leon_result_t leon_fstest_checkPathAccessTimes(const char* path, struct stat* pathInfo);

/*!
  @function leon_fstest_checkPathMaxTimes
  @discussion
    Use whichever timestamp -- last-accessed or last-modified -- is newer to calculate filesystem
    objects' ages.  If the age is older than leon_fstest_temporalThreshold, use the test chain to
    further attempt to disqualify the entity's eligibility.
  @result
    Returns kLeonResultYes if the filesystem object at path is eligible for removal.
*/
leon_result_t leon_fstest_checkPathMaxTimes(const char* path, struct stat* pathInfo);

#endif /* __LEON_FSTEST_H__ */
