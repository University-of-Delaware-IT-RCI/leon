//
// leon_path.c
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
// $Id: leon_path.c 479 2013-09-06 18:33:43Z frey $
//

#include "leon_path.h"
#include "leon_stat.h"
#include <stdarg.h>

//

char*
leon_stpncpy(
  char*         dst,
  const char*   src,
  size_t        n
)
{
  if ( n ) {
    while ( n && *src ) {
      *dst++ = *src++;
      n--;
    }
    if ( n == 0 ) dst--;
    *dst = '\0';
  }
  return dst;
}

//

typedef struct __leon_path_snapshot_t {
  struct __leon_path_snapshot_t   *link;
  size_t                          length;
} leon_path_snapshot_t;

leon_path_snapshot_t*             __leon_path_snapshot_pool = NULL;

//

leon_path_snapshot_t*
__leon_path_snapshot_alloc(
  size_t    length
)
{
  leon_path_snapshot_t*     ss = NULL;
  
  if ( __leon_path_snapshot_pool ) {
    ss = __leon_path_snapshot_pool;
    __leon_path_snapshot_pool = ss->link;
  } else {
    ss = malloc(sizeof(leon_path_snapshot_t));
  }
  if ( ss ) {
    ss->link = NULL;
    ss->length = length;
  }
  return ss;
}

//

void
__leon_path_snapshot_free(
  leon_path_snapshot_t*     ss
)
{
  ss->link = __leon_path_snapshot_pool;
  __leon_path_snapshot_pool = ss;
}

//

typedef struct __leon_path_t {
  char*                           cString;
  size_t                          length, capacity;
  leon_path_snapshot_t            *snapshot;
} leon_path_t;

//

leon_path_t*
__leon_path_alloc(
  size_t      capacity
)
{
  leon_path_t*    new_path = (leon_path_t*)malloc(sizeof(leon_path_t));
  
  if ( capacity < 32 ) capacity = 32;
  
  if ( new_path ) {
    if ( (new_path->cString = (char*)malloc(capacity)) ) {
      new_path->length = 0;
      new_path->capacity = capacity;
      new_path->snapshot = NULL;
    } else {
      free((void*)new_path);
      new_path = NULL;
    }
  }
  return new_path;
}

//

leon_path_ref
leon_path_createEmpty(void)
{
  return __leon_path_alloc(0);
}

//

leon_path_ref
leon_path_createWithCString(
  const char*       cString
)
{
  size_t            length = strlen(cString);
  leon_path_t*      new_path = __leon_path_alloc(1 + length);
  
  if ( new_path ) {
    char*           end = leon_stpncpy(new_path->cString, cString, length + 1);
    
    if ( end ) new_path->length = end - new_path->cString;
  }
  return new_path;
}

//
    
leon_path_ref
leon_path_createWithCStrings(
  const char*     cString1,
  ...
)
{
  size_t            length = 0;
  leon_path_t*      new_path = NULL;
  va_list           vargs;
  char*             s;
  
  if ( cString1 ) {
    length = strlen(cString1);
    va_start(vargs, cString1);
    while ( (s = va_arg(vargs, char*)) ) {
      length += 1 + strlen(s);
    }
    va_end(vargs);
  }
  new_path = __leon_path_alloc(1 + length);
  if ( new_path && cString1 ) {
    size_t          capacity = 1 + length;
    size_t          frag_length;
    char            *dst = new_path->cString;
    char            *end;
    
    length = 0;
    if ( (end = leon_stpncpy(dst, cString1, capacity)) ) {
      frag_length = end - dst;
      capacity -= frag_length;
      length += frag_length;
      dst += frag_length;
      va_start(vargs, cString1);
      while ( (s = va_arg(vargs, char*)) ) {
        if ( capacity > 1 ) {
          *dst++ = '/';
          capacity--;
          length++;
          if ( (end = leon_stpncpy(dst, s, capacity)) ) {
            frag_length = end - dst;
            capacity -= frag_length;
            length += frag_length;
            dst += frag_length;
          }
        }
      }
      va_end(vargs);
    }
    new_path->length = length;
  }
  return new_path;
}

//

leon_path_ref
leon_path_copy(
  leon_path_ref       aPath
)
{
  size_t            length = aPath->length;
  leon_path_t*      new_path = __leon_path_alloc(1 + length);
  
  if ( new_path && length ) {
    char*           end = leon_stpncpy(new_path->cString, aPath->cString, length + 1);
    
    if ( end ) new_path->length = end - new_path->cString;
  }
  return new_path;
}

