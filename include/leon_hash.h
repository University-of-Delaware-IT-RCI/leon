//
// leon_hash.h
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
// $Id: leon_hash.h 479 2013-09-06 18:33:43Z frey $
//

#ifndef __LEON_HASH_H__
#define __LEON_HASH_H__

#include "leon.h"

/*!
  @header leon_hash.h
  @discussion
    A generalized implementation of a hash table with keys and values being
    pointers.  Key and value behaviors are implemented as callback functions
    that are associated with a hash table object at the time of creation.
    
    Hash tables are always mutable, and have no capacity limit (save the available
    virtual memory in the machine).
    
    A simple enumeration mechanism is implemented using a struct that consumer
    code must allocate (usually on the stack).
*/

/*!
  @typedef leon_hash_key_t
  @discussion
    Type of hash table keys.
*/
typedef const void* leon_hash_key_t;

/*!
  @typedef leon_hash_value_t
  @discussion
    Type of hash table values.
*/
typedef const void* leon_hash_value_t;

//

/*!
	@typedef leon_hash_key_callbacks
	Structure containing the callback functions that implement behaviors
  of hash table keys.
  @field copy
    Duplicate a key, e.g. if the key were a string, call strdup() on it
    NULL => use the key directly
  @field destroy
    Destroy a key, e.g. if the key were a string, call free() on it
    NULL => do nothing
  @field cmp
    Compare two keys for ordering, e.g. if the keys were strings return
    the result of strcmp(key1, key2).  Negative means ordered ascending,
    positive ordered descending, and zero means equal.
  @field hash
    Calculate a 32-bit hash value for the key
  @field print
    Write the value of the key to the given stdio stream
*/
typedef leon_hash_key_t   (*leon_hash_key_copy_callback)(leon_hash_key_t key);
typedef void              (*leon_hash_key_destroy_callback)(leon_hash_key_t key);
typedef int               (*leon_hash_key_cmp_callback)(leon_hash_key_t key1, leon_hash_key_t key2);
typedef uint32_t          (*leon_hash_key_hash_callback)(leon_hash_key_t key);
typedef void              (*leon_hash_key_print_callback)(leon_hash_key_t key, FILE* stream);
typedef struct {
  leon_hash_key_copy_callback         copy;
  leon_hash_key_destroy_callback      destroy;
  leon_hash_key_cmp_callback          cmp;
  leon_hash_key_hash_callback         hash;
  leon_hash_key_print_callback        print;
} leon_hash_key_callbacks;

/*!
  @constant leon_hash_key_cString_callbacks
  @discussion
    Treat hash keys as C strings, making copies of them when added to the table.
*/
extern leon_hash_key_callbacks        leon_hash_key_cString_callbacks;

/*!
  @constant leon_hash_key_cStringNoCopy_callbacks
  @discussion
    Treat hash keys as C string constants, adding them directly and never calling
    free() on them.
*/
extern leon_hash_key_callbacks        leon_hash_key_cStringNoCopy_callbacks;

//

/*!
	@typedef leon_hash_value_callbacks
	Structure containing the callback functions that implement behaviors
  of hash table values.
  @field copy
    Duplicate a value, e.g. if the value were a string, call strdup() on it
    NULL => use the key directly
  @field destroy
    Destroy a value, e.g. if the value were a string, call free() on it
    NULL => do nothing
  @field cmp
    Compare two values for ordering, e.g. if the values were strings return
    the result of strcmp(value1, value2).  Negative means ordered ascending,
    positive ordered descending, and zero means equal.
  @field print
    Write the value to the given stdio stream
*/
typedef leon_hash_value_t (*leon_hash_value_copy_callback)(leon_hash_value_t value);
typedef void              (*leon_hash_value_destroy_callback)(leon_hash_value_t value);
typedef int               (*leon_hash_value_cmp_callback)(leon_hash_value_t value1, leon_hash_value_t value2);
typedef void              (*leon_hash_value_print_callback)(leon_hash_value_t value, FILE* stream);
typedef struct {
  leon_hash_value_copy_callback       copy;
  leon_hash_value_destroy_callback    destroy;
  leon_hash_value_cmp_callback        cmp;
  leon_hash_value_print_callback      print;
} leon_hash_value_callbacks;

/*!
  @constant leon_hash_value_cString_callbacks
  @discussion
    Treat hash values as C strings, making copies of them when added to the table.
*/
extern leon_hash_value_callbacks      leon_hash_value_cString_callbacks;

/*!
  @constant leon_hash_value_cStringNoCopy_callbacks
  @discussion
    Treat hash values as C string constants, adding them directly and never calling
    free() on them.
*/
extern leon_hash_value_callbacks      leon_hash_value_cStringNoCopy_callbacks;

//

/*!
  @function leon_hash_hashBytes
  @discussion
    A generalized hashing function for an arbitrarily-sized array of
    bytes.  Adapted from the algorithm presented on
    
        http://www.azillionmonkeys.com/qed/hash.html
        
*/
uint32_t leon_hash_hashBytes(const void* bytes, size_t length);

/*!
  @function leon_hash_hashCString
  @discussion
    The "k=33" algorithm of Bernstein (the XOR variant).  Walks the C
    string and processes all bytes up to the trailing NUL character.
*/
uint32_t leon_hash_hashCString(const char* cString);

/*!
  @function leon_hash_hashPointer
  @discussion
    Performs the "k=33" algorithm on the 4 (or 8) bytes of the given
    pointer.
*/
uint32_t leon_hash_hashPointer(const void* pointer);

