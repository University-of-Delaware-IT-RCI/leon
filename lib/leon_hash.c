//
// leon_hash.c
// leon - Directory-major scratch filesystem cleanup
//
//
// The leon_hash pseudo-class is a generalized hash table of pointer
// values.
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
// $Id: leon_hash.c 463 2013-08-02 16:51:57Z frey $
//

#include "leon_hash.h"

//

#ifndef LEON_HASH_BASELINE_CAPACITY
#define LEON_HASH_BASELINE_CAPACITY   33
#endif

//

#undef LEON_HASH_PTR_TO_UINT16
#define LEON_HASH_PTR_TO_UINT16(d) (*((const uint16_t *) (d)))

uint32_t
leon_hash_hashBytes(
  const void*       bytes,
  size_t            length
)
{
  uint32_t          hash = 0x9e3779b9, tmp;
  int               remnant;

  if (length <= 0 || bytes == NULL)
    return 0;

  remnant = length & 3;
  length >>= 2;

  /* Main loop */
  while ( length-- > 0 ) {
    hash += LEON_HASH_PTR_TO_UINT16(bytes);
    bytes += 2;
    tmp = (LEON_HASH_PTR_TO_UINT16(bytes) << 11) ^ hash;
    bytes += 2;
    
    hash = (hash << 16) ^ tmp;
    hash += hash >> 11;
    
  }

  /* Handle end cases */
  switch (remnant) {
    case 3: hash += LEON_HASH_PTR_TO_UINT16(bytes);
            hash ^= hash << 16;
            hash ^= ((char*)bytes)[2] << 18;
            hash += hash >> 11;
            break;
    case 2: hash += LEON_HASH_PTR_TO_UINT16(bytes);
            hash ^= hash << 11;
            hash += hash >> 17;
            break;
    case 1: hash += *((char*)bytes);
            hash ^= hash << 10;
            hash += hash >> 1;
            break;
  }

  /* Force "avalanching" of final 127 bits */
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;

  return hash;
}

#undef LEON_HASH_PTR_TO_UINT16

//

uint32_t
leon_hash_hashCString(
  const char*       cString
)
{
  uint32_t          hash = 0;
  uint32_t          c;
  
  if ( cString && (c = *cString) ) {
    hash = 5381;
    do {
      hash = (hash + (hash << 5)) ^ c;
      cString++;
    } while ( (c = *cString) );
  }
  return hash;
}

//

uint32_t
leon_hash_hashPointer(
  const void*       pointer
)
{
  uint32_t          hash = 0;
  uint8_t           *p = (uint8_t*)pointer;
  
  if ( pointer ) {
#if __SIZEOF_PTRDIFF_T__ >= 4
    hash = (hash + (hash << 5)) ^ *p++;
    hash = (hash + (hash << 5)) ^ *p++;
    hash = (hash + (hash << 5)) ^ *p++;
    hash = (hash + (hash << 5)) ^ *p++;
#  if __SIZEOF_PTRDIFF_T__ == 8
#warning Pointer arithmetic is 64-bit
    hash = (hash + (hash << 5)) ^ *p++;
    hash = (hash + (hash << 5)) ^ *p++;
    hash = (hash + (hash << 5)) ^ *p++;
    hash = (hash + (hash << 5)) ^ *p++;
#  else
#warning Pointer arithmetic is 32-bit
#  endif
#endif
  }
  return hash;
}

//
#if 0
#pragma mark -
#endif
//

static const char*    __leon_hash_nullCString = "";

//

leon_hash_key_t
__leon_hash_key_copy_cString(
  leon_hash_key_t     key
)
{
  const char*         cStringKey = (const char*)key;
  
  if ( ! *cStringKey ) return __leon_hash_nullCString;
  return strdup(cStringKey);
}

void
__leon_hash_key_destroy_cString(
  leon_hash_key_t     key
)
{
  if ( key != __leon_hash_nullCString ) free((void*)key);
}

int
__leon_hash_key_cmp_cString(
  leon_hash_key_t     key1,
  leon_hash_key_t     key2
)
{
  return strcmp((const char*)key1, (const char*)key2);
}

uint32_t
__leon_hash_key_hash_cString(
  leon_hash_key_t     key
)
{
  return leon_hash_hashCString((const char*)key);
}

