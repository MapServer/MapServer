#include "map.h"

// wrapper function for DB specific join functions
int msJoinTable(layerObj *layer, joinObj *join) {
  switch(join->connectiontype) {
  case(MS_DB_XBASE):
    return msDBFJoinTable(layer, join);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.", "msDBFJoinTable()");
  return MS_FAILURE;
}

// XBASE join function
int msDBFJoinTable(layerObj *layer, joinObj *join) {
  int i, j, idx;
  DBFHandle hDBF;
  int nrecs, *ids=NULL;

  char szPath[MS_MAXPATHLEN];

  /* first open the lookup table file, check shapepath and then mappath */
  if((hDBF = msDBFOpen( msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, join->table), "rb" )) == NULL) {
    if((hDBF = msDBFOpen( msBuildPath(szPath, layer->map->mappath, join->table), "rb" )) == NULL) {
      msSetError(MS_IOERR, "(%s)", "msDBFJoinTable()", join->table);   
      return(MS_FAILURE);
    }
  }

  if((idx = msDBFGetItemIndex(hDBF, join->to)) == -1) { 
    msSetError(MS_DBFERR, "Item %s not found.", "msDBFJoinTable()", join->to);
    msDBFClose(hDBF);
    return(MS_FAILURE);
  }

  /*
  ** Now we need to pull the record and item names in to the join buffers
  */
  join->numitems =  msDBFGetFieldCount(hDBF);
  if(!join->items) {
    join->items = msDBFGetItems(hDBF);
    if(!join->items) {
      return(MS_FAILURE);
    }
  }

  nrecs = msDBFGetRecordCount(hDBF);

  if(join->type == MS_JOIN_ONE_TO_ONE) { /* only one row */
    
    if((join->records = (char ***)malloc(sizeof(char **))) == NULL) {
      msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
      return(MS_FAILURE);
    }
    
    for(i=0; i<nrecs; i++) { /* find a match */
      if(strcmp(join->match, msDBFReadStringAttribute(hDBF, i, idx)) == 0)
	break;
    }  
    
    if(i == nrecs) { /* just return zero length strings */
      if((join->records[0] = (char **)malloc(sizeof(char *)*join->numitems)) == NULL) {
	msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
	return(MS_FAILURE);
      }
      for(i=0; i<join->numitems; i++)
	join->records[0][i] = strdup("\0"); /* intialize to zero length strings */
    } else {
      if((join->records[0] = msDBFGetValues(hDBF,i)) == NULL) return(MS_FAILURE);
    }

  } else {

    if(join->records) { /* free old records */
      for(i=0; i<join->numrecords; i++)
	msFreeCharArray(join->records[i], join->numitems);
      free(join->records);
      join->numrecords = 0;
    }

    ids = (int *)malloc(sizeof(int)*nrecs);
    if(!ids) {
      msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
      return(MS_FAILURE);
    }
    
    j=0;
    for(i=0; i<nrecs; i++) { /* find the matches, save ids */
      if(strcmp(join->match, msDBFReadStringAttribute(hDBF, i, idx)) == 0) {
	ids[j] = i;
	j++;
      }
    }
  
    join->numrecords = j;

    if(join->numrecords > 0) { /* save em */
      if((join->records = (char ***)malloc(sizeof(char **)*join->numrecords)) == NULL) {
	msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
	free(ids);
	return(MS_FAILURE);
      }

      for(i=0; i<join->numrecords; i++) {
	join->records[i] = (char **)malloc(sizeof(char *)*join->numitems);
	if(!join->records[i]) {
	  msSetError(MS_MEMERR, NULL, "msDBFJoinTable()");
	  free(ids);
	  return(MS_FAILURE);
	}

	join->records[i] = msDBFGetValues(hDBF,ids[i]);
	if(!join->records[i]) {
	  free(ids);
	  return(MS_FAILURE);
	}
      }
    }

    free(ids);
  }

  msDBFClose(hDBF);
  return(MS_SUCCESS);
}
