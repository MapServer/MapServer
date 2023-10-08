/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of bit array functions.
 * Author:   Steve Lime and the MapServer team.
 *
 * Notes: Derived from code placed in the public domain by Bob Stout, for more
 * information see http://c.snippets.org/snip_lister.php?fname=bitarray.c.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.

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

#include "mapserver.h"

#include <limits.h>

/*
 * Hardcoded size of our bit array.
 * See function msGetNextBit for another hardcoded value.
 */

/* #define msGetBit(array, index) (*((array) + (index)/MS_ARRAY_BIT) & ( 1 <<
 * ((index) % MS_ARRAY_BIT))) */

size_t msGetBitArraySize(int numbits) {
  return ((numbits + MS_ARRAY_BIT - 1) / MS_ARRAY_BIT);
}

ms_bitarray msAllocBitArray(int numbits) {
  ms_bitarray array =
      calloc((numbits + MS_ARRAY_BIT - 1) / MS_ARRAY_BIT, MS_ARRAY_BIT);

  return (array);
}

int msGetBit(ms_const_bitarray array, int index) {
  array += index / MS_ARRAY_BIT;
  return (*array & (1U << (index % MS_ARRAY_BIT))) != 0; /* 0 or 1 */
}

/*
** msGetNextBit( status, start, size)
**
** Quickly find the next bit set. If start == 0 and 0 is set, will return 0.
** If hits end of bitmap without finding set bit, will return -1.
**
*/
int msGetNextBit(ms_const_bitarray array, int i, int size) {

  register ms_uint32 b;

  while (i < size) {
    b = *(array + (i / MS_ARRAY_BIT));
    if (b && (b >> (i % MS_ARRAY_BIT))) {
      /* There is something in this byte */
      /* And it is not to the right of us */
      if (b & (1U << (i % MS_ARRAY_BIT))) {
        /* There is something at this bit! */
        return i;
      } else {
        i++;
      }
    } else {
      /* Nothing in this byte, move to start of next byte */
      i += MS_ARRAY_BIT - (i % MS_ARRAY_BIT);
    }
  }

  /* Got to the last byte with no hits! */
  return -1;
}

void msSetBit(ms_bitarray array, int index, int value) {
  array += index / MS_ARRAY_BIT;
  if (value)
    *array |= 1U << (index % MS_ARRAY_BIT); /* set bit */
  else
    *array &= ~(1U << (index % MS_ARRAY_BIT)); /* clear bit */
}

void msSetAllBits(ms_bitarray array, int numbits, int value) {
  if (value)
    memset(array, 0xff, ((numbits + 7) / 8)); /* set bit */
  else
    memset(array, 0x0, ((numbits + 7) / 8)); /* clear bit */
}

void msFlipBit(ms_bitarray array, int index) {
  array += index / MS_ARRAY_BIT;
  *array ^= 1U << (index % MS_ARRAY_BIT); /* flip bit */
}
