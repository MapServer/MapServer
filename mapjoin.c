#include "map.h"

// wrapper function for DB specific join functions
int msJoinConnect(layerObj *layer, joinObj *join) 
{
  switch(join->connectiontype) {
  case(MS_DB_XBASE):
    return msDBFJoinConnect(layer, join);
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
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.", "msJoinClose()");
  return MS_FAILURE;
}

//
// XBASE join functions
//
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

  if(join->joininfo) return(MS_SUCCESS); // already open
    
  // allocate a msDBFJoinInfo struct
  joininfo = (msDBFJoinInfo *) malloc(sizeof(msDBFJoinInfo));
  if(!joininfo) {
    msSetError(MS_MEMERR, "Error allocating XBase table info structure.", "msDBFJoinConnect()");
    return(MS_FAILURE);
  }

  // initialize any members that won't get set later on in this function
  joininfo->target = NULL;
  joininfo->nextrecord = 0;

  join->joininfo = joininfo;

  // open the XBase file
  if((joininfo->hDBF = msDBFOpen( msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, join->table), "rb" )) == NULL) {
    if((joininfo->hDBF = msDBFOpen( msBuildPath(szPath, layer->map->mappath, join->table), "rb" )) == NULL) {     
      msSetError(MS_IOERR, "(%s)", "msDBFJoinConnect()", join->table);   
      return(MS_FAILURE);
    }
  }

  // get "to" item index
  if((joininfo->toindex = msDBFGetItemIndex(joininfo->hDBF, join->to)) == -1) { 
    msSetError(MS_DBFERR, "Item %s not found in table %s.", "msDBFJoinConnect()", join->to, join->table); 
    return(MS_FAILURE);
  }

  // get "from" item index  
  for(i=0; i<layer->numitems; i++) {
    if(strcasecmp(layer->items[i],join->from) == 0) { // found it
      joininfo->fromindex = i;
      break;
    }
  }

  if(i == layer->numitems) {
    msSetError(MS_JOINERR, "Item %s not found in layer %s.", "msDBFJoinConnect()", join->from, layer->name); 
    return(MS_FAILURE);
  }

  // finally store away the item names in the XBase table
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

  joininfo->nextrecord = 0; // starting with the first record

  if(joininfo->target) free(joininfo->target); // clear last target
  joininfo->target = strdup(shape->values[joininfo->fromindex]);

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

  // clear any old data
  msFreeCharArray(join->values, join->numitems);
  join->values = NULL;

  n = msDBFGetRecordCount(joininfo->hDBF);
    
  for(i=joininfo->nextrecord; i<n; i++) { // find a match
    if(strcmp(joininfo->target, msDBFReadStringAttribute(joininfo->hDBF, i, joininfo->toindex)) == 0) break;
  }  
    
  if(i == n) { // unable to do the join
    if((join->values = (char **)malloc(sizeof(char *)*join->numitems)) == NULL) {
      msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
      return(MS_FAILURE);
    }
    for(i=0; i<join->numitems; i++)
      join->values[i] = strdup("\0"); /* intialize to zero length strings */

    joininfo->nextrecord = n;
    return(MS_DONE);
  }
    
  if((join->values = msDBFGetValues(joininfo->hDBF,i)) == NULL) 
    return(MS_FAILURE);

  joininfo->nextrecord = i+1; // so we know where to start looking next time through

  return(MS_SUCCESS);
}

int msDBFJoinClose(joinObj *join) 
{
  msDBFJoinInfo *joininfo = join->joininfo;

  if(!joininfo) return(MS_SUCCESS); // already closed

  msDBFClose(joininfo->hDBF);
  if(joininfo->target) free(joininfo->target);
  free(joininfo);
  joininfo = NULL;

  return(MS_SUCCESS);
}
