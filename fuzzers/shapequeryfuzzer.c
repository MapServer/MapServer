/* Fuzzer for the MapServer shapefile stack: geometry/index/DBF/quadtree.
 *
 * Threat model: .shp/.shx/.dbf/.qix files are untrusted data files opened by
 * MapServer (SHAPEPATH layers, tile indices, shp2img-style tooling). The
 * existing shapefuzzer covers open + shape reading; this harness additionally
 * drives the DBF attribute read paths (mapxbase.c) and the quadtree spatial
 * index paths (maptree.c: disk-tree parsing and in-memory tree build/search),
 * plus bounding-box filtering, none of which any current target reaches.
 *
 * Input format: five segments separated by the fixed string "deadbeef":
 *   <foo.shp bytes> "deadbeef" <foo.shx bytes> "deadbeef"
 *   <foo.dbf bytes> "deadbeef" <foo.qix bytes> "deadbeef" <32-byte search rect>
 * The segment layout follows the existing shapefuzzer convention; the final
 * 16 bytes are four native doubles (minx,miny,maxx,maxy) of the query
 * rectangle and are optional (the shapefile bounds are used when absent).
 */
/* struct stat64 (VSIStatBufL) is only complete with LFS feature macros; the
 * explicit sys/stat.h include below needs this before any system header. */
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <math.h>

#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"
#include "src/mapserver.h"
#include "src/mapshape.h"
#include "src/maptree.h"

#define kMaxShapes 100000
/* Bound the per-iteration DBF record reads */
#define kMaxDbfRecords 1000
#define kMaxDbfFields 64

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size);
extern int LLVMFuzzerInitialize(int *argc, char ***argv);

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;
  return 0;
}

static void *msMemmem(const void *haystack, size_t haystack_len,
                      const void *const needle, const size_t needle_len) {
  if (haystack_len == 0)
    return NULL;
  if (needle_len == 0)
    return NULL;

  for (const char *h = (const char *)haystack; haystack_len >= needle_len;
       ++h, --haystack_len) {
    if (!memcmp(h, needle, needle_len)) {
      return (void *)h;
    }
  }
  return NULL;
}


/* Read all shapes indicated by a hit bitarray, with the standard guard */
static void ReadHitShapes(shapefileObj *file, ms_bitarray hits,
                          errorObj *ms_error) {
  for (int i = 0; i < file->numshapes; ++i) {
    if (!msGetBit(hits, i))
      continue;
    shapeObj shape;
    msInitShape(&shape);
    msSHPReadShape(file->hSHP, i, &shape);
    msFreeShape(&shape);
    if (ms_error->code != MS_NOERR)
      break;
  }
}

/* Locate the first occurrence of the "deadbeef" separator at or after
 * *from_p; returns its absolute offset or the sentinel "not found". */
#define SEG_NPOS ((size_t)-1)

