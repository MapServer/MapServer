/*
** This code is based in part on the previous work of Frank Warmerdam. See
** the README for licence details.
*/

#include "map.h"
#include "maptree.h"

/* -------------------------------------------------------------------- */
/*      If the following is 0.5, nodes will be split in half.  If it    */
/*      is 0.6 then each subnode will contain 60% of the parent         */
/*      node, with 20% representing overlap.  This can be help to       */
/*      prevent small objects on a boundary from shifting too high      */
/*      up the tree.                                                    */
/* -------------------------------------------------------------------- */
#define SPLITRATIO  0.55

typedef unsigned char uchar;

static int treeAddShapeId(treeObj *tree, int id, rectObj rect);

static void SwapWord( int length, void * wordP )
{
  int i;
  uchar	temp;
  
  for( i=0; i < length/2; i++ )
    {
      temp = ((uchar *) wordP)[i];
      ((uchar *)wordP)[i] = ((uchar *) wordP)[length-i-1];
      ((uchar *) wordP)[length-i-1] = temp;
    }
}

static void * SfRealloc( void * pMem, int nNewSize )

{
    if( pMem == NULL )
        return( (void *) malloc(nNewSize) );
    else
        return( (void *) realloc(pMem,nNewSize) );
}

static treeNodeObj *treeNodeCreate(rectObj rect)
{
    treeNodeObj	*node;

    node = (treeNodeObj *) malloc(sizeof(treeNodeObj));

    node->numshapes = 0;
    node->ids = NULL;

    node->numsubnodes = 0;

    memcpy(&(node->rect), &(rect), sizeof(rectObj));

    return node;
}

