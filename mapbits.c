#include "map.h"
#include "maperror.h"

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
