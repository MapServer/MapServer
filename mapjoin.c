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
} msDBFJoinTableInfo;

int msDBFJoinOpenTable(layerObj *layer, joinObj *join) {
  int i;
  char szPath[MS_MAXPATHLEN];
  msDBFJoinTableInfo *tableinfo;

  if(join->tableinfo) return(MS_SUCCESS); // already open
    
  // allocate a msDBFJoinTableInfo struct
  tableinfo = (msDBFJoinTableInfo *) malloc(sizeof(msDBFJoinTableInfo));
  if(!tableinfo) {
    msSetError(MS_MEMERR, "Error allocating XBase table info structure.", "msDBFJoinOpenTable()");
    return(MS_FAILURE);
  }

  join->tableinfo = tableinfo;

  // open the XBase file
  if((tableinfo->hDBF = msDBFOpen( msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, join->table), "rb" )) == NULL) {
    if((tableinfo->hDBF = msDBFOpen( msBuildPath(szPath, layer->map->mappath, join->table), "rb" )) == NULL) {     
      msSetError(MS_IOERR, "(%s)", "msDBFJoinOpenTable()", join->table);   
      return(MS_FAILURE);
    }
  }

  // get "to" item index
  if((tableinfo->toindex = msDBFGetItemIndex(tableinfo->hDBF, join->to)) == -1) { 
    msSetError(MS_DBFERR, "Item %s not found in table %s.", "msDBFJoinOpenTable()", join->to, join->table); 
    return(MS_FAILURE);
  }

  // get "from" item index  
  for(i=0; i<layer->numitems; i++) {
    if(strcasecmp(layer->items[i],join->from) == 0) { // found it
      tableinfo->fromindex = i;
      break;
    }
  }

  if(i == layer->numitems) {
    msSetError(MS_JOINERR, "Item %s not found in layer %s.", "msDBFJoinOpenTable()", join->from, layer->name); 
    return(MS_FAILURE);
  }

  // finally store away the item names in the XBase table
  join->numitems =  msDBFGetFieldCount(tableinfo->hDBF);
  join->items = msDBFGetItems(tableinfo->hDBF);
  if(!join->items) return(MS_FAILURE);  

  return(MS_SUCCESS);
}

int msDBFJoinCloseTable(joinObj *join) {
  msDBFJoinTableInfo *tableinfo = join->tableinfo;

  if(!tableinfo) return(MS_SUCCESS); // already closed
  msDBFClose(tableinfo->hDBF);
  free(tableinfo);
  tableinfo = NULL;
  return(MS_SUCCESS);
}

int msDBFJoinTable(layerObj *layer, joinObj *join, shapeObj *shape) {
  int i, j;
  int numrecords, *ids=NULL;

  msDBFJoinTableInfo *tableinfo;

  if(msDBFJoinOpenTable(layer, join) != MS_SUCCESS) return(MS_FAILURE);
  tableinfo = join->tableinfo;

  numrecords = msDBFGetRecordCount(tableinfo->hDBF);

  if(join->type == MS_JOIN_ONE_TO_ONE) { /* only one row */
    
    if((join->values = (char ***)malloc(sizeof(char **))) == NULL) {
      msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
      return(MS_FAILURE);
    }
    
    for(i=0; i<numrecords; i++) { /* find a match */
      if(strcmp(shape->values[tableinfo->fromindex], msDBFReadStringAttribute(tableinfo->hDBF, i, tableinfo->toindex)) == 0)
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
      if((join->values[0] = msDBFGetValues(tableinfo->hDBF,i)) == NULL) return(MS_FAILURE);      
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
      if(strcmp(shape->values[tableinfo->fromindex], msDBFReadStringAttribute(tableinfo->hDBF, i, tableinfo->toindex)) == 0) {
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

	join->values[i] = msDBFGetValues(tableinfo->hDBF,ids[i]);
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
