#include "map.h"

// wrapper function for DB specific join functions
int msJoinTable(layerObj *layer, joinObj *join, shapeObj *shape) {
  switch(join->connectiontype) {
  case(MS_DB_XBASE):
    return msDBFJoinTable(layer, join, shape);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.", "msDBFJoinTable()");
  return MS_FAILURE;
}

int msJoinCloseTable(joinObj *join) {
  switch(join->connectiontype) {
  case(MS_DB_XBASE):
    return msDBFJoinCloseTable(join);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.", "msDBFJoinTable()");
  return MS_FAILURE;
}

// XBASE join functions
typedef struct {
  DBFHandle hDBF;
  int fromindex, toindex;
  int *indexes;
  int numindexes;
} msDBFJoinInfo;

int msDBFJoinOpenTable(layerObj *layer, joinObj *join) {
  int i;
  char szPath[MS_MAXPATHLEN];
  msDBFJoinInfo *joininfo;

  if(join->joininfo) return(MS_SUCCESS); // already open
    
  // allocate a msDBFJoinInfo struct
  joininfo = (msDBFJoinInfo *) malloc(sizeof(msDBFJoinInfo));
  if(!joininfo) {
    msSetError(MS_MEMERR, "Error allocating XBase table info structure.", "msDBFJoinOpenTable()");
    return(MS_FAILURE);
  }

  join->joininfo = joininfo;

  // open the XBase file
  if((joininfo->hDBF = msDBFOpen( msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, join->table), "rb" )) == NULL) {
    if((joininfo->hDBF = msDBFOpen( msBuildPath(szPath, layer->map->mappath, join->table), "rb" )) == NULL) {     
      msSetError(MS_IOERR, "(%s)", "msDBFJoinOpenTable()", join->table);   
      return(MS_FAILURE);
    }
  }

  // get "to" item index
  if((joininfo->toindex = msDBFGetItemIndex(joininfo->hDBF, join->to)) == -1) { 
    msSetError(MS_DBFERR, "Item %s not found in table %s.", "msDBFJoinOpenTable()", join->to, join->table); 
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
    msSetError(MS_JOINERR, "Item %s not found in layer %s.", "msDBFJoinOpenTable()", join->from, layer->name); 
    return(MS_FAILURE);
  }

  // finally store away the item names in the XBase table
  join->numitems =  msDBFGetFieldCount(joininfo->hDBF);
  join->items = msDBFGetItems(joininfo->hDBF);
  if(!join->items) return(MS_FAILURE);  

  return(MS_SUCCESS);
}

int msDBFJoinCloseTable(joinObj *join) {
  msDBFJoinInfo *joininfo = join->joininfo;

  if(!joininfo) return(MS_SUCCESS); // already closed
  msDBFClose(joininfo->hDBF);
  free(joininfo);
  joininfo = NULL;
  return(MS_SUCCESS);
}

int msDBFJoinTable(layerObj *layer, joinObj *join, shapeObj *shape) {
  int i, j;
  int numrecords, *ids=NULL;

  msDBFJoinInfo *joininfo;

  if(msDBFJoinOpenTable(layer, join) != MS_SUCCESS) return(MS_FAILURE);
  joininfo = join->joininfo;

  numrecords = msDBFGetRecordCount(joininfo->hDBF);

  if(join->type == MS_JOIN_ONE_TO_ONE) { /* only one row */
    
    if((join->values = (char ***)malloc(sizeof(char **))) == NULL) {
      msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
      return(MS_FAILURE);
    }
    
    for(i=0; i<numrecords; i++) { /* find a match */
      if(strcmp(shape->values[joininfo->fromindex], msDBFReadStringAttribute(joininfo->hDBF, i, joininfo->toindex)) == 0)
	break;
    }  
    
    if(i == numrecords) { /* just return zero length strings */
      if((join->values[0] = (char **)malloc(sizeof(char *)*join->numitems)) == NULL) {
	msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
	return(MS_FAILURE);
      }
      for(i=0; i<join->numitems; i++)
	join->values[0][i] = strdup("\0"); /* intialize to zero length strings */
    } else {
      if((join->values[0] = msDBFGetValues(joininfo->hDBF,i)) == NULL) return(MS_FAILURE);      
    }

  } else {

    if(join->values) { /* free old values */
      for(i=0; i<join->numrecords; i++)
	msFreeCharArray(join->values[i], join->numitems);
      free(join->values);
      join->numrecords = 0;
    }

    ids = (int *)malloc(sizeof(int)*numrecords);
    if(!ids) {
      msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
      return(MS_FAILURE);
    }
    
    j=0;
    for(i=0; i<numrecords; i++) { /* find the matches, save ids */
      if(strcmp(shape->values[joininfo->fromindex], msDBFReadStringAttribute(joininfo->hDBF, i, joininfo->toindex)) == 0) {
	ids[j] = i;
	j++;
      }
    }
  
    join->numrecords = j;

    if(join->numrecords > 0) { /* save em */
      if((join->values = (char ***)malloc(sizeof(char **)*join->numrecords)) == NULL) {
	msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
	free(ids);
	return(MS_FAILURE);
      }

      for(i=0; i<join->numrecords; i++) {
	join->values[i] = (char **)malloc(sizeof(char *)*join->numitems);
	if(!join->values[i]) {
	  msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
	  free(ids);
	  return(MS_FAILURE);
	}

	join->values[i] = msDBFGetValues(joininfo->hDBF,ids[i]);
	if(!join->values[i]) {
	  free(ids);
	  return(MS_FAILURE);
	}
      }
    }

    free(ids);
  }
  
  return(MS_SUCCESS);
}