//

void
leon_path_destroy(
  leon_path_ref       aPath
)
{
  leon_path_snapshot_t* ss = aPath->snapshot;
  
  while ( ss ) {
    leon_path_snapshot_t* link = ss->link;
    
    __leon_path_snapshot_free(ss);
    ss = link;
  }
  if ( aPath->cString) free((void*)aPath->cString);
  free((void*)aPath);
}

//

const char*
leon_path_cString(
  leon_path_ref       aPath
)
{
  return aPath->cString;
}

//

const char*
leon_path_lastComponent(
  leon_path_ref       aPath
)
{
  const char*         s = aPath->cString;
  
  if ( s && *s ) {
    const char*       p = s;
    
    while ( *s ) {
      if (*s == '/') p = s + 1;
      s++;
    }
    return p;
  }
  return NULL;
}

//

void
leon_path_resetBasePath(
  leon_path_ref       aPath,
  const char*         newBasePath
)
{
  size_t                newBasePathLen = strlen(newBasePath);
  leon_path_snapshot_t  *ss = aPath->snapshot;
  char*                 end;
  
  // Do we have the capacity?
  if ( aPath->capacity <= newBasePathLen ) {
    size_t          new_capacity = newBasePathLen + 1;
    char*           new_str;
    
    new_capacity = 32 * ((new_capacity / 32) + ((new_capacity % 32) ? 1 : 0));
    new_str = (char*)realloc(aPath->cString, new_capacity);
    if ( ! new_str ) return;
    aPath->capacity = new_capacity;
    aPath->cString = new_str;
  }
  
  // Drop all snapshots:
  while ( ss ) {
    leon_path_snapshot_t  *link = ss->link;
    
    __leon_path_snapshot_free(ss);
    ss = link;
  }
  
  // Copy-in the new bits:
  end = leon_stpncpy(aPath->cString, newBasePath, aPath->capacity);
  if ( end ) aPath->length = end - aPath->cString;
}

//

void
leon_path_append(
  leon_path_ref     aPath,
  const char*       suffix
)
{
  size_t            suffixLen = strlen(suffix);
  char*             end;
  
  // Do we have the capacity?
  if ( aPath->capacity <= aPath->length + suffixLen ) {
    size_t          new_capacity = aPath->length + suffixLen + 1;
    char*           new_str;
    
    new_capacity = 32 * ((new_capacity / 32) + ((new_capacity % 32) ? 1 : 0));
    new_str = (char*)realloc(aPath->cString, new_capacity);
    if ( ! new_str ) return;
    aPath->capacity = new_capacity;
    aPath->cString = new_str;
  }
  
  // Copy-in the new bits:
  end = leon_stpncpy(aPath->cString + aPath->length, suffix, aPath->capacity - aPath->length);
  if ( end ) aPath->length = end - aPath->cString;
}

//

void
leon_path_appendFormat(
  leon_path_ref       aPath,
  const char*         format,
  ...
)
{
  va_list             vargs;
  size_t              bufferLen;
  
  va_start(vargs, format);
  bufferLen = vsnprintf(NULL, 0, format, vargs);
  va_end(vargs);
  
  if ( bufferLen > 0 ) {
    // Do we have the capacity?
    if ( aPath->capacity <= aPath->length + bufferLen ) {
      size_t          new_capacity = aPath->length + bufferLen + 1;
      char*           new_str;
      
      new_capacity = 32 * ((new_capacity / 32) + ((new_capacity % 32) ? 1 : 0));
      new_str = (char*)realloc(aPath->cString, new_capacity);
      if ( ! new_str ) return;
      aPath->capacity = new_capacity;
      aPath->cString = new_str;
    }
    
    va_start(vargs, format);
    bufferLen = vsnprintf(aPath->cString + aPath->length, bufferLen + 1, format, vargs);
    va_end(vargs);
    aPath->length += bufferLen;
  }
}

//

unsigned int
leon_path_depth(
  leon_path_ref       aPath
)
{
  unsigned int          depth = 0;
  leon_path_snapshot_t  *ss = aPath->snapshot;

  while ( ss ) {
    depth++;
    ss = ss->link;
  }
  return depth;
}

//