/*!
  @typedef leon_hash_ref
  @discussion
    Type of an opaque reference to a leon_hash pseudo-object.
*/
typedef struct _leon_hash_t * leon_hash_ref;

/*!
  @function leon_hash_create
  @discussion
    Create a new hash table with the given key and value callbacks associated with it.
    The capacity is a guess at the best initial size for the table, but will grow as more
    key-value pairs are added.
    
    If either of keyCallbacks or valueCallbacks is NULL, then the default behavior is used:
    the key/value is treated as a pointer, its value used directly in the table and displayed
    a'la printf() "%p" conversion.
  @result
    Returns NULL on error, otherwise a reference to a leon_hash pseudo-object that should be
    deallocated using leon_worklog_destroy().
*/
leon_hash_ref leon_hash_create(unsigned int capacity, leon_hash_key_callbacks* keyCallbacks, leon_hash_value_callbacks* valueCallbacks);

/*!
  @function leon_hash_destroy
  @discussion
    Deallocate all key-value pairs in theHash and deallocate theHash itself.
*/
void leon_hash_destroy(leon_hash_ref theHash);

/*!
  @function leon_hash_pairCount
  @discussion
    Returns the number of key-value pairs contained in theHash.
*/
unsigned int leon_hash_pairCount(leon_hash_ref theHash);

/*!
  @function leon_hash_containsKey
  @discussion
    Returns true if theHash contains a key-value pair for which the key is equivalent to
    theKey.
  @result
    Returns NULL if theKey does not exist in theHash.
*/
bool leon_hash_containsKey(leon_hash_ref theHash, leon_hash_key_t theKey);

/*!
  @function leon_hash_containsValue
  @discussion
    Returns true if theHash contains a key-value pair for which the value is equivalent to
    theValue.
  @result
    Returns NULL if theValue does not exist in theHash.
*/
bool leon_hash_containsValue(leon_hash_ref theHash, leon_hash_value_t theValue);

/*!
  @function leon_hash_valueForKey
  @discussion
    If theHash contains a key-value pair for which the key is equivalent to theKey, return
    the value of that pair.
  @result
    Returns NULL if theKey does not exist in theHash.
*/
leon_hash_value_t leon_hash_valueForKey(leon_hash_ref theHash, leon_hash_key_t theKey);

/*!
  @function leon_hash_valueForKeyIfPresent
  @discussion
    If theHash contains a key-value pair for which the key is equivalent to theKey, set *theValue
    to the value of that pair and return true.
    
    This function is preferable to leon_hash_valueForKey() if a hash table allows NULL values
    to be present.
  @result
    Returns false if theKey does not exist in theHash.
*/
bool leon_hash_valueForKeyIfPresent(leon_hash_ref theHash, leon_hash_key_t theKey, leon_hash_value_t *theValue);

/*!
  @function leon_hash_setValueForKey
  @discussion
    Add the key-value pair (theKey, theValue) to theHash.  If theKey already exists in theHash then
    the value associated with it is replaced.
*/
void leon_hash_setValueForKey(leon_hash_ref theHash, leon_hash_key_t theKey, leon_hash_value_t theValue);

/*!
  @function leon_hash_removeValueForKey
  @discussion
    If theKey exists in theHash, remove the key-value pair for which key is equivalent to theKey.
*/
void leon_hash_removeValueForKey(leon_hash_ref theHash, leon_hash_key_t theKey);

/*!
  @function leon_hash_removeAllValues
  @discussion
    Remove all key-value pairs from theHash.
*/
void leon_hash_removeAllValues(leon_hash_ref theHash);

/*!
  @function leon_hash_description
  @discussion
    Print a descritpion of theHash to the given stdio stream.  The description includes a header summarizing
    internal properties and interates over all key-value pairs, displaying them via the key and value print
    callbacks that were registered when theHash was created.
*/
void leon_hash_description(leon_hash_ref theHash, FILE* stream);


/*!
  @typedef leon_hash_enum_t
  @discussion
    Structure used to enumerate a leon_hash pseudo-object.  All fields should be
    considered private.  The struct is exposed in this way so that consumer code can
    simply place it on the stack (versus allocating on the heap).
*/
typedef struct {
  void*           r0;
  void*           r1;
  unsigned int    r2, r3;
} leon_hash_enum_t;

/*!
  @function leon_hash_enum_init
  @discussion
    Initialize a hash table enumerator structure, theEnum, to enumerate the contents of
    theHash.
*/
void leon_hash_enum_init(leon_hash_ref theHash, leon_hash_enum_t* theEnum);

/*!
  @function leon_hash_enum_isComplete
  @discussion
    Returns true if the enumeration of theHash, as driven by theEnum, is complete (no
    more key-value pairs remain).
*/
bool leon_hash_enum_isComplete(leon_hash_enum_t* theEnum);

/*!
  @function leon_hash_enum_nextKey
  @discussion
    If the enumeration of theHash represented by theEnum is not complete, advance to
    the next key-value pair and return the key associated with it.
*/
leon_hash_key_t leon_hash_enum_nextKey(leon_hash_enum_t* theEnum);

/*!
  @function leon_hash_enum_nextValue
  @discussion
    If the enumeration of theHash represented by theEnum is not complete, advance to
    the next key-value pair and return the value associated with it.
*/
leon_hash_value_t leon_hash_enum_nextValue(leon_hash_enum_t* theEnum);

#endif /* __LEON_HASH_H__ */
