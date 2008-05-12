/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of bit array functions.
 * Author:   Steve Lime and the MapServer team.
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

#include "mapserver.h"

MS_CVSID("$Id$")

#include <limits.h>

/* originally found at http://www.snippets.org/ */

size_t msGetBitArraySize(int numbits)
{
  return((numbits + CHAR_BIT - 1) / CHAR_BIT);
}

char *msAllocBitArray(int numbits)
{
  char *array = calloc((numbits + CHAR_BIT - 1) / CHAR_BIT, sizeof(char));
  
  return(array);
}

int msGetBit(char *array, int index)
{
  array += index / CHAR_BIT;
  return (*array & (1 << (index % CHAR_BIT))) != 0;    /* 0 or 1 */
}

/*
** msGetNextBit( status, start, size)
**
** Quickly find the next bit set. If start == 0 and 0 is set, will return 0.
** If hits end of bitmap without finding set bit, will return -1.
**
*/
int msGetNextBit(char *array, int i, int size) { 

  register char b;

  while(i < size) {
    b = *(array + (i/CHAR_BIT));
    if( b && (b >> (i % CHAR_BIT)) ) {
      /* There is something in this byte */
      /* And it is not to the right of us */
      if( msGetBit( array, i ) ) {
        return i;
      }
      else {
        i++;
      }
    }
    else {
      /* Nothing in this byte, move to start of next byte */
      i += CHAR_BIT - (i % CHAR_BIT);
    }
  }
  
  /* Got to the last byte with no hits! */
  return -1;
}
  
void msSetBit(char *array, int index, int value)
{
  array += index / CHAR_BIT;
  if (value)
    *array |= 1 << (index % CHAR_BIT);           /* set bit */
  else    
    *array &= ~(1 << (index % CHAR_BIT));        /* clear bit */
}

void msFlipBit(char *array, int index)
{
  array += index / CHAR_BIT;
  *array ^= 1 << (index % CHAR_BIT);                   /* flip bit */
}