void
leon_path_push(
  leon_path_ref       aPath,
  const char*         cString
)
{
  size_t              frag_length = strlen(cString);
  
  if ( frag_length ) {
    size_t                  new_capacity = aPath->length + 1 + frag_length + 1;
    leon_path_snapshot_t    *ss = NULL;
    char*                   end;
    
    // Realloc if necessary:
    if ( new_capacity > aPath->capacity ) {
      char*           new_str;
      
      new_capacity = 32 * ((new_capacity / 32) + ((new_capacity % 32) ? 1 : 0));
      new_str = (char*)realloc(aPath->cString, new_capacity);
      if ( ! new_str ) return;
      aPath->capacity = new_capacity;
      aPath->cString = new_str;
    }
    
    // Take a snapshot:
    if ( ! (ss = __leon_path_snapshot_alloc(aPath->length)) ) return;
    ss->link = aPath->snapshot;
    aPath->snapshot = ss;
    
    // Copy-in the new bits:
    aPath->cString[aPath->length++] = '/';
    end = leon_stpncpy(aPath->cString + aPath->length, cString, aPath->capacity - aPath->length);
    if ( end ) aPath->length = end - aPath->cString;
  }
}

//

void
leon_path_pushFormat(
  leon_path_ref       aPath,
  const char*         format, 
  ...
)
{
  va_list             vargs;
  int                 frag_length = 0;
  
  va_start(vargs, format);
  frag_length = vsnprintf(NULL, 0, format, vargs);
  va_end(vargs);
  
  if ( frag_length > 0 ) {
    size_t                  new_capacity = aPath->length + 1 + frag_length + 1;
    leon_path_snapshot_t    *ss = NULL;
    char*                   end;
    
    // Realloc if necessary:
    if ( new_capacity > aPath->capacity ) {
      char*           new_str;
      
      new_capacity = 32 * ((new_capacity / 32) + ((new_capacity % 32) ? 1 : 0));
      new_str = (char*)realloc(aPath->cString, new_capacity);
      if ( ! new_str ) return;
      aPath->capacity = new_capacity;
      aPath->cString = new_str;
    }
    
    // Take a snapshot:
    if ( ! (ss = __leon_path_snapshot_alloc(aPath->length)) ) return;
    ss->link = aPath->snapshot;
    aPath->snapshot = ss;
    
    // Copy-in the new bits:
    aPath->cString[aPath->length++] = '/';
    va_start(vargs, format);
    frag_length = vsnprintf(aPath->cString + aPath->length, frag_length + 1, format, vargs);
    va_end(vargs);
    aPath->length += frag_length;
  }
}

//

void
leon_path_pop(
  leon_path_ref       aPath
)
{
  if ( aPath->snapshot ) {
    leon_path_snapshot_t    *ss = aPath->snapshot;
    
    aPath->snapshot = ss->link;
    aPath->length = ss->length;
    aPath->cString[aPath->length] = '\0';
    __leon_path_snapshot_free(ss);
  }
}

//

void
__leon_path_description_driver(
  char*                   base_ptr,
  leon_path_snapshot_t    *ss,
  size_t                  start,
  size_t                  length
)
{
  if ( ss ) {
    __leon_path_description_driver(base_ptr, ss->link, 0, ss->length);
    start = ss->length;
    length -= ss->length;
  }
  fputc('[', stdout);
  base_ptr += start;
  while ( length-- ) fputc(*base_ptr++, stdout);
  fputc(']', stdout);
}

void
leon_path_description(
  leon_path_ref     aPath,
  bool              shouldShowSnapshots
)
{
  if ( shouldShowSnapshots ) {
    __leon_path_description_driver(aPath->cString, aPath->snapshot, 0, aPath->length);
  } else {
    printf("%s", aPath->cString);
  }
}

//

bool
leon_path_doesExist(
  leon_path_ref     aPath
)
{
  struct stat       fInfo;
  
  if ( leon_stat(aPath->cString, &fInfo) == 0 ) {
    return true;
  }
  return false;
}
bool
leon_path_isFile(
  leon_path_ref     aPath
)
{
  struct stat       fInfo;
  
  if ( leon_stat(aPath->cString, &fInfo) == 0 ) {
    return ( (fInfo.st_mode & S_IFMT) == S_IFREG ) ? true : false;
  }
  return false;
}
bool
leon_path_isDirectory(
  leon_path_ref     aPath
)
{
  struct stat       fInfo;
  
  if ( leon_stat(aPath->cString, &fInfo) == 0 ) {
    return ( (fInfo.st_mode & S_IFMT) == S_IFDIR ) ? true : false;
  }
  return false;
}