void
__leon_hash_key_print_cString(
  leon_hash_key_t     key,
  FILE*               stream
)
{
  fprintf(stream, "%s", (const char*)key);
}

//

leon_hash_key_callbacks             leon_hash_key_default_callbacks = {
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL
                                          };
                                          
leon_hash_key_callbacks             leon_hash_key_cString_callbacks = {
                                            __leon_hash_key_copy_cString,
                                            __leon_hash_key_destroy_cString,
                                            __leon_hash_key_cmp_cString,
                                            __leon_hash_key_hash_cString,
                                            __leon_hash_key_print_cString
                                          };

leon_hash_key_callbacks             leon_hash_key_cStringNoCopy_callbacks = {
                                            NULL,
                                            NULL,
                                            __leon_hash_key_cmp_cString,
                                            __leon_hash_key_hash_cString,
                                            __leon_hash_key_print_cString
                                          };

//

leon_hash_value_t
__leon_hash_value_copy_cString(
  leon_hash_value_t   value
)
{
  const char*         cStringValue = (const char*)value;
  
  if ( ! *cStringValue ) return __leon_hash_nullCString;
  return strdup(cStringValue);
}

void
__leon_hash_value_destroy_cString(
  leon_hash_value_t   value
)
{
  if ( value != __leon_hash_nullCString ) free((void*)value);
}

int
__leon_hash_value_cmp_cString(
  leon_hash_value_t   value1,
  leon_hash_value_t   value2
)
{
  return strcmp((const char*)value1, (const char*)value2);
}

void
__leon_hash_value_print_cString(
  leon_hash_value_t   value,
  FILE*               stream
)
{
  fprintf(stream, "\"%s\"", (const char*)value);
}

//

leon_hash_value_callbacks             leon_hash_value_default_callbacks = {
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL
                                          };
                                          
leon_hash_value_callbacks             leon_hash_value_cString_callbacks = {
                                            __leon_hash_value_copy_cString,
                                            __leon_hash_value_destroy_cString,
                                            __leon_hash_value_cmp_cString,
                                            __leon_hash_value_print_cString
                                          };

leon_hash_value_callbacks             leon_hash_value_cStringNoCopy_callbacks = {
                                            NULL,
                                            NULL,
                                            __leon_hash_value_cmp_cString,
                                            __leon_hash_value_print_cString
                                          };

//
#if 0
#pragma mark -
#endif
//

typedef struct _leon_hash_node_t {
  leon_hash_key_t       key;
  leon_hash_value_t     value;
  
  struct _leon_hash_node_t *link;
} leon_hash_node_t;

//

leon_hash_node_t*
__leon_hash_node_alloc(void)
{
  return (leon_hash_node_t*)calloc(1, sizeof(leon_hash_node_t));
}

//

void
__leon_hash_node_dealloc(
  leon_hash_node_t                  *node,
  leon_hash_key_destroy_callback    dKey,
  leon_hash_value_destroy_callback  dValue
)
{
  while ( node ) {
    leon_hash_node_t      *next = node->link;
    
    if ( node->key && dKey ) dKey(node->key);
    if ( node->value && dValue ) dKey(node->value);
    free((void*)node);
    node = next;
  }
}

//

leon_hash_node_t*
__leon_hash_node_findKey(
  leon_hash_node_t            *node,
  leon_hash_key_t             key,
  leon_hash_key_cmp_callback  cmp
)
{
  if ( cmp ) {
    while ( node ) {
      int                 cmpVal = cmp(key, node->key);
      
      if ( cmpVal == 0 ) return node;
      if ( cmpVal > 0 ) break;
      node = node->link;
    }
  } else {
    while ( node ) {
      if ( key == node->key ) return node;
      if ( key > node->key ) break;
      node = node->link;
    }
  }
  return NULL;
}

//

leon_hash_node_t*
__leon_hash_node_findValue(
  leon_hash_node_t              *node,
  leon_hash_value_t             value,
  leon_hash_value_cmp_callback  cmp
)
{
  if ( cmp ) {
    while ( node ) {
      if ( cmp(value, node->value) == 0 ) return node;
      node = node->link;
    }
  } else {
    while ( node ) {
      if ( value == node->value ) return node;
      node = node->link;
    }
  }
  return NULL;
}

//
#if 0
#pragma mark -
#endif
//