static size_t FindSep(const uint8_t *data, size_t size, size_t from) {
  if (from >= size)
    return SEG_NPOS;
  void *sep = msMemmem(data + from, size - from, "deadbeef", 8);
  if (sep == NULL)
    return SEG_NPOS;
  return (size_t)((const uint8_t *)sep - data);
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  /* Four separators delimit five segments: shp | shx | dbf | qix | rect.
   * Degenerate inputs (missing separators / empty required segments) exercise
   * nothing beyond the open-failure path the existing shapefuzzer already
   * covers, so they are skipped before any /vsimem file is created. */
  size_t sep[4];
  size_t pos = 0;
  int n;
  for (n = 0; n < 4; n++) {
    pos = FindSep(Data, Size, pos);
    if (pos == SEG_NPOS)
      break;
    sep[n] = pos;
    pos += 8;
  }
  if (n < 3)
    return 0;

  size_t shpOff = 0, shpLen = sep[0];
  size_t shxOff = sep[0] + 8, shxLen = sep[1] - shxOff;
  size_t dbfOff = sep[1] + 8, dbfLen = sep[2] - dbfOff;
  size_t qixOff = sep[2] + 8;
  size_t qixLen;
  const uint8_t *qixData;
  size_t rectOff, rectSize;
  if (n == 4) {
    qixLen = sep[3] - qixOff;
    qixData = Data + qixOff;
    rectOff = sep[3] + 8;
    rectSize = Size - rectOff;
  } else {
    qixLen = Size - qixOff;
    qixData = Data + qixOff;
    rectOff = 0;
    rectSize = 0;
  }
  if (shpLen == 0 || shxLen == 0 || dbfLen == 0)
    return 0;

  /* Remaining bytes (if any) are the query rectangle: four native doubles */
  rectObj rect;
  if (rectSize >= 4 * sizeof(double)) {
    memcpy(&rect.minx, Data + rectOff, sizeof(double));
    memcpy(&rect.miny, Data + rectOff + sizeof(double), sizeof(double));
    memcpy(&rect.maxx, Data + rectOff + 2 * sizeof(double), sizeof(double));
    memcpy(&rect.maxy, Data + rectOff + 3 * sizeof(double), sizeof(double));
  } else {
    rect.minx = rect.miny = rect.maxx = rect.maxy = 0.0;
  }

  VSILFILE *shp = VSIFileFromMemBuffer("/vsimem/foo.shp", (GByte *)Data + shpOff,
                                       shpLen, false);
  VSILFILE *shx = VSIFileFromMemBuffer("/vsimem/foo.shx", (GByte *)Data + shxOff,
                                       shxLen, false);
  VSILFILE *dbf = VSIFileFromMemBuffer("/vsimem/foo.dbf", (GByte *)Data + dbfOff,
                                       dbfLen, false);
  if (!shp || !shx || !dbf) {
    if (shp)
      VSIFCloseL(shp);
    if (shx)
      VSIFCloseL(shx);
    if (dbf)
      VSIFCloseL(dbf);
    VSIUnlink("/vsimem/foo.shp");
    VSIUnlink("/vsimem/foo.shx");
    VSIUnlink("/vsimem/foo.dbf");
    return 0;
  }

  shapefileObj file;
  errorObj *ms_error = msGetErrorObj();
  if (msShapefileOpenVirtualFile(&file, "/vsimem/foo.shp", shp, shx, dbf,
                                 false) == 0) {
    if (file.numshapes > kMaxShapes) {
      VSIStatBufL sStat;
      if (VSIStatL("/vsimem/foo.shx", &sStat) == 0 && sStat.st_size >= 100 &&
          file.numshapes > (int)(sStat.st_size - 100) / 8) {
        file.numshapes = (int)(sStat.st_size - 100) / 8;
      }
    }
    for (int i = 0; i < file.numshapes; ++i) {
      shapeObj shape;
      msInitShape(&shape);
      msSHPReadShape(file.hSHP, i, &shape);
      msFreeShape(&shape);
      /* Give up as soon an error is triggered to avoid too long processing
       * time. */
      if (ms_error->code != MS_NOERR)
        break;
    }

    /* DBF attribute read paths (mapxbase.c) */
    int numrecords = file.hDBF ? msDBFGetRecordCount(file.hDBF) : 0;
    if (numrecords > kMaxDbfRecords)
      numrecords = kMaxDbfRecords;
    int numfields = file.hDBF ? msDBFGetFieldCount(file.hDBF) : 0;
    if (numfields > kMaxDbfFields)
      numfields = kMaxDbfFields;
    for (int r = 0; r < numrecords && ms_error->code == MS_NOERR; ++r) {
      for (int f = 0; f < numfields; ++f) {
        switch (msDBFGetFieldInfo(file.hDBF, f, NULL, NULL, NULL)) {
          case FTInteger:
            (void)msDBFReadIntegerAttribute(file.hDBF, r, f);
            break;
          case FTDouble:
            (void)msDBFReadDoubleAttribute(file.hDBF, r, f);
            break;
          case FTString:
            /* returns internal storage; do not free */
            (void)msDBFReadStringAttribute(file.hDBF, r, f);
            break;
          default:
            break;
        }
      }
    }


    /* Quadtree paths (maptree.c). The fuzzed .qix is written to a real file
     * because msSHPDiskTreeOpen uses fopen(); a non-matching or corrupt index
     * falls back to the in-memory tree build from the fuzzed geometries. */
    if (!(isfinite(rect.minx) && isfinite(rect.miny) && isfinite(rect.maxx) &&
          isfinite(rect.maxy) && rect.minx <= rect.maxx &&
          rect.miny <= rect.maxy)) {
      rect = file.bounds;
    }

    ms_bitarray hits = NULL;
    const char *dir = CPLGenerateTempFilename(NULL);
    char *qixpath = msStrdup(CPLSPrintf("%s/tree.qix", dir));
    /* The qix segment is read straight from the input buffer (no /vsimem
     * file) and written to a real file, because msSHPDiskTreeOpen uses
     * fopen(); a non-matching or corrupt index falls back to the in-memory
     * tree build from the fuzzed geometries. */
    char *qixbuf = NULL;
    size_t qsize = qixLen;
    if (qsize > 0) {
      qixbuf = (char *)msSmallMalloc(qsize);
      memcpy(qixbuf, qixData, qsize);
    }
    FILE *fp = fopen(qixpath, "wb");
    if (fp && qixbuf) {
      fwrite(qixbuf, 1, qsize, fp);
      fclose(fp);
      fp = NULL;
      hits = msSearchDiskTree(CPLSPrintf("%s/tree", dir), rect, 0,
                              file.numshapes);
    }
    if (fp)
      fclose(fp);
    msFree(qixbuf);
    unlink(qixpath);
    rmdir(dir);
    msFree(qixpath);

    if (hits) {
      /* disk-tree hits: apply the shape-level overlap filter, then read */
      msFilterTreeSearch(&file, hits, rect);
      ReadHitShapes(&file, hits, ms_error);
      free(hits);
    } else {
      /* in-memory tree build + search from the fuzzed geometries */
      treeObj *tree = msCreateTree(&file, -1);
      if (tree) {
        ms_bitarray memhits = msSearchTree(tree, rect);
        if (memhits) {
          ReadHitShapes(&file, memhits, ms_error);
          free(memhits);
        }
        msDestroyTree(tree);
      }
    }

    msShapefileClose(&file);
  }

  msResetErrorList();

  VSIUnlink("/vsimem/foo.shp");
  VSIUnlink("/vsimem/foo.shx");
  VSIUnlink("/vsimem/foo.dbf");

  return EXIT_SUCCESS;
}

