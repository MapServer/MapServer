/******************************************************************************
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.3  2005/06/14 16:03:33  dan
 * Updated copyright date to 2005
 *
 * Revision 1.2  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include "map.h"

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
