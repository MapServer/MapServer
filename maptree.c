/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  .qix spatial index implementation.  Derived from shapelib, and
 *           relicensed with permission of Frank Warmerdam (shapelib author).
 * Author:   Steve Lime
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
#include "maptree.h"

#include <limits.h>

#ifdef __BYTE_ORDER__
/* GCC/clang predefined macro */
#define bBigEndian (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#elif defined(_MSC_VER)
/* MSVC doesn't support the C99 trick below, but all Microsoft
   platforms are little-endian */
#define bBigEndian false
#else
/* generic check */
#define bBigEndian                                                             \
  (((union {                                                                   \
     int in;                                                                   \
     char out;                                                                 \
   }){1})                                                                      \
       .out)
#endif

/* -------------------------------------------------------------------- */
/*      If the following is 0.5, nodes will be split in half.  If it    */
/*      is 0.6 then each subnode will contain 60% of the parent         */
/*      node, with 20% representing overlap.  This can be help to       */
/*      prevent small objects on a boundary from shifting too high      */
/*      up the tree.                                                    */
/* -------------------------------------------------------------------- */
#define SPLITRATIO 0.55

static int treeAddShapeId(treeObj *tree, int id, rectObj rect);

static void SwapWord(int length, void *wordP) {
  int i;
  uchar temp;

  for (i = 0; i < length / 2; i++) {
    temp = ((uchar *)wordP)[i];
    ((uchar *)wordP)[i] = ((uchar *)wordP)[length - i - 1];
    ((uchar *)wordP)[length - i - 1] = temp;
  }
}

static void *SfRealloc(void *pMem, int nNewSize)

{
  if (pMem == NULL)
    return ((void *)malloc(nNewSize));
  else
    return ((void *)realloc(pMem, nNewSize));
}

static treeNodeObj *treeNodeCreate(rectObj rect) {
  treeNodeObj *node;

  node = (treeNodeObj *)msSmallMalloc(sizeof(treeNodeObj));

  node->numshapes = 0;
  node->ids = NULL;

  node->numsubnodes = 0;

  memcpy(&(node->rect), &(rect), sizeof(rectObj));

  return node;
}