typedef struct _leon_hash_t {
  leon_hash_key_callbacks         keyCallbacks;
  leon_hash_value_callbacks       valueCallbacks;
  
  unsigned int                    pairCount;
  
  // The table of node buckets:
  unsigned int                    bucketCount;
  unsigned int                    fillCount;
  leon_hash_node_t                **buckets;
  
  // Pool of available nodes:
  leon_hash_node_t                *nodes;
} leon_hash_t;

//

leon_hash_t*
__leon_hash_alloc(
  unsigned int            bucketCount
)
{
  leon_hash_t*            newHash = NULL;
  leon_hash_node_t*       *newBuckets = (leon_hash_node_t**)calloc(bucketCount, sizeof(leon_hash_node_t*));
  
  if ( newBuckets ) {
    if ( (newHash = (leon_hash_t*)calloc(1, sizeof(leon_hash_t))) ) {
      newHash->bucketCount    = bucketCount;
      newHash->buckets        = newBuckets;
    } else {
      free((void*)newBuckets);
    }
  }
  return newHash;
}

//

leon_hash_node_t*
__leon_hash_allocNode(
  leon_hash_t     *theHash
)
{
  leon_hash_node_t*   node;
  
  // Allocate a pair node:
  if ( theHash->nodes ) {
    node = theHash->nodes;
    theHash->nodes = node->link;
    node->key = node->value = node->link = NULL;
  } else {
    node = __leon_hash_node_alloc();
  }
  return node;
}

//
#if 0
#pragma mark -
#endif
//

leon_hash_ref
leon_hash_create(
  unsigned int                  capacity,
  leon_hash_key_callbacks*      keyCallbacks,
  leon_hash_value_callbacks*    valueCallbacks
)
{
  leon_hash_t*                  newHash;
  
  if ( capacity < LEON_HASH_BASELINE_CAPACITY ) capacity = LEON_HASH_BASELINE_CAPACITY;
  if ( (newHash = __leon_hash_alloc(capacity)) ) {
    newHash->keyCallbacks = ( keyCallbacks ? *keyCallbacks : leon_hash_key_default_callbacks );
    newHash->valueCallbacks = ( valueCallbacks ? *valueCallbacks : leon_hash_value_default_callbacks );
  }
  return newHash;
}

//

void
leon_hash_destroy(
  leon_hash_ref     theHash
)
{
  // Destroy any free nodes:
  if ( theHash->nodes ) __leon_hash_node_dealloc(theHash->nodes, NULL, NULL);
  if ( theHash->pairCount ) {
    unsigned int        index = 0;
    
    // Walk the buckets and destroy all nodes therein:
    while ( index < theHash->bucketCount ) {
      leon_hash_node_t    *node = theHash->buckets[index++];
      
      if ( node ) __leon_hash_node_dealloc(node, theHash->keyCallbacks.destroy, theHash->valueCallbacks.destroy);
    }
  }
  free((void*)theHash->buckets);
  free((void*)theHash);
}

//

unsigned int
leon_hash_pairCount(
  leon_hash_ref     theHash
)
{
  return theHash->pairCount;
}

//

bool
leon_hash_containsKey(
  leon_hash_ref       theHash,
  leon_hash_key_t     theKey
)
{
  leon_hash_node_t    *node;
  unsigned int        index;
  uint32_t            keyHash;
  
  keyHash = ( theHash->keyCallbacks.hash ) ? theHash->keyCallbacks.hash(theKey) : leon_hash_hashPointer(theKey);
  index = keyHash % theHash->bucketCount;
  
  if ( (node = theHash->buckets[index]) ) return ( __leon_hash_node_findKey(node, theKey, theHash->keyCallbacks.cmp) ? true : false );
  return false;
}

//

bool
leon_hash_containsValue(
  leon_hash_ref       theHash,
  leon_hash_value_t   theValue
)
{
  unsigned int        index = 0;
  
  while ( index < theHash->bucketCount ) {
    leon_hash_node_t  *node = theHash->buckets[index++];
    
    if ( node && __leon_hash_node_findValue(node, theValue, theHash->valueCallbacks.cmp) ) return true;
  }
  return false;
}

//

