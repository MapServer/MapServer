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


typedef struct
{
    FILE        *fp;
    char        signature[3];
    char	LSB_order;
    char        needswap;
    char	version;
    char	flags[3];

    int         nShapes;
    int         nDepth;
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