SHPTreeHandle msSHPDiskTreeOpen(const char *pszTree, int debug) {
  char *pszFullname, *pszBasename;
  SHPTreeHandle psTree;

  char pabyBuf[16];
  int i;

  /* -------------------------------------------------------------------- */
  /*  Initialize the info structure.              */
  /* -------------------------------------------------------------------- */
  psTree = (SHPTreeHandle)msSmallMalloc(sizeof(SHPTreeInfo));

  /* -------------------------------------------------------------------- */
  /*  Compute the base (layer) name.  If there is any extension     */
  /*  on the passed in filename we will strip it off.         */
  /* -------------------------------------------------------------------- */
  pszBasename = (char *)msSmallMalloc(strlen(pszTree) + 5);
  strcpy(pszBasename, pszTree);
  for (i = strlen(pszBasename) - 1;
       i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' &&
       pszBasename[i] != '\\';
       i--) {
  }

  if (pszBasename[i] == '.')
    pszBasename[i] = '\0';

  /* -------------------------------------------------------------------- */
  /*  Open the .shp and .shx files.  Note that files pulled from      */
  /*  a PC to Unix with upper case filenames won't work!        */
  /* -------------------------------------------------------------------- */
  pszFullname = (char *)msSmallMalloc(strlen(pszBasename) + 5);
  sprintf(pszFullname, "%s%s", pszBasename, MS_INDEX_EXTENSION);
  psTree->fp = fopen(pszFullname, "rb");
  if (psTree->fp == NULL) {
    sprintf(pszFullname, "%s.QIX", pszBasename);
    psTree->fp = fopen(pszFullname, "rb");
  }

  msFree(pszBasename); /* don't need these any more */
  msFree(pszFullname);

  if (psTree->fp == NULL) {
    msFree(psTree);
    return (NULL);
  }

  if (fread(pabyBuf, 8, 1, psTree->fp) != 1) {
    msFree(psTree);
    return (NULL);
  }

  memcpy(&psTree->signature, pabyBuf, 3);
  if (strncmp(psTree->signature, "SQT", 3)) {
    /* ---------------------------------------------------------------------- */
    /*     must check if the 2 first bytes equal 0 of max depth that cannot   */
    /*     be more than 65535. If yes, we must swap all value. The problem    */
    /*     here is if there's no Depth (bytea 5,6,7,8 in the file) all bytes  */
    /*     will be set to 0. So,we will test with the number of shapes (bytes */
    /*     1,2,3,4) that cannot be more than 65535 too.                       */
    /* ---------------------------------------------------------------------- */
    if (debug) {
      msDebug("WARNING in msSHPDiskTreeOpen(): %s is in old index format "
              "which has been deprecated.  It is strongly recommended to "
              "regenerate it in new format.\n",
              pszTree);
    }
    if ((pabyBuf[4] == 0 && pabyBuf[5] == 0 && pabyBuf[6] == 0 &&
         pabyBuf[7] == 0)) {
      psTree->LSB_order = !(pabyBuf[0] == 0 && pabyBuf[1] == 0);
    } else {
      psTree->LSB_order = !(pabyBuf[4] == 0 && pabyBuf[5] == 0);
    }
    psTree->needswap = ((psTree->LSB_order) != (!bBigEndian));

    /* ---------------------------------------------------------------------- */
    /*     poor hack to see if this quadtree was created by a computer with a */
    /*     different Endian                                                   */
    /* ---------------------------------------------------------------------- */
    psTree->version = 0;
  } else {
    psTree->needswap = ((pabyBuf[3] == MS_NEW_MSB_ORDER) ^ (bBigEndian));

    psTree->LSB_order = (pabyBuf[3] == MS_NEW_LSB_ORDER);
    memcpy(&psTree->version, pabyBuf + 4, 1);
    memcpy(&psTree->flags, pabyBuf + 5, 3);

    if (fread(pabyBuf, 8, 1, psTree->fp) != 1) {
      msFree(psTree);
      return (NULL);
    }
  }

  if (psTree->needswap)
    SwapWord(4, pabyBuf);
  memcpy(&psTree->nShapes, pabyBuf, 4);

  if (psTree->needswap)
    SwapWord(4, pabyBuf + 4);
  memcpy(&psTree->nDepth, pabyBuf + 4, 4);

  return (psTree);
}

void msSHPDiskTreeClose(SHPTreeHandle disktree) {
  fclose(disktree->fp);
  free(disktree);
}