leon_hash_value_t
leon_hash_valueForKey(
  leon_hash_ref       theHash,
  leon_hash_key_t     theKey
)
{
  leon_hash_node_t    *node;
  unsigned int        index;
  uint32_t            keyHash;
  
  keyHash = ( theHash->keyCallbacks.hash ) ? theHash->keyCallbacks.hash(theKey) : leon_hash_hashPointer(theKey);
  index = keyHash % theHash->bucketCount;
  
  if ( (node = theHash->buckets[index]) && (node = __leon_hash_node_findKey(node, theKey, theHash->keyCallbacks.cmp)) ) return node->value;
  return NULL;
}

//

bool
leon_hash_valueForKeyIfPresent(
  leon_hash_ref       theHash,
  leon_hash_key_t     theKey,
  leon_hash_value_t   *theValue
)
{
  leon_hash_node_t    *node;
  unsigned int        index;
  uint32_t            keyHash;
  
  keyHash = ( theHash->keyCallbacks.hash ? theHash->keyCallbacks.hash(theKey) : leon_hash_hashPointer(theKey) );
  index = keyHash % theHash->bucketCount;
  
  if ( (node = theHash->buckets[index]) && (node = __leon_hash_node_findKey(node, theKey, theHash->keyCallbacks.cmp)) ) {
    if ( theValue ) *theValue = node->value;
    return true;
  }
  return false;
}

//

void
leon_hash_setValueForKey(
  leon_hash_ref       theHash,
  leon_hash_key_t     theKey,
  leon_hash_value_t   theValue
)
{
  leon_hash_node_t    *node;
  unsigned int        index;
  uint32_t            keyHash;
  
  // Calculate key hash and bucket index:
  keyHash = ( theHash->keyCallbacks.hash ? theHash->keyCallbacks.hash(theKey) : leon_hash_hashPointer(theKey) );
  index = keyHash % theHash->bucketCount;
  
  // Insert into bucket:
  if ( theHash->buckets[index] == NULL ) {
    if ( (node = __leon_hash_allocNode(theHash)) ) {
      theHash->buckets[index] = node;
      theHash->fillCount++;
    }
  } else {
    leon_hash_node_t  *bucket = theHash->buckets[index];
    leon_hash_node_t  *last = NULL;
    int               cmpVal = -1;
    
    if ( theHash->keyCallbacks.cmp ) {
      while ( bucket && ((cmpVal = theHash->keyCallbacks.cmp(theKey, bucket->key)) < 0) ) {
        last = bucket;
        bucket = bucket->link;
      }
    } else {
      while ( bucket ) {
        if ( theKey == bucket->key ) {
          cmpVal = 0;
          break;
        }
        if ( theKey > bucket->key ) {
          cmpVal = 1;
          break;
        }
        last = bucket;
        bucket = bucket->link;
      }
    }
    
    // If cmpVal is zero, then we found a node that already contains the key:
    if ( cmpVal == 0 ) {
      if ( theHash->valueCallbacks.destroy ) theHash->valueCallbacks.destroy(bucket->value);
      bucket->value = ( theHash->valueCallbacks.copy ? theHash->valueCallbacks.copy(theValue) : theValue );
    } else if ( (node = __leon_hash_allocNode(theHash)) ) {
      // If last is NULL, then we're at the head of the chain; otherwise just insert
      // between last and bucket:
      if ( last == NULL ) {
        node->link = bucket;
        theHash->buckets[index] = node;
      } else {
        node->link = bucket;
        last->link = node;
      }
    }
  }
  if ( node ) {
    // Add the pair to the node:
    node->key = ( theHash->keyCallbacks.copy ? theHash->keyCallbacks.copy(theKey) : theKey );
    node->value = ( theHash->valueCallbacks.copy ? theHash->valueCallbacks.copy(theValue) : theValue );
    theHash->pairCount++;
  }
}

//

