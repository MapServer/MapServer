#include "map.h"
#include "mapindex.h"

/*
** insertNode(): Insert to where l is pointing.
*/
int insertNode(struct nodeObj *list, int id, rectObj rect) 
{
  struct nodeObj *tmp;
  	
  if((tmp = (struct nodeObj *)malloc(sizeof(struct nodeObj))) == NULL) { /* error */
    msSetError(MS_MEMERR, NULL, "insertNode()");
    return(-1);
  }

  tmp->id = id;
  tmp->rect = rect;
  tmp->next = list->next;
  list->next = tmp;

  return(0); /* success */
}

/*
** initList(): Initializes a list.
*/
struct nodeObj *initList() 
{
  struct nodeObj *tmp;

  if((tmp = (struct nodeObj *)malloc(sizeof(struct nodeObj))) == NULL) { /* error */
    msSetError(MS_MEMERR, NULL, "initList()");
    return(NULL);
  }
  
  tmp->next = NULL;
  return(tmp);
}

/*
** freeList(): Unallocates a list.
*/
void freeList(struct nodeObj *list)
{
  if(list) {
	freeList(list->next); /* free any children */
	free(list);  
  }
  return;
}

/*
** msNewIndex(): Creates a new index.
*/
indexObj *msNewIndex(rectObj rect, int sx, int sy, int n)
{
  indexObj *idx;
  int i,j;
  double xi, yi;

  /* allocate space for the index, etc... */
  if((idx = (indexObj *)malloc(sizeof(indexObj))) == NULL) { /* error */
    msSetError(MS_MEMERR, NULL, "msNewIndex()");
    return(NULL);
  }

  idx->fp = NULL;
  idx->nCells = sx*sy;
  idx->nRects = n;
  idx->rect = rect;
  
  /* allocate the cells */
  if((idx->cells = (cellObj *)malloc(sizeof(cellObj)*(idx->nCells))) == NULL) { /* error */
    msSetError(MS_MEMERR, NULL, "msNewIndex()");
    return(NULL);
  }
 
  xi = (rect.maxx - rect.minx)/sx;
  yi = (rect.maxy - rect.miny)/sy;
  
  for(i=0;i<sy;i++) {
    for(j=0;j<sx;j++) {
      idx->cells[i*sx + j].rect.minx = j*xi + rect.minx;
      idx->cells[i*sx + j].rect.maxx = (j+1)*xi + rect.minx;
      idx->cells[i*sx + j].rect.miny = i*yi + rect.miny;
      idx->cells[i*sx + j].rect.maxy = (i+1)*yi + rect.miny;
      if((idx->cells[i*sx + j].list = initList()) == NULL)
	return(NULL);
      idx->cells[i*sx + j].nNodes = 0; 
    }
  }
  
  return(idx);
}

/*
** msInsertRect(): Adds entries to appropriate cells in the index.
*/
int msInsertRect(indexObj *idx, int id, rectObj rect)
{
  int i;
  struct nodeObj *tmp;

  for(i=0;i<idx->nCells;i++) { /* each cell */
    if(msRectOverlap(&(idx->cells[i].rect), &rect) == MS_TRUE) {
      tmp = idx->cells[i].list;
      idx->cells[i].nNodes++;
      while(tmp->next != NULL)
	tmp = tmp->next; /* skip to the end */
      if(insertNode(tmp, id, rect) == -1) /* error */
	return(-1);
    }
  }

  return(1);
}

/*
** msCloseIndex(): Cleans up an index.
*/
void msCloseIndex(indexObj *idx)
{
  int i;

  for(i=0;i<idx->nCells;i++) /* each cell */
    freeList(idx->cells[i].list);
  free(idx->cells);

  if(idx->fp)
    fclose(idx->fp);

  free(idx);

  return;
}

