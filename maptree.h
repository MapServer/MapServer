#ifndef MAPTREE_H
#define MAPTREE_H

#ifdef __cplusplus
extern "C" {
#endif

/* this can be 2 or 4 for binary or quad tree */
#define MAX_SUBNODES 4

typedef struct shape_tree_node {
  // area covered by this node
  rectObj rect;
  
  // list of shapes stored at this node.
  int numshapes;
  int *ids;
  
  int numsubnodes;
  struct shape_tree_node *subnode[MAX_SUBNODES];
} treeNodeObj;

typedef struct {
  int numshapes;
  int maxdepth;
  treeNodeObj *root;
} treeObj;


treeObj *msCreateTree(shapefileObj *shapefile, int maxdepth);
void msTreeTrim(treeObj *tree);
void msDestroyTree(treeObj *tree);

char *msSearchTree(treeObj *tree, rectObj aoi);
char *msSearchDiskTree(char *filename, rectObj aoi);

treeObj *msReadTree(char *filename);
int msWriteTree(treeObj *tree, char *filename);

#ifdef __cplusplus
}
#endif

#endif /* MAPTREE_H */
