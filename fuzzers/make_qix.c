/* Helper that writes a quadtree spatial index (.qix) for a shapefile, used
 * at build time to generate shapequeryfuzzer seed corpora entries that
 * exercise the disk-tree parsing path (msSearchDiskTree).
 *
 * Not a fuzz target: only compiled in the fuzzers' build.sh seed step.
 */
#include <stdio.h>

#include "src/mapserver.h"
#include "src/mapshape.h"
#include "src/maptree.h"

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <shapefile-base> <output-base>\n", argv[0]);
    return 1;
  }

  shapefileObj file;
  if (msShapefileOpen(&file, "rb", argv[1], 0) != 0) {
    fprintf(stderr, "cannot open %s\n", argv[1]);
    return 1;
  }

  treeObj *tree = msCreateTree(&file, -1);
  int rc = 1;
  if (tree) {
    rc = (msWriteTree(tree, argv[2], 1) == MS_SUCCESS) ? 0 : 1;
    msDestroyTree(tree);
  }
  msShapefileClose(&file);
  return rc;
}
