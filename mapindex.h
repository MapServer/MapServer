#ifndef MAPINDEX_H
#define MAPINDEX_H

/*
** Structure definitions
*/

struct nodeObj {
  int id;
  rectObj rect;
  struct nodeObj *next;
};

typedef struct {
  int nNodes;
  struct nodeObj *list;
  rectObj rect;
  int offset;
} cellObj;

typedef struct {
  FILE *fp;
  rectObj rect;
  int nCells;
  cellObj *cells; /* array of cells */
  int nRects;
} indexObj;

/*
** Function prototypes
*/

indexObj *msNewIndex(rectObj rect, int sx, int sy, int n);
indexObj *msReadIndex(char *filename);
void msWriteIndex(indexObj *idx, char *filename);
void msCloseIndex(indexObj *idx);
int msInsertRect(indexObj *idx, int id, rectObj rect);
char *msSearchIndex(indexObj *idx, rectObj rect);

#endif /* MAPINDEX_H */