treeObj *msCreateTree(shapefileObj *shapefile, int maxdepth)
{
  int i;
  treeObj *tree;
  rectObj bounds;

  if(!shapefile) return NULL;

  /* -------------------------------------------------------------------- */
  /*      Allocate the tree object                                        */
  /* -------------------------------------------------------------------- */
  tree = (treeObj *) malloc(sizeof(treeObj));
  
  tree->numshapes = shapefile->numshapes;
  tree->maxdepth = maxdepth;

  /* -------------------------------------------------------------------- */
  /*      If no max depth was defined, try to select a reasonable one     */
  /*      that implies approximately 8 shapes per node.                   */
  /* -------------------------------------------------------------------- */
  if( tree->maxdepth == 0 ) {
    int numnodes = 1;
    
    while(numnodes*4 < shapefile->numshapes) {
      tree->maxdepth += 1;
      numnodes = numnodes * 2;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Allocate the root node.                                         */
  /* -------------------------------------------------------------------- */
  tree->root = treeNodeCreate(shapefile->bounds);

  for(i=0; i<shapefile->numshapes; i++) {
    SHPReadBounds(shapefile->hSHP, i, &bounds);
    treeAddShapeId(tree, i, bounds);
  }

  return tree;
}

static void destroyTreeNode(treeNodeObj *node)
{
  int i;
  
  for(i=0; i<node->numsubnodes; i++ ) {
    if(node->subnode[i]) 
      destroyTreeNode(node->subnode[i]);
  }
  
  if(node->ids)
    free(node->ids);

  free(node);
}

void msDestroyTree(treeObj *tree)
{
  destroyTreeNode(tree->root);
  free(tree);
}

static void treeSplitBounds( rectObj *in, rectObj *out1, rectObj *out2)
{
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
  if((in->maxx - in->minx) > (in->maxy - in->miny)) {
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

static int treeNodeAddShapeId( treeNodeObj *node, int id, rectObj rect, int maxdepth)
{
  int i;
    
  /* -------------------------------------------------------------------- */
  /*      If there are subnodes, then consider wiether this object        */
  /*      will fit in them.                                               */
  /* -------------------------------------------------------------------- */
  if( maxdepth > 1 && node->numsubnodes > 0 ) {
    for(i=0; i<node->numsubnodes; i++ ) {
      if( msRectContained(&rect, &node->subnode[i]->rect)) {
	return treeNodeAddShapeId( node->subnode[i], id, rect, maxdepth-1);
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Otherwise, consider creating four subnodes if could fit into    */
  /*      them, and adding to the appropriate subnode.                    */
  /* -------------------------------------------------------------------- */
#if MAX_SUBNODES == 4
  else if( maxdepth > 1 && node->numsubnodes == 0 ) {
    rectObj half1, half2, quad1, quad2, quad3, quad4;

    treeSplitBounds(&node->rect, &half1, &half2);
    treeSplitBounds(&half1, &quad1, &quad2);
    treeSplitBounds(&half2, &quad3, &quad4);
  
    if(msRectContained(&rect, &quad1) || msRectContained(&rect, &quad2) || 	msRectContained(&rect, &quad3) || msRectContained(&rect, &quad4)) {
      node->numsubnodes = 4;
      node->subnode[0] = treeNodeCreate(quad1);
      node->subnode[1] = treeNodeCreate(quad2);
      node->subnode[2] = treeNodeCreate(quad3);
      node->subnode[3] = treeNodeCreate(quad4);
      
      /* recurse back on this node now that it has subnodes */
      return(treeNodeAddShapeId(node, id, rect, maxdepth));
    }
  }
#endif

    /* -------------------------------------------------------------------- */
    /*      Otherwise, consider creating two subnodes if could fit into     */
    /*      them, and adding to the appropriate subnode.                    */
    /* -------------------------------------------------------------------- */
#if MAX_SUBNODE == 2
    else if( maxdepth > 1 && node->numsubnodes == 0 ) {
      rectObj half1, half2;
      
      treeSplitBounds(&node->rect, &half1, &half2);

      if( msRectContained(&rect, &half1)) {
	node->numsubnodes = 2;
	node->subnode[0] = treeNodeCreate(half1);
	node->subnode[1] = treeNodeCreate(half2);
	
	return(treeNodeAddShapeId(node->subnode[0], id, rect, maxdepth-1));
      } else 
	if(msRectContained(&rect, &half2)) {
            node->numsubnodes = 2;
            node->subnode[0] = treeNodeCreate(&half1);
            node->subnode[1] = treeNodeCreate(&half2);

            return(treeNodeAddShapeId(node->subnode[1], id, rect, maxdepth-1));
        }
    }
#endif /* MAX_SUBNODE == 2 */

/* -------------------------------------------------------------------- */
/*      If none of that worked, just add it to this nodes list.         */
/* -------------------------------------------------------------------- */
    node->numshapes++;

    node->ids = SfRealloc( node->ids, sizeof(int) * node->numshapes );
    node->ids[node->numshapes-1] = id;

    return MS_TRUE;
}

static int treeAddShapeId(treeObj *tree, int id, rectObj rect)
{
  return(treeNodeAddShapeId(tree->root, id, rect, tree->maxdepth));
}

static void treeCollectShapeIds(treeNodeObj *node, rectObj aoi, char *status)
{
  int i;
    
  /* -------------------------------------------------------------------- */
  /*      Does this node overlap the area of interest at all?  If not,    */
  /*      return without adding to the list at all.                       */
  /* -------------------------------------------------------------------- */
  if(!msRectOverlap(&node->rect, &aoi))
    return;

  /* -------------------------------------------------------------------- */
  /*      Add the local nodes shapeids to the list.                       */
  /* -------------------------------------------------------------------- */
  for(i=0; i<node->numshapes; i++)
    msSetBit(status, node->ids[i], 1);
  
  /* -------------------------------------------------------------------- */
  /*      Recurse to subnodes if they exist.                              */
  /* -------------------------------------------------------------------- */
  for(i=0; i<node->numsubnodes; i++) {
    if(node->subnode[i])
      treeCollectShapeIds(node->subnode[i], aoi, status);
  }
}

char *msSearchTree(treeObj *tree, rectObj aoi)
{
  char *status=NULL;

  status = msAllocBitArray(tree->numshapes);
  if(!status) {
    msSetError(MS_MEMERR, NULL, "msSearchTree()");
    return(NULL);
  }
  
  treeCollectShapeIds(tree->root, aoi, status);

  return(status);
}

static int treeNodeTrim( treeNodeObj *node )
{
    int	i;

    /* -------------------------------------------------------------------- */
    /*      Trim subtrees, and free subnodes that come back empty.          */
    /* -------------------------------------------------------------------- */
    for(i=0; i<node->numsubnodes; i++ ) {
      if(treeNodeTrim(node->subnode[i])) {
	destroyTreeNode(node->subnode[i]);
	node->subnode[i] = node->subnode[node->numsubnodes-1];
	node->numsubnodes--;
	i--; /* process the new occupant of this subnode entry */
      }
    }

    /* -------------------------------------------------------------------- */
    /*      We should be trimmed if we have no subnodes, and no shapes.     */
    /* -------------------------------------------------------------------- */
    return(node->numsubnodes == 0 && node->numshapes == 0);
}

void msTreeTrim(treeObj *tree)
{
  treeNodeTrim(tree->root);
}

static void searchDiskTreeNode(FILE *stream, rectObj aoi, char *status) 
{
  int i;
  long offset;
  int numshapes, numsubnodes;
  rectObj rect;

  int *ids=NULL;

  fread(&offset, sizeof(long), 1, stream);
  fread(&rect, sizeof(rectObj), 1, stream);
  fread(&numshapes, sizeof(int), 1, stream);

  if(!msRectOverlap(&rect, &aoi)) { // skip rest of this node and sub-nodes
    offset += numshapes*sizeof(int) + sizeof(int);
    fseek(stream, offset, SEEK_CUR);
    return;
  }

  if(numshapes > 0) {
    ids = (int *)malloc(numshapes*sizeof(int));
    
    fread(ids, numshapes*sizeof(int), 1, stream);

    for(i=0; i<numshapes; i++)
      msSetBit(status, ids[i], 1);

    free(ids);
  }

  fread(&numsubnodes, sizeof(int), 1, stream);

  for(i=0; i<numsubnodes; i++)
    searchDiskTreeNode(stream, aoi, status);

  return;
}

char *msSearchDiskTree(char *filename, rectObj aoi)
{
  FILE *stream;
  char *status=NULL;
  int numshapes, maxdepth;

  stream = fopen(filename, "rb");
  if(!stream) {
    msSetError(MS_IOERR, NULL, "msSearchDiskTree()");
    return(NULL);
  }

  // read the header
  fread(&numshapes, sizeof(int), 1, stream);
  fread(&maxdepth, sizeof(int), 1, stream);

  status = msAllocBitArray(numshapes);
  if(!status) {
    msSetError(MS_MEMERR, NULL, "msSearchDiskTree()");
    fclose(stream);
    return(NULL);
  }

  searchDiskTreeNode(stream, aoi, status);

  fclose(stream);
  return(status);
}

static treeNodeObj *readTreeNode(FILE *stream) 
{
  int i;
  long offset;
  treeNodeObj *node;

  node = (treeNodeObj *) malloc(sizeof(treeNodeObj));
  node->ids = NULL;

  fread(&offset, sizeof(long), 1, stream);

  fread(&node->rect, sizeof(rectObj), 1, stream);

  fread(&node->numshapes, sizeof(int), 1, stream);
  if(node->numshapes>0)
    node->ids = (int *)malloc(sizeof(int)*node->numshapes);
  fread(node->ids, node->numshapes*sizeof(int), 1, stream);

  fread(&node->numsubnodes, sizeof(int), 1, stream);

  for(i=0; i<node->numsubnodes; i++ )
    node->subnode[i] = readTreeNode(stream);

  return node;
}

treeObj *msReadTree(char *filename)
{
  treeObj *tree=NULL;
  FILE *stream;

  stream = fopen(filename, "rb");
  if(!stream) {
    msSetError(MS_IOERR, NULL, "msReadTree()");
    return(NULL);
  }

  tree = (treeObj *) malloc(sizeof(treeObj));
  if(!tree) {
    msSetError(MS_MEMERR, NULL, "msReadTree()");
    return(NULL);
  }
  
  // read the header
  fread(&tree->numshapes, sizeof(int), 1, stream);
  fread(&tree->maxdepth, sizeof(int), 1, stream);

  tree->root = readTreeNode(stream);

  return(tree);
}

static long getSubNodeOffset(treeNodeObj *node) 
{
  int i;
  long offset=0;

  for(i=0; i<node->numsubnodes; i++ ) {
    if(node->subnode[i]) {
      offset += sizeof(long) + sizeof(rectObj) + (node->subnode[i]->numshapes+2)*sizeof(int);
      offset += getSubNodeOffset(node->subnode[i]);
    }
  }

  return(offset);
}

static void writeTreeNode(FILE *stream, treeNodeObj *node) 
{
  int i;
  long offset;

  offset = getSubNodeOffset(node);
  fwrite(&offset, sizeof(long), 1, stream);
  fwrite(&node->rect, sizeof(rectObj), 1, stream);
  fwrite(&node->numshapes, sizeof(int), 1, stream);
  if(node->numshapes>0)
    fwrite(node->ids,  node->numshapes*sizeof(int), 1, stream);
  fwrite(&node->numsubnodes, sizeof(int), 1, stream);

  for(i=0; i<node->numsubnodes; i++ ) {
    if(node->subnode[i])
      writeTreeNode(stream, node->subnode[i]);
  }

  return;
}

int msWriteTree(treeObj *tree, char *filename)
{
  FILE *stream;

  if((stream = fopen(filename, "wb")) == NULL) {
    msSetError(MS_IOERR, NULL, "msWriteTree()");
    return(MS_FALSE);
  }

  // for efficiency, trim the tree
  msTreeTrim(tree);

  // write the header
  fwrite(&tree->numshapes, sizeof(int), 1, stream);
  fwrite(&tree->maxdepth, sizeof(int), 1, stream);

  writeTreeNode(stream, tree->root);

  return(MS_TRUE);
}