treeObj *msCreateTree(shapefileObj *shapefile, int maxdepth) {
  int i;
  treeObj *tree;
  rectObj bounds;

  if (!shapefile)
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      Allocate the tree object                                        */
  /* -------------------------------------------------------------------- */
  tree = (treeObj *)msSmallMalloc(sizeof(treeObj));

  tree->numshapes = shapefile->numshapes;
  tree->maxdepth = maxdepth;

  /* -------------------------------------------------------------------- */
  /*      If no max depth was defined, try to select a reasonable one     */
  /*      that implies approximately 8 shapes per node.                   */
  /* -------------------------------------------------------------------- */
  if (tree->maxdepth == 0) {
    int numnodes = 1;

    while (numnodes * 4 < shapefile->numshapes) {
      tree->maxdepth += 1;
      numnodes = numnodes * 2;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Allocate the root node.                                         */
  /* -------------------------------------------------------------------- */
  tree->root = treeNodeCreate(shapefile->bounds);

  for (i = 0; i < shapefile->numshapes; i++) {
    if (msSHPReadBounds(shapefile->hSHP, i, &bounds) == MS_SUCCESS)
      treeAddShapeId(tree, i, bounds);
  }

  return tree;
}

static void destroyTreeNode(treeNodeObj *node) {
  int i;

  for (i = 0; i < node->numsubnodes; i++) {
    if (node->subnode[i])
      destroyTreeNode(node->subnode[i]);
  }

  if (node->ids)
    free(node->ids);

  free(node);
}

void msDestroyTree(treeObj *tree) {
  destroyTreeNode(tree->root);
  free(tree);
}

static void treeSplitBounds(rectObj *in, rectObj *out1, rectObj *out2) {
  double range;

  /* -------------------------------------------------------------------- */
  /*      The output bounds will be very similar to the input bounds,     */
  /*      so just copy over to start.                                     */
  /* -------------------------------------------------------------------- */
  memcpy(out1, in, sizeof(rectObj));
  memcpy(out2, in, sizeof(rectObj));

  /* -------------------------------------------------------------------- */
  /*      Split in X direction.                                           */
  /* -------------------------------------------------------------------- */
  if ((in->maxx - in->minx) > (in->maxy - in->miny)) {
    range = in->maxx - in->minx;

    out1->maxx = in->minx + range * SPLITRATIO;
    out2->minx = in->maxx - range * SPLITRATIO;
  }

  /* -------------------------------------------------------------------- */
  /*      Otherwise split in Y direction.                                 */
  /* -------------------------------------------------------------------- */
  else {
    range = in->maxy - in->miny;

    out1->maxy = in->miny + range * SPLITRATIO;
    out2->miny = in->maxy - range * SPLITRATIO;
  }
}

static int treeNodeAddShapeId(treeNodeObj *node, int id, rectObj rect,
                              int maxdepth) {
  int i;

  /* -------------------------------------------------------------------- */
  /*      If there are subnodes, then consider whether this object        */
  /*      will fit in them.                                               */
  /* -------------------------------------------------------------------- */
  if (maxdepth > 1 && node->numsubnodes > 0) {
    for (i = 0; i < node->numsubnodes; i++) {
      if (msRectContained(&rect, &node->subnode[i]->rect)) {
        return treeNodeAddShapeId(node->subnode[i], id, rect, maxdepth - 1);
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Otherwise, consider creating four subnodes if could fit into    */
  /*      them, and adding to the appropriate subnode.                    */
  /* -------------------------------------------------------------------- */
#if MAX_SUBNODES == 4
  else if (maxdepth > 1 && node->numsubnodes == 0) {
    rectObj half1, half2, quad1, quad2, quad3, quad4;

    treeSplitBounds(&node->rect, &half1, &half2);
    treeSplitBounds(&half1, &quad1, &quad2);
    treeSplitBounds(&half2, &quad3, &quad4);

    if (msRectContained(&rect, &quad1) || msRectContained(&rect, &quad2) ||
        msRectContained(&rect, &quad3) || msRectContained(&rect, &quad4)) {
      node->numsubnodes = 4;
      node->subnode[0] = treeNodeCreate(quad1);
      node->subnode[1] = treeNodeCreate(quad2);
      node->subnode[2] = treeNodeCreate(quad3);
      node->subnode[3] = treeNodeCreate(quad4);

      /* recurse back on this node now that it has subnodes */
      return (treeNodeAddShapeId(node, id, rect, maxdepth));
    }
  }
#endif

  /* -------------------------------------------------------------------- */
  /*      Otherwise, consider creating two subnodes if could fit into     */
  /*      them, and adding to the appropriate subnode.                    */
  /* -------------------------------------------------------------------- */
#if MAX_SUBNODE == 2
  else if (maxdepth > 1 && node->numsubnodes == 0) {
    rectObj half1, half2;

    treeSplitBounds(&node->rect, &half1, &half2);

    if (msRectContained(&rect, &half1)) {
      node->numsubnodes = 2;
      node->subnode[0] = treeNodeCreate(half1);
      node->subnode[1] = treeNodeCreate(half2);

      return (treeNodeAddShapeId(node->subnode[0], id, rect, maxdepth - 1));
    } else if (msRectContained(&rect, &half2)) {
      node->numsubnodes = 2;
      node->subnode[0] = treeNodeCreate(&half1);
      node->subnode[1] = treeNodeCreate(&half2);

      return (treeNodeAddShapeId(node->subnode[1], id, rect, maxdepth - 1));
    }
  }
#endif /* MAX_SUBNODE == 2 */

  /* -------------------------------------------------------------------- */
  /*      If none of that worked, just add it to this nodes list.         */
  /* -------------------------------------------------------------------- */
  node->numshapes++;

  node->ids = SfRealloc(node->ids, sizeof(ms_int32) * node->numshapes);
  node->ids[node->numshapes - 1] = id;

  return MS_TRUE;
}

static int treeAddShapeId(treeObj *tree, int id, rectObj rect) {
  return (treeNodeAddShapeId(tree->root, id, rect, tree->maxdepth));
}

static void treeCollectShapeIds(treeNodeObj *node, rectObj aoi,
                                ms_bitarray status) {
  int i;

  /* -------------------------------------------------------------------- */
  /*      Does this node overlap the area of interest at all?  If not,    */
  /*      return without adding to the list at all.                       */
  /* -------------------------------------------------------------------- */
  if (!msRectOverlap(&node->rect, &aoi))
    return;

  /* -------------------------------------------------------------------- */
  /*      Add the local nodes shapeids to the list.                       */
  /* -------------------------------------------------------------------- */
  for (i = 0; i < node->numshapes; i++)
    msSetBit(status, node->ids[i], 1);

  /* -------------------------------------------------------------------- */
  /*      Recurse to subnodes if they exist.                              */
  /* -------------------------------------------------------------------- */
  for (i = 0; i < node->numsubnodes; i++) {
    if (node->subnode[i])
      treeCollectShapeIds(node->subnode[i], aoi, status);
  }
}

ms_bitarray msSearchTree(const treeObj *tree, rectObj aoi) {
  ms_bitarray status = NULL;

  status = msAllocBitArray(tree->numshapes);
  if (!status) {
    msSetError(MS_MEMERR, NULL, "msSearchTree()");
    return (NULL);
  }

  treeCollectShapeIds(tree->root, aoi, status);

  return (status);
}

static int treeNodeTrim(treeNodeObj *node) {
  int i;

  /* -------------------------------------------------------------------- */
  /*      Trim subtrees, and free subnodes that come back empty.          */
  /* -------------------------------------------------------------------- */
  for (i = 0; i < node->numsubnodes; i++) {
    if (treeNodeTrim(node->subnode[i])) {
      destroyTreeNode(node->subnode[i]);
      node->subnode[i] = node->subnode[node->numsubnodes - 1];
      node->numsubnodes--;
      i--; /* process the new occupant of this subnode entry */
    }
  }

  /* -------------------------------------------------------------------- */
  /*      If the current node has 1 subnode and no shapes, promote that   */
  /*      subnode to the current node position.                           */
  /* -------------------------------------------------------------------- */
  if (node->numsubnodes == 1 && node->numshapes == 0) {
    treeNodeObj *psSubNode = node->subnode[0];

    memcpy(&node->rect, &psSubNode->rect, sizeof(psSubNode->rect));
    node->numshapes = psSubNode->numshapes;
    assert(node->ids == NULL);
    node->ids = psSubNode->ids;
    node->numsubnodes = psSubNode->numsubnodes;
    for (i = 0; i < psSubNode->numsubnodes; i++)
      node->subnode[i] = psSubNode->subnode[i];
    free(psSubNode);
  }

  /* -------------------------------------------------------------------- */
  /*      We should be trimmed if we have no subnodes, and no shapes.     */
  /* -------------------------------------------------------------------- */

  return (node->numsubnodes == 0 && node->numshapes == 0);
}

void msTreeTrim(treeObj *tree) { treeNodeTrim(tree->root); }

static void searchDiskTreeNode(SHPTreeHandle disktree, rectObj aoi,
                               ms_bitarray status) {
  int i;
  ms_int32 offset;
  ms_int32 numshapes, numsubnodes;
  rectObj rect;

  int *ids = NULL;

  if (fread(&offset, 4, 1, disktree->fp) != 1)
    goto error;
  if (disktree->needswap)
    SwapWord(4, &offset);

  if (fread(&rect, sizeof(rectObj), 1, disktree->fp) != 1)
    goto error;
  if (disktree->needswap)
    SwapWord(8, &rect.minx);
  if (disktree->needswap)
    SwapWord(8, &rect.miny);
  if (disktree->needswap)
    SwapWord(8, &rect.maxx);
  if (disktree->needswap)
    SwapWord(8, &rect.maxy);

  if (fread(&numshapes, 4, 1, disktree->fp) != 1)
    goto error;
  if (disktree->needswap)
    SwapWord(4, &numshapes);
  if (numshapes < 0 || numshapes > INT_MAX / 4)
    goto error;

  if (!msRectOverlap(&rect, &aoi)) { /* skip rest of this node and sub-nodes */
    offset += numshapes * sizeof(ms_int32) + sizeof(ms_int32);
    if (fseek(disktree->fp, offset, SEEK_CUR) < 0)
      goto error;
    return;
  }
  if (numshapes > 0) {
    ids = (int *)msSmallMalloc(numshapes * sizeof(ms_int32));

    if (fread(ids, numshapes * sizeof(ms_int32), 1, disktree->fp) != 1)
      goto error;
    if (disktree->needswap) {
      for (i = 0; i < numshapes; i++) {
        SwapWord(4, &ids[i]);
        msSetBit(status, ids[i], 1);
      }
    } else {
      for (i = 0; i < numshapes; i++)
        msSetBit(status, ids[i], 1);
    }
    free(ids);
    ids = NULL;
  }

  if (fread(&numsubnodes, 4, 1, disktree->fp) != 1)
    goto error;
  if (disktree->needswap)
    SwapWord(4, &numsubnodes);
  if (numsubnodes < 0 || numsubnodes > INT_MAX / 4)
    goto error;

  for (i = 0; i < numsubnodes; i++)
    searchDiskTreeNode(disktree, aoi, status);

  return;

error:
  msSetError(MS_IOERR, NULL, "searchDiskTreeNode()");
  free(ids);
  return;
}

ms_bitarray msSearchDiskTree(const char *filename, rectObj aoi, int debug,
                             int numshapes) {
  SHPTreeHandle disktree;
  ms_bitarray status = NULL;

  disktree = msSHPDiskTreeOpen(filename, debug);
  if (!disktree) {

    /* only set this error IF debugging is turned on, gets annoying otherwise */
    if (debug)
      msSetError(
          MS_NOTFOUND,
          "Unable to open spatial index for %s. In most cases you can safely "
          "ignore this message, otherwise check file names and permissions.",
          "msSearchDiskTree()", filename);

    return (NULL);
  }

  if (disktree->nShapes != numshapes) {
    msSetError(MS_SHPERR, "The spatial index file %s is corrupt.",
               "msSearchDiskTree()", filename);
    msSHPDiskTreeClose(disktree);
    return (NULL);
  }

  status = msAllocBitArray(disktree->nShapes);
  if (!status) {
    msSetError(MS_MEMERR, NULL, "msSearchDiskTree()");
    msSHPDiskTreeClose(disktree);
    return (NULL);
  }

  searchDiskTreeNode(disktree, aoi, status);

  msSHPDiskTreeClose(disktree);
  return (status);
}

treeNodeObj *readTreeNode(SHPTreeHandle disktree) {
  int i, res;
  ms_int32 offset;
  treeNodeObj *node;

  node = (treeNodeObj *)msSmallMalloc(sizeof(treeNodeObj));
  node->ids = NULL;

  res = fread(&offset, 4, 1, disktree->fp);
  if (!res) {
    free(node);
    return NULL;
  }

  if (disktree->needswap)
    SwapWord(4, &offset);

  res = fread(&node->rect, sizeof(rectObj), 1, disktree->fp);
  if (!res) {
    free(node);
    return NULL;
  }
  if (disktree->needswap)
    SwapWord(8, &node->rect.minx);
  if (disktree->needswap)
    SwapWord(8, &node->rect.miny);
  if (disktree->needswap)
    SwapWord(8, &node->rect.maxx);
  if (disktree->needswap)
    SwapWord(8, &node->rect.maxy);

  res = fread(&node->numshapes, 4, 1, disktree->fp);
  if (!res) {
    free(node);
    return NULL;
  }
  if (disktree->needswap)
    SwapWord(4, &node->numshapes);
  if (node->numshapes < 0 || node->numshapes > INT_MAX / 4) {
    free(node);
    return NULL;
  }
  if (node->numshapes > 0) {
    node->ids = (ms_int32 *)msSmallMalloc(sizeof(ms_int32) * node->numshapes);
    res = fread(node->ids, node->numshapes * 4, 1, disktree->fp);
    if (!res) {
      free(node->ids);
      free(node);
      return NULL;
    }
  }
  for (i = 0; i < node->numshapes; i++) {
    if (disktree->needswap)
      SwapWord(4, &node->ids[i]);
  }

  res = fread(&node->numsubnodes, 4, 1, disktree->fp);
  if (!res) {
    free(node->ids);
    free(node);
    return NULL;
  }
  if (disktree->needswap)
    SwapWord(4, &node->numsubnodes);

  return node;
}

treeObj *msReadTree(char *filename, int debug) {
  treeObj *tree = NULL;
  SHPTreeHandle disktree;

  disktree = msSHPDiskTreeOpen(filename, debug);
  if (!disktree) {
    msSetError(MS_IOERR, NULL, "msReadTree()");
    return (NULL);
  }

  tree = (treeObj *)malloc(sizeof(treeObj));
  MS_CHECK_ALLOC(tree, sizeof(treeObj), NULL);

  tree->numshapes = disktree->nShapes;
  tree->maxdepth = disktree->nDepth;

  tree->root = readTreeNode(disktree);

  return (tree);
}

static ms_int32 getSubNodeOffset(treeNodeObj *node) {
  int i;
  ms_int32 offset = 0;

  for (i = 0; i < node->numsubnodes; i++) {
    if (node->subnode[i]) {
      offset +=
          sizeof(rectObj) + (node->subnode[i]->numshapes + 3) * sizeof(int);
      offset += getSubNodeOffset(node->subnode[i]);
    }
  }

  /* offset is the disk offset in the index file on disk       */
  /*   that format is (per node)                               */
  /*      int       offset      4 bytes           */
  /*      rectObj   rect      4 * 8 bytes         */
  /*    int   numShapes   4 bytes           */
  /*    int   ids[numShapes]  4 * numShapes bytes     */
  /*    int   numSubNodes   4 bytes           */
  /*                                               */

  return (offset);
}

static void writeTreeNode(SHPTreeHandle disktree, treeNodeObj *node) {
  int i, j;
  ms_int32 offset;
  char *pabyRec = NULL;

  offset = getSubNodeOffset(node);

  pabyRec = msSmallMalloc(sizeof(rectObj) + (3 * sizeof(ms_int32)) +
                          (node->numshapes * sizeof(ms_int32)));

  memcpy(pabyRec, &offset, 4);
  if (disktree->needswap)
    SwapWord(4, pabyRec);

  memcpy(pabyRec + 4, &node->rect, sizeof(rectObj));
  for (i = 0; i < 4; i++)
    if (disktree->needswap)
      SwapWord(8, pabyRec + 4 + (8 * i));

  memcpy(pabyRec + 36, &node->numshapes, 4);
  if (disktree->needswap)
    SwapWord(4, pabyRec + 36);

  j = node->numshapes * sizeof(ms_int32);
  memcpy(pabyRec + 40, node->ids, j);
  for (i = 0; i < node->numshapes; i++)
    if (disktree->needswap)
      SwapWord(4, pabyRec + 40 + (4 * i));

  memcpy(pabyRec + j + 40, &node->numsubnodes, 4);
  if (disktree->needswap)
    SwapWord(4, pabyRec + 40 + j);

  fwrite(pabyRec, 44 + j, 1, disktree->fp);
  free(pabyRec);

  for (i = 0; i < node->numsubnodes; i++) {
    if (node->subnode[i])
      writeTreeNode(disktree, node->subnode[i]);
  }

  return;
}

int msWriteTree(treeObj *tree, char *filename, int B_order) {
  char signature[3] = "SQT";
  char version = 1;
  char reserved[3] = {0, 0, 0};
  SHPTreeHandle disktree;
  int i;
  char mtBigEndian;
  char pabyBuf[32];
  char *pszBasename, *pszFullname;

  disktree = (SHPTreeHandle)malloc(sizeof(SHPTreeInfo));
  MS_CHECK_ALLOC(disktree, sizeof(SHPTreeInfo), MS_FALSE);

  /* -------------------------------------------------------------------- */
  /*  Compute the base (layer) name.  If there is any extension     */
  /*  on the passed in filename we will strip it off.         */
  /* -------------------------------------------------------------------- */
  pszBasename = (char *)msSmallMalloc(strlen(filename) + 5);
  strcpy(pszBasename, filename);
  for (i = strlen(pszBasename) - 1;
       i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' &&
       pszBasename[i] != '\\';
       i--) {
  }

  if (pszBasename[i] == '.')
    pszBasename[i] = '\0';

  /* -------------------------------------------------------------------- */
  /*  Open the .shp and .shx files.  Note that files pulled from      */
  /*  a PC to Unix with upper case filenames won't work!        */
  /* -------------------------------------------------------------------- */
  pszFullname = (char *)msSmallMalloc(strlen(pszBasename) + 5);
  sprintf(pszFullname, "%s%s", pszBasename, MS_INDEX_EXTENSION);
  disktree->fp = fopen(pszFullname, "wb");

  msFree(pszBasename); /* not needed */
  msFree(pszFullname);

  if (!disktree->fp) {
    msFree(disktree);
    msSetError(MS_IOERR, NULL, "msWriteTree()");
    return (MS_FALSE);
  }

  /* for efficiency, trim the tree */
  msTreeTrim(tree);

  /* -------------------------------------------------------------------- */
  /*  Establish the byte order on this machine.         */
  /* -------------------------------------------------------------------- */
  i = 1;
  /* cppcheck-suppress knownConditionTrueFalse */
  if (*((uchar *)&i) == 1)
    mtBigEndian = MS_FALSE;
  else
    mtBigEndian = MS_TRUE;

  if (!(mtBigEndian ^ (B_order == MS_LSB_ORDER || B_order == MS_NEW_LSB_ORDER)))
    disktree->needswap = 1;
  else
    disktree->needswap = 0;

  if (B_order == MS_NATIVE_ORDER)
    disktree->needswap = 0;

  /* write the header */
  if (B_order > 0) {
    memcpy(pabyBuf, &signature, 3);
    memcpy(&disktree->signature, &signature, 3);
    pabyBuf[3] = B_order;

    memcpy(pabyBuf + 4, &version, 1);
    memcpy(pabyBuf + 5, &reserved, 3);

    memcpy(&disktree->version, &version, 1);
    memcpy(&disktree->flags, &reserved, 3);

    fwrite(pabyBuf, 8, 1, disktree->fp);
  }

  memcpy(pabyBuf, &tree->numshapes, 4);
  if (disktree->needswap)
    SwapWord(4, pabyBuf);

  memcpy(pabyBuf + 4, &tree->maxdepth, 4);
  if (disktree->needswap)
    SwapWord(4, pabyBuf + 4);

  i = fwrite(pabyBuf, 8, 1, disktree->fp);
  if (!i) {
    fprintf(stderr, "unable to write to index file ... exiting \n");
    msSHPDiskTreeClose(disktree);
    return (MS_FALSE);
  }

  writeTreeNode(disktree, tree->root);

  msSHPDiskTreeClose(disktree);

  return (MS_TRUE);
}

/* Function to filter search results further against feature bboxes */
void msFilterTreeSearch(shapefileObj *shp, ms_bitarray status,
                        rectObj search_rect) {
  int i;
  rectObj shape_rect;

  i = msGetNextBit(status, 0, shp->numshapes);
  while (i >= 0) {
    if (msSHPReadBounds(shp->hSHP, i, &shape_rect) == MS_SUCCESS) {
      if (msRectOverlap(&shape_rect, &search_rect) != MS_TRUE) {
        msSetBit(status, i, 0);
      }
    }
    i = msGetNextBit(status, i + 1, shp->numshapes);
  }
}
