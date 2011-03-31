/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implements MapServer joins. 
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
 ****************************************************************************/

#include "mapserver.h"

MS_CVSID("$Id$")

#define ROW_ALLOCATION_SIZE 10


/* DBF/XBase function prototypes */
int msDBFJoinConnect(layerObj *layer, joinObj *join);
int msDBFJoinPrepare(joinObj *join, shapeObj *shape);
int msDBFJoinNext(joinObj *join);
int msDBFJoinClose(joinObj *join);
int msDBFJoinTable(layerObj *layer, joinObj *join, shapeObj *shape);

/* CSV (comma delimited text file) function prototypes */
int msCSVJoinConnect(layerObj *layer, joinObj *join);
int msCSVJoinPrepare(joinObj *join, shapeObj *shape);
int msCSVJoinNext(joinObj *join);
int msCSVJoinClose(joinObj *join);
int msCSVJoinTable(layerObj *layer, joinObj *join, shapeObj *shape);

/* PostgreSQL function prototypes */
int msPOSTGRESQLJoinConnect(layerObj *layer, joinObj *join);
int msPOSTGRESQLJoinPrepare(joinObj *join, shapeObj *shape);
int msPOSTGRESQLJoinNext(joinObj *join);
int msPOSTGRESQLJoinClose(joinObj *join);

