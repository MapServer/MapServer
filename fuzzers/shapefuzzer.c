#include "../mapserver.h"
#include "../mapshape.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

extern int LLVMFuzzerTestOneInput(GByte *data, size_t size);

static VSILFILE *
SegmentFile(const char *filename, GByte **data_p, size_t *size_p)
{
  GByte *data = *data_p;
  size_t size = *size_p;

  GByte *separator = memmem(data, size, "deadbeef", 8);
  if (separator != NULL) {
    size = separator - data;
    *data_p = separator + 8;
    *size_p -= size + 8;
  } else {
    *size_p = 0;
  }

  return VSIFileFromMemBuffer(filename, data, size, false);
}

int
LLVMFuzzerTestOneInput(GByte *data, size_t size)
{
  /* this fuzzer expects three files concatenated, separated by the
     string "deadbeef"; you can generate such a file by typing:

     { cat foo.shp; echo -n "deadbeef"; cat foo.shx; echo -n "deadbeef"; cat foo.dbf; } >/tmp/corpus/start

     then run the fuzzer:

     ./build/fuzzer/fuzzer /tmp/corpus
  */

  VSILFILE *shp = SegmentFile("/vsimem/foo.shp", &data, &size);
  VSILFILE *shx = SegmentFile("/vsimem/foo.shx", &data, &size);
  VSILFILE *dbf = SegmentFile("/vsimem/foo.dbf", &data, &size);

  shapefileObj file;
  if (msShapefileOpenVirtualFile(&file, "/vsimem/foo.shp", shp, shx, dbf, false) == 0) {
    for (int i = 0; i < file.numshapes; ++i) {
      shapeObj shape;
      msInitShape(&shape);
      msSHPReadShape(file.hSHP, i, &shape);
      msFreeShape(&shape);
    }

    msShapefileClose(&file);
  }

  msResetErrorList();

  return EXIT_SUCCESS;
}
