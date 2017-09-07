//
// leon_fstest.c
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
// $Id: leon_fstest.c 468 2013-08-21 18:36:51Z frey $
//

#include "leon_fstest.h"
#include "leon_log.h"

bool        leon_fstest_excludeRoot = true;
time_t      leon_fstest_temporalThreshold = 0;

//

typedef struct _leon_fstest_node_t {
  struct _leon_fstest_node_t  *link;
  
  leon_fstest_callback        callback;
  const void*               	context;
  char                        name[1];
} leon_fstest_node_t;

static leon_fstest_node_t* _leon_fstest_stack = NULL;


//

leon_fstest_node_t*
__leon_fstest_node_alloc(
  size_t        nameLen
)
{
  nameLen += sizeof(leon_fstest_node_t);
  return (leon_fstest_node_t*)calloc(1, nameLen);
}

//

void
leon_fstest_registerCallback(
  const char*           aName,
  leon_fstest_callback  aCallback,
  const void*           context
)
{
  leon_fstest_node_t    *prev = NULL, *node = _leon_fstest_stack;
  
  while ( node ) {
    if ( strcmp(&node->name[0], aName) == 0 ) {
      node->callback = aCallback;
      node->context = context;
      return;
    }
    prev = node;
    node = node->link;
  }
  
  if ( (node = __leon_fstest_node_alloc(strlen(aName))) ) {
    strcpy(&node->name[0], aName);
    node->callback = aCallback;
    node->context = context;
    if ( prev ) {
      prev->link = node;
    } else {
      _leon_fstest_stack = node;
    }
  }
}

//

void
leon_fstest_description(void)
{
  leon_fstest_node_t    *node = _leon_fstest_stack;
  int                   i = 1;
  
  leon_log(kLeonLogInfo, "Filesystem test stack:");
  leon_log(kLeonLogInfo, "  (0) default tests");
  while ( node ) {
    leon_log(kLeonLogInfo, "  (%d) %s", i++, &node->name[0]);
    node = node->link;
  }
}

//

void
leon_fstest_unregisterCallback(
  const char*       aName
)
{
  leon_fstest_node_t    *prev = NULL, *node = _leon_fstest_stack;
  
  while ( node ) {
    if ( strcmp(&node->name[0], aName) == 0 ) {
      if ( prev ) {
        prev->link = node->link;
      } else {
        _leon_fstest_stack = node->link;
      }
      free((void*)node);
      return;
    }
    prev = node;
    node = node->link;
  }
}

//

leon_result_t
leon_fstest_checkPathModificationTimes(
  const char*     path,
  struct stat*    pathInfo
)
{
  leon_fstest_node_t    *node = _leon_fstest_stack;
  leon_result_t         result = kLeonResultUnknown;
    
  leon_log(kLeonLogDebug2, "leon_fstest_checkPath: %s", path);
  
  if ( leon_stat(path, pathInfo) == 0 ) {
    //
    // If we're ignoring stuff owned by root, check that now:
    //
    if ( leon_fstest_excludeRoot && ((pathInfo->st_uid == 0) || (pathInfo->st_gid == 0)) ) return kLeonResultNo;
    
    //
    // If the modification time is newer than the threshold, short-circuit:
    //
    if ( pathInfo->st_mtime >= leon_fstest_temporalThreshold ) return kLeonResultNo;
    
    // Barring any disgreement from the callback chain, this item
    // should be deleted:
    result = kLeonResultYes;
    while ( node ) {
      result = node->callback(path, pathInfo, node->context);
      leon_log(kLeonLogDebug2, "leon_fstest_checkPath: %s(%s) = %d", &node->name[0], path, result);
      if ( result != kLeonResultYes ) break;
      node = node->link;
    }
  }
  return result;
}

//

leon_result_t
leon_fstest_checkPathAccessTimes(
  const char*     path,
  struct stat*    pathInfo
)
{
  leon_fstest_node_t    *node = _leon_fstest_stack;
  leon_result_t         result = kLeonResultUnknown;
    
  leon_log(kLeonLogDebug2, "leon_fstest_checkPath: %s", path);
  
  if ( leon_stat(path, pathInfo) == 0 ) {
    //
    // If we're ignoring stuff owned by root, check that now:
    //
    if ( leon_fstest_excludeRoot && ((pathInfo->st_uid == 0) || (pathInfo->st_gid == 0)) ) return kLeonResultNo;
    
    //
    // If the modification time is newer than the threshold, short-circuit:
    //
    if ( pathInfo->st_atime >= leon_fstest_temporalThreshold ) return kLeonResultNo;
    
    // Barring any disgreement from the callback chain, this item
    // should be deleted:
    result = kLeonResultYes;
    while ( node ) {
      result = node->callback(path, pathInfo, node->context);
      leon_log(kLeonLogDebug2, "leon_fstest_checkPath: %s(%s) = %d", &node->name[0], path, result);
      if ( result != kLeonResultYes ) break;
      node = node->link;
    }
  }
  return result;
}

//

leon_result_t
leon_fstest_checkPathMaxTimes(
  const char*     path,
  struct stat*    pathInfo
)
{
  leon_fstest_node_t    *node = _leon_fstest_stack;
  leon_result_t         result = kLeonResultUnknown;
    
  leon_log(kLeonLogDebug2, "leon_fstest_checkPath: %s", path);
  
  if ( leon_stat(path, pathInfo) == 0 ) {
    time_t              lastUpdate;
    
    //
    // If we're ignoring stuff owned by root, check that now:
    //
    if ( leon_fstest_excludeRoot && ((pathInfo->st_uid == 0) || (pathInfo->st_gid == 0)) ) return kLeonResultNo;
    
    //
    // If the modification time is newer than the threshold, short-circuit:
    //
    if ( (lastUpdate = pathInfo->st_atime) < pathInfo->st_mtime ) lastUpdate = pathInfo->st_mtime;
    if ( lastUpdate >= leon_fstest_temporalThreshold ) return kLeonResultNo;
    
    // Barring any disgreement from the callback chain, this item
    // should be deleted:
    result = kLeonResultYes;
    while ( node ) {
      result = node->callback(path, pathInfo, node->context);
      leon_log(kLeonLogDebug2, "leon_fstest_checkPath: %s(%s) = %d", &node->name[0], path, result);
      if ( result != kLeonResultYes ) break;
      node = node->link;
    }
  }
  return result;
}

