//
// leon_worklog.h
// leon - Directory-major scratch filesystem cleanup
//
//
// The leon_worklog functions are used to produce and playback a work
// log.
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
// $Id: leon_worklog.h 479 2013-09-06 18:33:43Z frey $
//

#ifndef __LEON_WORKLOG_H__
#define __LEON_WORKLOG_H__

#include "leon_path.h"

/*!
  @header leon_worklog.h
  @abstract
    Database containing paths eligible for removal.
  @discussion
    To facilitate dry-runs and pre-removal analyses of a directory, LEON does not remove content
    as it scans the filesystem.  Rather, it produces a list of directories that it deems eligible
    for removal (and renames eligible directories if this is NOT a dry-run).  Eligible paths are
    pushed to a worklog as the initial scan is performed.  When a path is pushed, all descendent
    paths already present in the worklog are removed, since removal of the parent implies removal
    of its children.
    
    If the program has not been invoked in dry-run mode, directories are purged by popping paths
    from the worklog and doing a recursive rmdir on each.
    
    Internally, the worklog is implemented as an SQLite database with a single table:
    
        CREATE TABLE worklog (
          pathId         INTEGER PRIMARY KEY,
          origPath       TEXT UNIQUE NOT NULL,
          altPath        TEXT UNIQUE NOT NULL
        );
        
    When not in dry-run mode, LEON renames eligibile directories as it scans the filesystem in
    order to mitigate post-scan, pre-removal changes to the directory; the original directory
    name is stored in the origPath field and the renamed path in altPath.
    
    The initial filesystem scan is "wrapped" in a single database transaction to avoid repeated
    transaction cycles that would otherwise radically throttle the scan.  When the initial scan
    has completed (successfully) the leon_worklog_scanComplete() function is called to commit
    the transaction and a new transaction is begun.  A dry-run would exit at this point; otherwise,
    the worklog is replayed and directory removal performed.
*/

/*!
  @typedef leon_worklog_ref
  @discussion
    The type of an opaque reference to a worklog pseudo-object.
*/
typedef struct _leon_worklog_t * leon_worklog_ref;

/*!
  @function leon_worklog_create
  @discussion
    Create a worklog that uses an in-memory SQLite database.
  @result
    Returns NULL on error, otherwise a reference to a worklog pseudo-object
    that should be deallocated using leon_worklog_destroy().
*/
leon_worklog_ref leon_worklog_create(void);

/*!
  @function leon_worklog_createWithFile
  @discussion
    Create a worklog that uses an SQLite database contained in a file at
    aPath.  If the file does not exist it is created.
    
    If the file does exist but is not an SQLite database, the function
    returns in error.  If it is an SQLite database, the "worklog" table will
    be dropped and recreated.
  @result
    Returns NULL on error, otherwise a reference to a worklog pseudo-object
    that should be deallocated using leon_worklog_destroy().
*/
leon_worklog_ref leon_worklog_createWithFile(leon_path_ref aPath);

/*!
  @function leon_worklog_destroy
  @discussion
    Deallocate a worklog pseudo-object.  If the worklog used an SQLite database
    on disk, the file is removed if doNotDelete is false.
*/
void leon_worklog_destroy(leon_worklog_ref aWorkLog, bool doNotDelete);

/*!
  @function leon_worklog_addPath
  @discussion
    Add the eligible directory inOrigPath (renamed to inAltPath) to aWorkLog.  Any paths extant
    in aWorkLog that descend from inOrigPath will be removed from the worklog.
  @result
    Returns false if the directory could not be added to the worklog or if descendent paths could
    not be removed.
*/
bool leon_worklog_addPath(leon_worklog_ref aWorkLog, leon_path_ref inOrigPath, leon_path_ref inAltPath);

/*!
  @function leon_worklog_getPath
  @discussion
    Pop an eligible directory from aWorkLog.  Only the renamed form of the path is returned.
    
    If *outAltPath is NULL then a new leon_path pseudo-object is allocated to wrap the path.
    Otherwise, the leon_path is reset to have the popped directory as its base path.
  @result
    Returns true if the function was able to pop a directory and either store it in *outAltPath
    or allocate a new leon_path pseudo-object to contain it.  On any error the function returns
    false.
*/
bool leon_worklog_getPath(leon_worklog_ref aWorkLog, leon_path_ref *outAltPath);

/*!
  @function leon_worklog_scanComplete
  @discussion
    When the program completes its initial scan of the filesystem it calls this function to
    either commit the newly-produced worklog to the database or discard all changes (e.g.
    if leon_worklog_addPath() failed).  The disposition is controlled by discardChanges; if
    discardChanges is false then changes to the worklog will be committed to the database.
  @result
    Returns true if the discard/commit succeeded.
*/
bool leon_worklog_scanComplete(leon_worklog_ref aWorkLog, bool discardChanges);

#endif /* __LEON_WORKLOG_H__ */
