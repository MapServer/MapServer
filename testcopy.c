/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Test of msCopyMap()
 * Author:   Sean Gilles
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
 * Revision 1.8.6.2  2007/04/06 19:29:56  dan
 * Fixed a few more DOS linefeeds
 *
 * Revision 1.8.6.1  2007/04/06 19:27:45  dan
 * Removed DOS linefeeds
 *
 * Revision 1.8  2005/06/14 16:03:35  dan
 * Updated copyright date to 2005
 *
 * Revision 1.7  2005/02/18 03:06:48  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.6  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include "map.h"

MS_CVSID("$Id$")

void printAtts(mapObj*, const char*);

int main(int argc, char *argv[]) {

    mapObj *original_map, *clone_map;

    /* ---------------------------------------------------------------------
     * Test 1: free original before freeing clone
     * --------------------------------------------------------------------- */

    /* Load map file */
    original_map = msLoadMap("tests/test.map", NULL);
    
    /* Dump out some attributes */
    printAtts(original_map, "Original"); 

    /* Clone it */
    clone_map = msNewMapObj();
    msCopyMap(clone_map, original_map);

    /* Write errors */
    msWriteError(stderr);
    msResetErrorList();

    /* Free */
    msFreeMap(original_map);

    /* Dump clone's attributes */
    printAtts(clone_map, "Clone");
  
    /* Free clone */
    msFreeMap(clone_map);

    exit(0);    
}

void printAtts(mapObj *map, const char *title) {
    printf("\n%s Attributes\n----------------------\n", title);
    printf("Map Name: %s\n", map->name);
    printf("Numlayers: %d\n", map->numlayers);
    printf("Map Fontset Filename: %s\n", map->fontset.filename);
    printf("Map Symbolset Filename: %s\n", map->symbolset.filename);
}