void
leon_hash_removeValueForKey(
  leon_hash_ref       theHash,
  leon_hash_key_t     theKey
)
{
  leon_hash_node_t    *node;
  unsigned int        index;
  uint32_t            keyHash;
  
  // Calculate key hash and bucket index:
  keyHash = ( theHash->keyCallbacks.hash ? theHash->keyCallbacks.hash(theKey) : leon_hash_hashPointer(theKey) );
  index = keyHash % theHash->bucketCount;
  
  // Delete from bucket if it exists:
  if ( theHash->buckets[index] ) {
    leon_hash_node_t  *bucket = theHash->buckets[index];
    leon_hash_node_t  *last = NULL;
    int               cmpVal = -1;
    
    if ( theHash->keyCallbacks.cmp ) {
      while ( bucket && ((cmpVal = theHash->keyCallbacks.cmp(theKey, bucket->key)) < 0) ) {
        last = bucket;
        bucket = bucket->link;
      }
    } else {
      while ( bucket ) {
        if ( theKey == bucket->key ) {
          cmpVal = 0;
          break;
        }
        if ( theKey > bucket->key ) {
          cmpVal = 1;
          break;
        }
        last = bucket;
        bucket = bucket->link;
      }
    }
    
    // If cmpVal is zero, then we found a node that contains the key:
    if ( cmpVal == 0 ) {
      if ( theHash->keyCallbacks.destroy ) theHash->keyCallbacks.destroy(bucket->key);
      if ( theHash->valueCallbacks.destroy ) theHash->valueCallbacks.destroy(bucket->value);
      
      if ( last ) {
        last->link = bucket->link;
      } else {
        if ( (theHash->buckets[index] = bucket->link) == NULL ) theHash->fillCount--;
      }
      theHash->pairCount--;
      
      // Reclaim the node:
      bucket->key = bucket->value = NULL;
      bucket->link = theHash->nodes;
      theHash->nodes = bucket;
    }
  }
}

//

void
leon_hash_removeAllValues(
  leon_hash_ref       theHash
)
{
  if ( theHash->pairCount ) {
    leon_hash_key_destroy_callback    dKey = theHash->keyCallbacks.destroy;
    leon_hash_value_destroy_callback  dValue = theHash->valueCallbacks.destroy;
    unsigned int                      index = 0;
    
    while ( index < theHash->bucketCount ) {
      leon_hash_node_t*   node = theHash->buckets[index];
      
      theHash->buckets[index++] = NULL;
      
      // Reclaim the nodes:
      while ( node ) {
        leon_hash_node_t* link = node->link;
        
        if ( dKey ) dKey(node->key);
        if ( dValue ) dValue(node->value);
        
        node->key = node->value = NULL;
        node->link = theHash->nodes;
        theHash->nodes = node;
        node = link;
      }
    }
    theHash->pairCount = 0;
    theHash->fillCount = 0;
  }
}

//

void
leon_hash_description(
  leon_hash_ref     theHash,
  FILE*             stream
)
{
  fprintf(
      stream,
      "leon_hash@%p ( %u pairs, %u / %u buckets ) {\n",
      theHash, theHash->pairCount, theHash->fillCount, theHash->bucketCount
    );
  if ( theHash->pairCount ) {
    leon_hash_key_print_callback    keyPrint = theHash->keyCallbacks.print;
    leon_hash_value_print_callback  valuePrint = theHash->valueCallbacks.print;
    unsigned int                    index = 0;
    
    if ( keyPrint ) {
      if ( valuePrint ) {
        while ( index < theHash->bucketCount ) {
          leon_hash_node_t  *node = theHash->buckets[index++];
          
          while ( node ) {
            fprintf(stream, "  ");
            keyPrint(node->key, stream);
            fprintf(stream, " = ");
            valuePrint(node->value, stream);
            fputc('\n', stream);
            node = node->link;
          }
        }
      } else {
        while ( index < theHash->bucketCount ) {
          leon_hash_node_t  *node = theHash->buckets[index++];
          
          while ( node ) {
            fprintf(stream, "  ");
            keyPrint(node->key, stream);
            fprintf(stream, " = %p\n", node->value);
            node = node->link;
          }
        }
      }
    } else if ( valuePrint ) {
        while ( index < theHash->bucketCount ) {
          leon_hash_node_t  *node = theHash->buckets[index++];
          
          while ( node ) {
            fprintf(stream, "  %p = ", node->key);
            valuePrint(node->value, stream);
            fputc('\n', stream);
            node = node->link;
          }
        }
    } else {
      while ( index < theHash->bucketCount ) {
        leon_hash_node_t  *node = theHash->buckets[index++];
        
        while ( node ) {
          fprintf(stream, "  %p = %p\n", node->key, node->value);
          node = node->link;
        }
      }
    }
  }
  fprintf(stream, "}\n");
}

//
#if 0
#pragma mark -
#endif
//

