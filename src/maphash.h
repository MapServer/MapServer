/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Declarations for the hashTableObj and related stuff.
 * Author:   Sean Gillies, sgillies@frii.com
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef MAPHASH__H
#define MAPHASH__H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define MS_DLL_EXPORT     __declspec(dllexport)
#else
#define  MS_DLL_EXPORT
#endif

#define MS_HASHSIZE 41

  /* =========================================================================
   * Structs
   * ========================================================================= */

#ifndef SWIG
  struct hashObj {
    struct hashObj *next;  /* pointer to next item */
    char           *key;   /* string key that is hashed */
    char           *data;  /* string stored in this item */
  };
#endif /*SWIG*/


/**
 * An object to store key-value pairs
 *
 */
  typedef struct {
#ifndef SWIG
    struct hashObj **items;  /* the hash table */
#endif
#ifdef SWIG
    %immutable;
#endif /*SWIG*/
    int              numitems;  ///< \**immutable** number of items
#ifdef SWIG
    %mutable;
#endif /*SWIG*/
  } hashTableObj;

  /* =========================================================================
   * Functions
   * ========================================================================= */

#ifndef SWIG
  /* msCreateHashTable - create a hash table
   * ARGS:
   *     None
   * RETURNS:
   *     New hash table
   */
  MS_DLL_EXPORT hashTableObj *msCreateHashTable( void );

  /* For use in mapfile.c with hashTableObj structure members */
  MS_DLL_EXPORT int initHashTable( hashTableObj *table );

  /* msFreeHashTable - free allocated memory
   * ARGS:
   *     table - target hash table
   * RETURNS:
   *     None
   */
  MS_DLL_EXPORT void msFreeHashTable( hashTableObj *table );

  /* Free only the items for hashTableObj structure members (metadata, &c) */
  MS_DLL_EXPORT void msFreeHashItems( hashTableObj *table );

  /* msInsertHashTable - insert new item
   * ARGS:
   *     table - the target hash table
   *     key   - key string for new item
   *     value - data string for new item
   * RETURNS:
   *     pointer to the new item or NULL
   * EXCEPTIONS:
   *     raise MS_HASHERR on failure
   */
  MS_DLL_EXPORT struct hashObj *msInsertHashTable( hashTableObj *table,
      const char *key,
      const char *value );

  /* msLookupHashTable - get the value of an item by its key
   * ARGS:
   *     table - the target hash table
   *     key   - key string of item
   * RETURNS:
   *     string value of item
   */
  MS_DLL_EXPORT const char *msLookupHashTable( const hashTableObj *table, const char *key);

  /* msRemoveHashTable - remove item from table at key
   * ARGS:
   *     table - target hash table
   *     key   - key string
   * RETURNS:
   *     MS_SUCCESS or MS_FAILURE
   */
  MS_DLL_EXPORT int msRemoveHashTable( hashTableObj *table, const char *key);

  /* msFirstKeyFromHashTable - get key of first item
   * ARGS:
   *     table - target hash table
   * RETURNS:
   *     first key as a string
   */
  MS_DLL_EXPORT const char *msFirstKeyFromHashTable( const hashTableObj *table );

  /* msNextKeyFromHashTable - get next key
   * ARGS:
   *     table - target hash table
   *     prevkey - the previous key
   * RETURNS:
   *     the key of the item of following prevkey as a string
   */
  MS_DLL_EXPORT const char *msNextKeyFromHashTable( const hashTableObj *table,
      const char *prevkey );

  /* msHashIsEmpty - get next key
   * ARGS:
   *     table - target hash table
   * RETURNS:
   *     MS_TRUE if the table is empty and MS_FALSE if the table has items
   */

  MS_DLL_EXPORT int msHashIsEmpty( const hashTableObj* table );

#endif /*SWIG*/

#ifdef __cplusplus
}
#endif

#endif /* MAPHASH__H */
