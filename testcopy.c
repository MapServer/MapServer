/* ========================================================================= 
 * $Id$
 * 
 * Test of msCopyMap()
 *
 * ========================================================================= */
   
#include "map.h"

void printAtts(mapObj*);

int main(int argc, char *argv[]) {

    mapObj *original_map, *clone_map;

    /* ---------------------------------------------------------------------
     * Test 1: free original before freeing clone
     * --------------------------------------------------------------------- */

    // Load map file 
    original_map = msLoadMap("tests/test.map", NULL);
    
    // Dump out some attributes
    printAtts(original_map); 

    // Clone it
    clone_map = msNewMapObj();
    msCopyMap(clone_map, original_map);

    // Dump clone's attributes
    printAtts(clone_map);
   
    // Free
    msFreeMap(original_map);
    msFreeMap(clone_map);

    exit(0);    
}

void printAtts(mapObj *map) {
    printf("\nAttributes\n----------\n");
    printf("Map Name: %s\n", map->name);
    printf("Map Fontset: %s\n", map->fontset.filename);
    printf("Map Symbolset: %s\n", map->symbolset.filename);
}
