/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  .qix spatial index declarations.
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
 * Revision 1.10  2005/08/25 14:20:16  sdlime
 * Applied patch for bug 1440.
 *
 * Revision 1.9  2005/06/14 16:03:35  dan
 * Updated copyright date to 2005
 *
 * Revision 1.8  2005/02/18 03:06:47  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.7  2004/10/21 04:30:56  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#ifndef MAPTREE_H
#define MAPTREE_H

#ifdef __cplusplus
extern "C" {
#endif

/* this can be 2 or 4 for binary or quad tree */
#define MAX_SUBNODES 4
   
typedef struct shape_tree_node {
  /* area covered by this node */
  rectObj rect;
  
  /* list of shapes stored at this node. */
  ms_int32 numshapes;
  ms_int32 *ids;
  
  int numsubnodes;
  struct shape_tree_node *subnode[MAX_SUBNODES];
} treeNodeObj;

typedef struct {
  ms_int32 numshapes;
  ms_int32 maxdepth;
  treeNodeObj *root;
} treeObj;


typedef struct
{
    FILE        *fp;
    char        signature[3];
    char	LSB_order;
    char        needswap;
    char	version;
    char	flags[3];

    ms_int32        nShapes;
    ms_int32        nDepth;
} SHPTreeInfo;
typedef SHPTreeInfo * SHPTreeHandle;

#define MS_LSB_ORDER -1
#define MS_MSB_ORDER -2
#define MS_NATIVE_ORDER 0
#define MS_NEW_LSB_ORDER 1
#define MS_NEW_MSB_ORDER 2


MS_DLL_EXPORT SHPTreeHandle msSHPDiskTreeOpen(const char * pszTree, int debug);
MS_DLL_EXPORT void msSHPDiskTreeClose(SHPTreeHandle disktree);
MS_DLL_EXPORT treeNodeObj *readTreeNode( SHPTreeHandle disktree );

MS_DLL_EXPORT treeObj *msCreateTree(shapefileObj *shapefile, int maxdepth);
MS_DLL_EXPORT void msTreeTrim(treeObj *tree);
MS_DLL_EXPORT void msDestroyTree(treeObj *tree);

MS_DLL_EXPORT char *msSearchTree(treeObj *tree, rectObj aoi);
MS_DLL_EXPORT char *msSearchDiskTree(char *filename, rectObj aoi, int debug);

MS_DLL_EXPORT treeObj *msReadTree(char *filename, int debug);
MS_DLL_EXPORT int msWriteTree(treeObj *tree, char *filename, int LSB_order);

MS_DLL_EXPORT void msFilterTreeSearch(shapefileObj *shp, char *status, rectObj search_rect);

#ifdef __cplusplus
}
#endif

#endif /* MAPTREE_H */