indexObj *msReadIndex(char *filename)
{
  indexObj *idx;
  int i;

  /* allocate space for the index, etc... */
  idx = (indexObj *)malloc(sizeof(indexObj));
  if(!idx) {
    msSetError(MS_MEMERR, NULL, "msReadIndex()");
    return(NULL);
  }

  idx->fp = fopen(filename, "rb");
  if(!idx->fp) {
    msSetError(MS_IOERR, NULL, "msReadIndex()");
    return(NULL);
  }

  /* read basic information */
  fread(&idx->rect, sizeof(rectObj), 1, idx->fp);
  fread(&idx->nCells, sizeof(int), 1, idx->fp);
  fread(&idx->nRects, sizeof(int), 1, idx->fp);

  /* allocate cell space */
  idx->cells = (cellObj *)malloc(sizeof(cellObj)*(idx->nCells));
  if(!idx->cells) {
    msSetError(MS_MEMERR, NULL, "msReadIndex()");
    return(NULL);
  }
 
  for(i=0;i<idx->nCells;i++) {
    fread(&idx->cells[i].rect, sizeof(rectObj), 1, idx->fp);  /* cell information */ 
    fread(&idx->cells[i].nNodes, sizeof(int), 1, idx->fp);
    fread(&idx->cells[i].offset, sizeof(int), 1, idx->fp);
    if((idx->cells[i].list = initList()) == NULL)
      return(NULL);
  }
  
  return(idx);
}

/*
** msWriteIndex(): Writes an index to a named file.
*/
void msWriteIndex(indexObj *idx, char *filename)
{
  FILE *stream;
  int i;
  struct nodeObj *tmp;
  int offset=0;

  if((stream = fopen(filename, "wb")) == NULL) {
    msSetError(MS_IOERR, NULL, "msReadIndex()");
    exit(0);
  }

  fwrite(&idx->rect, sizeof(rectObj), 1, stream); /* header information */
  fwrite(&idx->nCells, sizeof(int), 1, stream);
  fwrite(&idx->nRects, sizeof(int), 1, stream);

  offset = sizeof(rectObj) + 2*sizeof(int) + (idx->nCells)*(sizeof(rectObj) + 2*sizeof(int));

  for(i=0;i<idx->nCells;i++) { /* each cell */
    fwrite(&idx->cells[i].rect, sizeof(rectObj), 1, stream);  /* cell information */ 
    fwrite(&idx->cells[i].nNodes, sizeof(int), 1, stream);
    fwrite(&offset, sizeof(int), 1, stream);
    offset += idx->cells[i].nNodes*(sizeof(rectObj) + sizeof(int));
  }

  for(i=0;i<idx->nCells;i++) { /* each cell */
    tmp = idx->cells[i].list->next; /* first id is empty */
    while(tmp != NULL) {     
      fwrite(&tmp->id, sizeof(int), 1, stream); /* node information */
      fwrite(&tmp->rect, sizeof(rectObj), 1, stream);
      tmp = tmp->next;
    }
  }

  fclose(stream);
}

char *msSearchIndex(indexObj *idx, rectObj rect)
{
  char *status=NULL;
  int i,j;
  int id;
  rectObj search_rect;

  status = msAllocBitArray(idx->nRects);
  if(!status) {
    msSetError(MS_MEMERR, NULL, "msSearchIndex()");
    return(NULL);
  }

  for(i=0;i<idx->nCells;i++) { /* each cell */
    if(msRectOverlap(&(idx->cells[i].rect), &rect) == MS_TRUE) {
      fseek(idx->fp, idx->cells[i].offset, SEEK_SET);
      if(msRectContained(&(idx->cells[i].rect), &rect) == MS_TRUE) { /* all */
	for(j=0;j<idx->cells[i].nNodes;j++) {
	  fread(&id, sizeof(int), 1, idx->fp);
	  msSetBit(status, id, 1);
	  fseek(idx->fp, sizeof(rectObj), SEEK_CUR);
	}
      } else { /* need to test em */
	for(j=0;j<idx->cells[i].nNodes;j++) {
	  fread(&id, sizeof(int), 1, idx->fp);
	  if(msGetBit(status,id) == 1) { /* already been processed */
	    fseek(idx->fp, sizeof(rectObj), SEEK_CUR);
	  } else {
	    fread(&search_rect, sizeof(rectObj), 1, idx->fp);
	    if(msRectOverlap(&rect, &search_rect) == MS_TRUE)
	      msSetBit(status, id, 1);
	  }
	}
      }
    }
  }
  return(status);
}