static inline bool
__leon_hash_enum_locateNextNode(
  leon_hash_enum_t*   theEnum
)
{
  bool                nextBucket = true;
  
  if ( (theEnum->r2 == theEnum->r3) && (theEnum->r1 == NULL) ) return false;
  
  if ( theEnum->r1 ) {
    if ( ((leon_hash_node_t*)theEnum->r1)->link ) {
      theEnum->r1 = ((leon_hash_node_t*)theEnum->r1)->link;
      return true;
    }
  }
  if ( nextBucket ) {
    theEnum->r1 = NULL;
    while ( ++theEnum->r2 < theEnum->r3 ) {
      if ( (theEnum->r1 = ((leon_hash_node_t**)theEnum->r0)[ theEnum->r2 ]) ) return true;
    }
  }
  return false;
}

//

void
leon_hash_enum_init(
  leon_hash_ref       theHash,
  leon_hash_enum_t*   theEnum
)
{
  if ( theHash->pairCount == 0 ) {
    theEnum->r0 = theEnum->r1 = NULL;
    theEnum->r2 = theEnum->r3 = 0;
  } else {
    theEnum->r0 = theHash->buckets;
    theEnum->r2 = 0;
    theEnum->r3 = theHash->bucketCount;
    if ( (theEnum->r1 = theHash->buckets[0]) == NULL ) __leon_hash_enum_locateNextNode(theEnum);
  }
}

//

bool
leon_hash_enum_isComplete(
  leon_hash_enum_t*   theEnum
)
{
  return ( ((theEnum->r2 == theEnum->r3) && (theEnum->r1 == NULL)) ? true : false );
}

//

leon_hash_key_t
leon_hash_enum_nextKey(
  leon_hash_enum_t*   theEnum
)
{
  leon_hash_key_t     key = NULL;
  
  if ( (theEnum->r2 == theEnum->r3) && (theEnum->r1 == NULL) ) return NULL;
  
  key = ((leon_hash_node_t*)theEnum->r1)->key;
  __leon_hash_enum_locateNextNode(theEnum);
  return key;
}

//

leon_hash_value_t
leon_hash_enum_nextValue(
  leon_hash_enum_t*   theEnum
)
{
  leon_hash_value_t   value = NULL;
  
  if ( (theEnum->r2 == theEnum->r3) && (theEnum->r1 == NULL) ) return NULL;
  
  value = ((leon_hash_node_t*)theEnum->r1)->value;
  __leon_hash_enum_locateNextNode(theEnum);
  return value;
}

//
#if 0
#pragma mark -
#endif
//

#ifdef LEON_HASH_MAIN

int
main()
{
  leon_hash_ref         myHash = leon_hash_create(0, &leon_hash_key_cString_callbacks, &leon_hash_value_cString_callbacks);
  
  if ( myHash ) {
    leon_hash_enum_t    myEnum;
    
    leon_hash_description(myHash, stdout);
    
    leon_hash_setValueForKey(myHash, "x", "Letmein");
    leon_hash_setValueForKey(myHash, "y", "Dummy");
    leon_hash_setValueForKey(myHash, "z", "Lorem ipsum");
    leon_hash_description(myHash, stdout);
    
    leon_hash_removeAllValues(myHash);
    leon_hash_description(myHash, stdout);
    
    leon_hash_setValueForKey(myHash, "y", "No dummy");
    leon_hash_setValueForKey(myHash, "π", "3.14159");
    leon_hash_setValueForKey(myHash, "abcdefg", "hijklmnop");
    leon_hash_description(myHash, stdout);
    
    leon_hash_setValueForKey(myHash, "y", "Dummy");
    leon_hash_description(myHash, stdout);
    
    leon_hash_removeValueForKey(myHash, "y");
    leon_hash_removeValueForKey(myHash, "π");
    leon_hash_description(myHash, stdout);
    
    // Enumerate:
    printf("\nEnumerate values:\n\n");
    leon_hash_enum_init(myHash, &myEnum);
    while ( ! leon_hash_enum_isComplete(&myEnum) ) {
      leon_hash_value_t   v = leon_hash_enum_nextValue(&myEnum);
      
      printf("%s\n", (const char*)v);
    }
    
    leon_hash_destroy(myHash);
  }
  return 0;
}

#endif
