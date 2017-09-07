//
// leon_path.h
// leon - Directory-major scratch filesystem cleanup
//
//
// The leon_path pseudo-class is used to manipulate filesystem paths in a push/pop
// manner.  The associated C string will grow as components are pushed to the
// path.  Popping a component restores each previous state, back to the original
// C string when the pseudo-object was created.
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
// $Id: leon_path.h 479 2013-09-06 18:33:43Z frey $
//

#ifndef __LEON_PATH_H__
#define __LEON_PATH_H__

#include "leon.h"

/*!
  @header leon_path.h
  @discussion
    The leon_path pseudo-class is used to manipulate filesystem paths in a push/pop
    manner.  The associated C string will grow as components are pushed to the
    path.  Popping a component restores each previous state, back to the original
    C string when the pseudo-object was created.
    
    This API is not thread safe.
*/

/*!
  @typedef leon_path_ref
  @discussion
    The type of an opaque reference to a path pseudo-object.
*/
typedef struct __leon_path_t * leon_path_ref;

/*!
  @function leon_path_createEmpty
  @discussion
    Create a new path with an empty base path component, a'la
    the root path.
  @result
    Returns NULL on error, otherwise a reference to a path pseudo-object
    that should be deallocated using leon_path_destroy().
*/
leon_path_ref leon_path_createEmpty(void);

/*!
  @function leon_path_createWithCString
  @discussion
    Create a new path with cString as the base path component.
  @result
    Returns NULL on error, otherwise a reference to a path pseudo-object
    that should be deallocated using leon_path_destroy().
*/
leon_path_ref leon_path_createWithCString(const char* cString);

/*!
  @function leon_path_createWithCStrings
  @discussion
    Create a new path with its base path component set to the concatenation
    of cString1 and subsequent C strings.  The argument list must be NULL
    terminated:
    
      leon_path_createWithCStrings("etc", "sysconfig", "network-scripts", \
          NULL)
    
    would produce a base path of "etc/sysconfig/network-scripts" in the
    returned object.
  @result
    Returns NULL on error, otherwise a reference to a path pseudo-object
    that should be deallocated using leon_path_destroy().
*/
leon_path_ref leon_path_createWithCStrings(const char* cString1, ...);

/*!
  @function leon_path_copy
  @discussion
    Create a new path that uses the full path of aPath (not just its base
    path) for its own base path.
  @result
    Returns NULL on error, otherwise a reference to a path pseudo-object
    that should be deallocated using leon_path_destroy().
*/
leon_path_ref leon_path_copy(leon_path_ref aPath);

/*!
  @function leon_path_destroy
  @discussion
    Deallocate a path pseudo-object.
*/
void leon_path_destroy(leon_path_ref aPath);

/*!
  @function leon_path_cString
  @discussion
    Retrieve a C string pointer representing the full path contained in
    aPath.  The returned C string should not be altered in any way.
*/
const char* leon_path_cString(leon_path_ref aPath);

/*!
  @function leon_path_lastComponent
  @discussion
    Retrieve a C string pointer representing the last path component contained
    in aPath.  The returned C string should not be altered in any way.
*/
const char* leon_path_lastComponent(leon_path_ref aPath);

/*!
  @function leon_path_resetBasePath
  @discussion
    Erase the path currently represented by aPath and replace its base
    component with the C string in newBasePath.
*/
void leon_path_resetBasePath(leon_path_ref aPath, const char* newBasePath);

/*!
  @function leon_path_append
  @discussion
    Append suffix to aPath.  If aPath contained
    
      /etc/sysconfig
      
    and "iptables" was appended, aPath would then contain
    
      /etc/sysconfigiptables
    
    This is in contrast to appending path components using leon_path_push()
    or leon_path_pushFormat().
*/
void leon_path_append(leon_path_ref aPath, const char* suffix);

/*!
  @function leon_path_appendFormat
  @discussion
    Converts the printf()-style format string using any additional arguments
    passed to the function and appends the result a'la leon_path_append().
*/
void leon_path_appendFormat(leon_path_ref aPath, const char* format, ...);

/*!
  @function leon_path_depth
  @discussion
    Returns the number of path components that have been pushed onto aPath.
*/
unsigned int leon_path_depth(leon_path_ref aPath);

/*!
  @function leon_path_push
  @discussion
    Append the path component cString to aPath.  If aPath contained
    
      /etc/sysconfig
      
    and "iptables" was pushed, aPath would then contain
    
      /etc/sysconfig/iptables
    
    This is in contrast to appending path components using leon_path_append()
    or leon_path_appendFormat().
*/
void leon_path_push(leon_path_ref aPath, const char* cString);

/*!
  @function leon_path_pushFormat
  @discussion
    Converts the printf()-style format string using any additional arguments
    passed to the function and pushed the result a'la leon_path_push().
*/
void leon_path_pushFormat(leon_path_ref aPath, const char* format, ...);

/*!
  @function leon_path_pop
  @discussion
    Remove the last-pushed path component from aPath.  If aPath was
    
      /etc[/sysconfig][/iptables]
      
    and this function was called, it would then contain
    
      /etc[/sysconfig]
*/
void leon_path_pop(leon_path_ref aPath);

/*!
  @function leon_path_description
  @discussion
    Writes the content of aPath to stdout.  If shouldShowSnapshots is true,
    then the components that were pushed onto the base path are delimited by
    square brackets, a'la
    
        /etc[/sysconfig][/iptables]
        
    versus
    
        /etc/sysconfig/iptables
*/
void leon_path_description(leon_path_ref aPath, bool shouldShowSnapshots);

/*!
  @function leon_path_doesExist
  @discussion
    Returns true if a filesystem object exists at the path contained in aPath.
*/
bool leon_path_doesExist(leon_path_ref aPath);

/*!
  @function leon_path_isFile
  @discussion
    Returns true if a filesystem object exists at the path contained in aPath
    and is a regular file.
*/
bool leon_path_isFile(leon_path_ref aPath);

/*!
  @function leon_path_isDirectory
  @discussion
    Returns true if a filesystem object exists at the path contained in aPath
    and is a directory.
*/
bool leon_path_isDirectory(leon_path_ref aPath);

#endif /* __LEON_PATH_H__ */