/* wrapper function for DB specific join functions */
int msJoinConnect(layerObj *layer, joinObj *join) 
{
  switch(join->connectiontype) {
  case(MS_DB_XBASE):
    return msDBFJoinConnect(layer, join);
    break;
  case(MS_DB_CSV):
    return msCSVJoinConnect(layer, join);
    break;
  case(MS_DB_POSTGRES):
    return msPOSTGRESQLJoinConnect(layer, join);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.", "msJoinConnect()");
  return MS_FAILURE;
}

int msJoinPrepare(joinObj *join, shapeObj *shape) 
{
  switch(join->connectiontype) {
  case(MS_DB_XBASE):
    return msDBFJoinPrepare(join, shape);
    break;
  case(MS_DB_CSV):
    return msCSVJoinPrepare(join, shape);
    break;
  case(MS_DB_POSTGRES):
    return msPOSTGRESQLJoinPrepare(join, shape);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.", "msJoinPrepare()");
  return MS_FAILURE;
}

int msJoinNext(joinObj *join) 
{
  switch(join->connectiontype) {
  case(MS_DB_XBASE):
    return msDBFJoinNext(join);
    break;
  case(MS_DB_CSV):
    return msCSVJoinNext(join);
    break;
  case(MS_DB_POSTGRES):
    return msPOSTGRESQLJoinNext(join);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.", "msJoinNext()");
  return MS_FAILURE;
}

int msJoinClose(joinObj *join) 
{
  switch(join->connectiontype) {
  case(MS_DB_XBASE):
    return msDBFJoinClose(join);
    break;
  case(MS_DB_CSV):
    return msCSVJoinClose(join);
    break;
  case(MS_DB_POSTGRES):
    return msPOSTGRESQLJoinClose(join);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.", "msJoinClose()");
  return MS_FAILURE;
}

/*  */
/* XBASE join functions */
/*  */
typedef struct {
  DBFHandle hDBF;
  int fromindex, toindex;
  char *target;
  int nextrecord;
} msDBFJoinInfo;

int msDBFJoinConnect(layerObj *layer, joinObj *join) 
{
  int i;
  char szPath[MS_MAXPATHLEN];
  msDBFJoinInfo *joininfo;

  if(join->joininfo) return(MS_SUCCESS); /* already open */
  
  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
	return MS_FAILURE;
  
    
  /* allocate a msDBFJoinInfo struct */
  joininfo = (msDBFJoinInfo *) malloc(sizeof(msDBFJoinInfo));
  if(!joininfo) {
    msSetError(MS_MEMERR, "Error allocating XBase table info structure.", "msDBFJoinConnect()");
    return(MS_FAILURE);
  }

  /* initialize any members that won't get set later on in this function */
  joininfo->target = NULL;
  joininfo->nextrecord = 0;

  join->joininfo = joininfo;

  /* open the XBase file */
  if((joininfo->hDBF = msDBFOpen( msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, join->table), "rb" )) == NULL) {
    if((joininfo->hDBF = msDBFOpen( msBuildPath(szPath, layer->map->mappath, join->table), "rb" )) == NULL) {     
      msSetError(MS_IOERR, "(%s)", "msDBFJoinConnect()", join->table);   
      return(MS_FAILURE);
    }
  }

  /* get "to" item index */
  if((joininfo->toindex = msDBFGetItemIndex(joininfo->hDBF, join->to)) == -1) { 
    msSetError(MS_DBFERR, "Item %s not found in table %s.", "msDBFJoinConnect()", join->to, join->table); 
    return(MS_FAILURE);
  }

  /* get "from" item index   */
  for(i=0; i<layer->numitems; i++) {
    if(strcasecmp(layer->items[i],join->from) == 0) { /* found it */
      joininfo->fromindex = i;
      break;
    }
  }

  if(i == layer->numitems) {
    msSetError(MS_JOINERR, "Item %s not found in layer %s.", "msDBFJoinConnect()", join->from, layer->name); 
    return(MS_FAILURE);
  }

  /* finally store away the item names in the XBase table */
  join->numitems =  msDBFGetFieldCount(joininfo->hDBF);
  join->items = msDBFGetItems(joininfo->hDBF);
  if(!join->items) return(MS_FAILURE);  

  return(MS_SUCCESS);
}

int msDBFJoinPrepare(joinObj *join, shapeObj *shape) 
{
  msDBFJoinInfo *joininfo = join->joininfo;  

  if(!joininfo) {
    msSetError(MS_JOINERR, "Join connection has not be created.", "msDBFJoinPrepare()"); 
    return(MS_FAILURE);
  }

  if(!shape) {
    msSetError(MS_JOINERR, "Shape to be joined is empty.", "msDBFJoinPrepare()"); 
    return(MS_FAILURE);
  }

  if(!shape->values) {
    msSetError(MS_JOINERR, "Shape to be joined has no attributes.", "msDBFJoinPrepare()"); 
    return(MS_FAILURE);
  }

  joininfo->nextrecord = 0; /* starting with the first record */

  if(joininfo->target) free(joininfo->target); /* clear last target */
  joininfo->target = msStrdup(shape->values[joininfo->fromindex]);

  return(MS_SUCCESS);
}

int msDBFJoinNext(joinObj *join) 
{
  int i, n;
  msDBFJoinInfo *joininfo = join->joininfo;

  if(!joininfo) {
    msSetError(MS_JOINERR, "Join connection has not be created.", "msDBFJoinNext()"); 
    return(MS_FAILURE);
  }

  if(!joininfo->target) {
    msSetError(MS_JOINERR, "No target specified, run msDBFJoinPrepare() first.", "msDBFJoinNext()");
    return(MS_FAILURE);
  }

  /* clear any old data */
  if(join->values) { 
    msFreeCharArray(join->values, join->numitems);
    join->values = NULL;
  }

  n = msDBFGetRecordCount(joininfo->hDBF);
    
  for(i=joininfo->nextrecord; i<n; i++) { /* find a match */
    if(strcmp(joininfo->target, msDBFReadStringAttribute(joininfo->hDBF, i, joininfo->toindex)) == 0) break;
  }  
    
  if(i == n) { /* unable to do the join */
    if((join->values = (char **)malloc(sizeof(char *)*join->numitems)) == NULL) {
      msSetError(MS_MEMERR, NULL, "msDBFJoinNext()");
      return(MS_FAILURE);
    }
    for(i=0; i<join->numitems; i++)
      join->values[i] = msStrdup("\0"); /* intialize to zero length strings */

    joininfo->nextrecord = n;
    return(MS_DONE);
  }
    
  if((join->values = msDBFGetValues(joininfo->hDBF,i)) == NULL) 
    return(MS_FAILURE);

  joininfo->nextrecord = i+1; /* so we know where to start looking next time through */

  return(MS_SUCCESS);
}

int msDBFJoinClose(joinObj *join) 
{
  msDBFJoinInfo *joininfo = join->joininfo;

  if(!joininfo) return(MS_SUCCESS); /* already closed */

  if(joininfo->hDBF) msDBFClose(joininfo->hDBF);
  if(joininfo->target) free(joininfo->target);
  free(joininfo);
  joininfo = NULL;

  return(MS_SUCCESS);
}

/*  */
/* CSV (comma separated value) join functions */
/*  */
typedef struct {
  int fromindex, toindex;
  char *target;
  char ***rows;
  int numrows;
  int nextrow;
} msCSVJoinInfo;

int msCSVJoinConnect(layerObj *layer, joinObj *join) 
{
  int i;
  FILE *stream;
  char szPath[MS_MAXPATHLEN];
  msCSVJoinInfo *joininfo;
  char buffer[MS_BUFFER_LENGTH];

  if(join->joininfo) return(MS_SUCCESS); /* already open */
  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
	return MS_FAILURE;
  
    
  /* allocate a msCSVJoinInfo struct */
  if((joininfo = (msCSVJoinInfo *) malloc(sizeof(msCSVJoinInfo))) == NULL) {
    msSetError(MS_MEMERR, "Error allocating CSV table info structure.", "msCSVJoinConnect()");
    return(MS_FAILURE);
  }

  /* initialize any members that won't get set later on in this function */
  joininfo->target = NULL;
  joininfo->nextrow = 0;

  join->joininfo = joininfo;

  /* open the CSV file */
  if((stream = fopen( msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, join->table), "r" )) == NULL) {
    if((stream = fopen( msBuildPath(szPath, layer->map->mappath, join->table), "r" )) == NULL) {     
      msSetError(MS_IOERR, "(%s)", "msCSVJoinConnect()", join->table);   
      return(MS_FAILURE);
    }
  }
  
  /* once through to get the number of rows */
  joininfo->numrows = 0;
  while(fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) joininfo->numrows++;
  rewind(stream);

  if((joininfo->rows = (char ***) malloc(joininfo->numrows*sizeof(char **))) == NULL) {
    msSetError(MS_MEMERR, "Error allocating rows.", "msCSVJoinConnect()");
    return(MS_FAILURE);
  }

  /* load the rows */
  i = 0;
  while(fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) {  
    msStringTrimEOL(buffer);  
    joininfo->rows[i] = msStringSplitComplex(buffer, ",", &(join->numitems), MS_ALLOWEMPTYTOKENS);
    i++;
  }
  fclose(stream);

  /* get "from" item index   */
  for(i=0; i<layer->numitems; i++) {
    if(strcasecmp(layer->items[i],join->from) == 0) { /* found it */
      joininfo->fromindex = i;
      break;
    }
  }

  if(i == layer->numitems) {
    msSetError(MS_JOINERR, "Item %s not found in layer %s.", "msCSVJoinConnect()", join->from, layer->name); 
    return(MS_FAILURE);
  }

  /* get "to" index (for now the user tells us which column, 1..n) */
  joininfo->toindex = atoi(join->to) - 1;
  if(joininfo->toindex < 0 || joininfo->toindex > join->numitems) {
    msSetError(MS_JOINERR, "Invalid column index %s.", "msCSVJoinConnect()", join->to); 
    return(MS_FAILURE);
  }

  /* store away the column names (1..n) */
  if((join->items = (char **) malloc(sizeof(char *)*join->numitems)) == NULL) {
    msSetError(MS_MEMERR, "Error allocating space for join item names.", "msCSVJoinConnect()");
    return(MS_FAILURE);
  }
  for(i=0; i<join->numitems; i++) {
    join->items[i] = (char *) malloc(8); /* plenty of space */
    sprintf(join->items[i], "%d", i+1);
  }

  return(MS_SUCCESS);
}

int msCSVJoinPrepare(joinObj *join, shapeObj *shape) 
{
  msCSVJoinInfo *joininfo = join->joininfo;

  if(!joininfo) {
    msSetError(MS_JOINERR, "Join connection has not be created.", "msCSVJoinPrepare()"); 
    return(MS_FAILURE);
  }

  if(!shape) {
    msSetError(MS_JOINERR, "Shape to be joined is empty.", "msCSVJoinPrepare()"); 
    return(MS_FAILURE);
  }

  if(!shape->values) {
    msSetError(MS_JOINERR, "Shape to be joined has no attributes.", "msCSVJoinPrepare()"); 
    return(MS_FAILURE);
  }

  joininfo->nextrow = 0; /* starting with the first record */

  if(joininfo->target) free(joininfo->target); /* clear last target */
  joininfo->target = msStrdup(shape->values[joininfo->fromindex]);

  return(MS_SUCCESS);
}

int msCSVJoinNext(joinObj *join) 
{
  int i,j;
  msCSVJoinInfo *joininfo = join->joininfo;

  if(!joininfo) {
    msSetError(MS_JOINERR, "Join connection has not be created.", "msCSVJoinNext()"); 
    return(MS_FAILURE);
  }

  /* clear any old data */
  if(join->values) { 
    msFreeCharArray(join->values, join->numitems);
    join->values = NULL;
  }

  for(i=joininfo->nextrow; i<joininfo->numrows; i++) { /* find a match     */
    if(strcmp(joininfo->target, joininfo->rows[i][joininfo->toindex]) == 0) break;
  }  

  if((join->values = (char ** )malloc(sizeof(char *)*join->numitems)) == NULL) {
    msSetError(MS_MEMERR, NULL, "msCSVJoinNext()");
    return(MS_FAILURE);
  }
  
  if(i == joininfo->numrows) { /* unable to do the join     */
    for(j=0; j<join->numitems; j++)
      join->values[j] = msStrdup("\0"); /* intialize to zero length strings */

    joininfo->nextrow = joininfo->numrows;
    return(MS_DONE);
  } 

  for(j=0; j<join->numitems; j++)
    join->values[j] = msStrdup(joininfo->rows[i][j]);

  joininfo->nextrow = i+1; /* so we know where to start looking next time through */

  return(MS_SUCCESS);
}

int msCSVJoinClose(joinObj *join) 
{ 
  int i;
  msCSVJoinInfo *joininfo = join->joininfo;

  if(!joininfo) return(MS_SUCCESS); /* already closed */

  for(i=0; i<joininfo->numrows; i++)
    msFreeCharArray(joininfo->rows[i], join->numitems);
  free(joininfo->rows);
  if(joininfo->target) free(joininfo->target);
  free(joininfo);
  joininfo = NULL;

  return(MS_SUCCESS);
}

