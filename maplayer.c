/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of most layerObj functions.
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
#include "maptime.h"
#include "mapogcfilter.h"

#include <assert.h>
MS_CVSID("$Id$")


static int populateVirtualTable(layerVTableObj *vtable);


/*
** Iteminfo is a layer parameter that holds information necessary to retrieve an individual item for
** a particular source. It is an array built from a list of items. The type of iteminfo will vary by
** source. For shapefiles and OGR it is simply an array of integers where each value is an index for
** the item. For SDE it's a ESRI specific type that contains index and column type information. Two
** helper functions below initialize and free that structure member which is used locally by layer
** specific functions.
*/
int msLayerInitItemInfo(layerObj *layer) 
{
    if ( ! layer->vtable) {
        int rv =  msInitializeVirtualTable(layer);
        if (rv != MS_SUCCESS)
            return rv;
    }
    return layer->vtable->LayerInitItemInfo(layer);
}

void msLayerFreeItemInfo(layerObj *layer) 
{
  if ( ! layer->vtable) {
      int rv =  msInitializeVirtualTable(layer);
      if (rv != MS_SUCCESS)
          return;
  }
  layer->vtable->LayerFreeItemInfo(layer);
}

/*
** Does exactly what it implies, readies a layer for processing.
*/
int msLayerOpen(layerObj *layer)
{
  if(layer->features && layer->connectiontype != MS_GRATICULE ) 
    layer->connectiontype = MS_INLINE;

  if(layer->tileindex && layer->connectiontype == MS_SHAPEFILE)
    layer->connectiontype = MS_TILED_SHAPEFILE;

  if(layer->type == MS_LAYER_RASTER && layer->connectiontype != MS_WMS)
    layer->connectiontype = MS_RASTER;

  if ( ! layer->vtable) {
      int rv =  msInitializeVirtualTable(layer);
      if (rv != MS_SUCCESS)
          return rv;
  }
  return layer->vtable->LayerOpen(layer);
}

/*
** Returns MS_TRUE if layer has been opened using msLayerOpen(), MS_FALSE otherwise
*/
int msLayerIsOpen(layerObj *layer)
{
  if ( ! layer->vtable) {
      int rv =  msInitializeVirtualTable(layer);
      if (rv != MS_SUCCESS)
          return rv;
  }
  return layer->vtable->LayerIsOpen(layer);
}

/*
** Performs a spatial, and optionally an attribute based feature search. The function basically
** prepares things so that candidate features can be accessed by query or drawing functions. For
** OGR and shapefiles this sets an internal bit vector that indicates whether a particular feature
** is to processed. For SDE it executes an SQL statement on the SDE server. Once run the msLayerNextShape
** function should be called to actually access the shapes.
**
** Note that for shapefiles we apply any maxfeatures constraint at this point. That may be the only
** connection type where this is feasible.
*/
int msLayerWhichShapes(layerObj *layer, rectObj rect)
{
  if ( ! layer->vtable) {
      int rv =  msInitializeVirtualTable(layer);
      if (rv != MS_SUCCESS)
          return rv;
  }
  return layer->vtable->LayerWhichShapes(layer, rect);
}

/*
** Called after msWhichShapes has been called to actually retrieve shapes within a given area
** and matching a vendor specific filter (i.e. layer FILTER attribute).
**
** Shapefiles: NULL shapes (shapes with attributes but NO vertices are skipped)
*/
int msLayerNextShape(layerObj *layer, shapeObj *shape) 
{
  if ( ! layer->vtable) {
      int rv =  msInitializeVirtualTable(layer);
      if (rv != MS_SUCCESS)
          return rv;
  }

  /* At the end of switch case (default -> break; -> return MS_FAILURE), 
   * was following TODO ITEM:
   */
  /* TO DO! This is where dynamic joins will happen. Joined attributes will be */
  /* tagged on to the main attributes with the naming scheme [join name].[item name]. */
  /* We need to leverage the iteminfo (I think) at this point */

  return layer->vtable->LayerNextShape(layer, shape);
}

/*
** Used to retrieve a shape from a result set by index. Result sets are created by the various
** msQueryBy...() functions. The index is assigned by the data source.
*/
int msLayerResultsGetShape(layerObj *layer, shapeObj *shape, int tile, long record)
{
  if ( ! layer->vtable) {
    int rv =  msInitializeVirtualTable(layer);
    if (rv != MS_SUCCESS)
      return rv;
  }

  return layer->vtable->LayerResultsGetShape(layer, shape, tile, record);
}

/*
** Used to retrieve a shape by index. All data sources must be capable of random access using
** a record number of some sort.
*/
int msLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record)
{
  if ( ! layer->vtable) {
      int rv =  msInitializeVirtualTable(layer);
      if (rv != MS_SUCCESS)
          return rv;
  }

  /* At the end of switch case (default -> break; -> return MS_FAILURE), 
   * was following TODO ITEM:
   */
  /* TO DO! This is where dynamic joins will happen. Joined attributes will be */
  /* tagged on to the main attributes with the naming scheme [join name].[item name]. */

  return layer->vtable->LayerGetShape(layer, shape, tile, record);
}

/*
** Closes resources used by a particular layer.
*/
void msLayerClose(layerObj *layer) 
{
  int i;

  /* no need for items once the layer is closed */
  msLayerFreeItemInfo(layer);
  if(layer->items) {
    msFreeCharArray(layer->items, layer->numitems);
    layer->items = NULL;
    layer->numitems = 0;
  }

  /* clear out items used as part of expressions (bug #2702) */
  for(i=0; i<layer->numclasses; i++) {    
    msFreeCharArray(layer->class[i]->expression.items, layer->class[i]->expression.numitems);
    msFree(layer->class[i]->expression.indexes);
    layer->class[i]->expression.items = NULL;
    layer->class[i]->expression.indexes = NULL;
    layer->class[i]->expression.numitems = 0;        
  }

  if (layer->vtable) {
    layer->vtable->LayerClose(layer);
  }
}

/*
** Retrieves a list of attributes available for this layer. Most sources also set the iteminfo array
** at this point. This function is used when processing query results to expose attributes to query
** templates. At that point all attributes are fair game.
*/
int msLayerGetItems(layerObj *layer) 
{
  const char *itemNames;
  /* clean up any previously allocated instances */
  msLayerFreeItemInfo(layer);
  if(layer->items) {
    msFreeCharArray(layer->items, layer->numitems);
    layer->items = NULL;
    layer->numitems = 0;
  }

  if ( ! layer->vtable) {
      int rv =  msInitializeVirtualTable(layer);
      if (rv != MS_SUCCESS)
          return rv;
  }

  /* At the end of switch case (default -> break; -> return MS_FAILURE), 
   * was following TODO ITEM:
   */
  /* TO DO! Need to add any joined itemd on to the core layer items, one long list!  */
  itemNames = msLayerGetProcessingKey( layer, "ITEMS" );
  if (itemNames)
  {
    layer->items = msStringSplit(itemNames, ',', &layer->numitems);
    return MS_SUCCESS;
  }
  else
    return layer->vtable->LayerGetItems(layer);
}

/*
** Returns extent of spatial coverage for a layer.
**
** If layer->extent is set then this value is used, otherwise the 
** driver-specific implementation is called (this can be expensive).
**
** If layer is not already opened then it is opened and closed (so this
** function can be called on both opened or closed layers).
**
** Returns MS_SUCCESS/MS_FAILURE.
*/
int msLayerGetExtent(layerObj *layer, rectObj *extent) 
{
  int need_to_close = MS_FALSE, status = MS_SUCCESS;

  if (MS_VALID_EXTENT(layer->extent))
  {
      *extent = layer->extent;
      return MS_SUCCESS;
  }

  if (!msLayerIsOpen(layer))
  {
      if (msLayerOpen(layer) != MS_SUCCESS)
          return MS_FAILURE;
      need_to_close = MS_TRUE;
  }

  if ( ! layer->vtable) {
      int rv =  msInitializeVirtualTable(layer);
      if (rv != MS_SUCCESS) {
        if (need_to_close)
            msLayerClose(layer);
            return rv;
      }
  }
  status = layer->vtable->LayerGetExtent(layer, extent);

  if (need_to_close)
      msLayerClose(layer);

  return(status);
}

static int string2list(char **list, int *listsize, char *string)
{
  int i;

  for(i=0; i<(*listsize); i++)
    if(strcasecmp(list[i], string) == 0) {
      /* printf("string2list (duplicate): %s %d\n", string, i); */
      return(i);
    }

  list[i] = strdup(string);
  (*listsize)++;

  /* printf("string2list: %s %d\n", string, i); */

  return(i);
}

/* TO DO: this function really needs to use the lexer */
static void expression2list(char **list, int *listsize, expressionObj *expression)
{
  int i, j, l;
  char tmpstr1[1024], tmpstr2[1024];
  short in=MS_FALSE;
  int tmpint;

  j = 0;
  l = strlen(expression->string);
  for(i=0; i<l; i++) {
    if(expression->string[i] == '[') {
      in = MS_TRUE;
      tmpstr2[j] = expression->string[i];
      j++;
      continue;
    }
    if(expression->string[i] == ']') {
      in = MS_FALSE;

      tmpint = expression->numitems;

      tmpstr2[j] = expression->string[i];
      tmpstr2[j+1] = '\0';
      string2list(expression->items, &(expression->numitems), tmpstr2);

      if(tmpint != expression->numitems) { /* not a duplicate, so no need to calculate the index */
        tmpstr1[j-1] = '\0';
        expression->indexes[expression->numitems - 1] = string2list(list, listsize, tmpstr1);
      }

      j = 0; /* reset */

      continue;
    }

    if(in) {
      tmpstr2[j] = expression->string[i];
      tmpstr1[j-1] = expression->string[i];
      j++;
    }
  }
}

/*
** This function builds a list of items necessary to draw or query a particular layer by
** examining the contents of the various xxxxitem parameters and expressions. That list is
** then used to set the iteminfo variable.
*/
int msLayerWhichItems(layerObj *layer, int get_all, char *metadata)
{
  int i, j, k, rv;
  int nt=0, ne=0;

  if (!layer->vtable) {
    rv =  msInitializeVirtualTable(layer);
    if (rv != MS_SUCCESS) return rv;
  }

   /* force get_all=MS_TRUE in some cases */
  if(layer->connectiontype == MS_INLINE || layer->connectiontype == MS_SDE ||
     (layer->connectiontype == MS_ORACLESPATIAL && layer->data && msCaseFindSubstring(layer->data, "UNIQUE")))
      get_all=MS_TRUE;

  /* cleanup any previous item selection */
  msLayerFreeItemInfo(layer);
  if(layer->items) {
    msFreeCharArray(layer->items, layer->numitems);
    layer->items = NULL;
    layer->numitems = 0;
  }

  /* layer level counts */
  if(layer->classitem) nt++;
  if(layer->filteritem) nt++;

  ne = 0;
  if(layer->filter.type == MS_EXPRESSION) {
    ne = msCountChars(layer->filter.string, '[');
    if(ne > 0) {
      layer->filter.items = (char **) calloc(ne, sizeof(char *)); /* should be more than enough space */
      if(!(layer->filter.items)) {
        msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
        return(MS_FAILURE);
      }
      layer->filter.indexes = (int *) malloc(ne*sizeof(int));
      if(!(layer->filter.indexes)) {
        msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
        return(MS_FAILURE);
      }
      layer->filter.numitems = 0;
      nt += ne;
    }
  }

  if(layer->labelitem) nt++;

  /* class level counts */
  for(i=0; i<layer->numclasses; i++) {

    for(j=0; j<layer->class[i]->numstyles; j++) {
      if(layer->class[i]->styles[j]->rangeitem) nt++;
      nt += layer->class[i]->styles[j]->numbindings;
    }

    ne = 0;
    if(layer->class[i]->expression.type == MS_EXPRESSION) {
      ne = msCountChars(layer->class[i]->expression.string, '[');
      if(ne > 0) {
        layer->class[i]->expression.items = (char **) calloc(ne, sizeof(char *)); /* should be more than enough space */
        if(!(layer->class[i]->expression.items)) {
          msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
          return(MS_FAILURE);
        }
        layer->class[i]->expression.indexes = (int *) malloc(ne*sizeof(int));
        if(!(layer->class[i]->expression.indexes)) {
          msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
          return(MS_FAILURE);
        }
        layer->class[i]->expression.numitems = 0;
        nt += ne;
      }
    }

    nt += layer->class[i]->label.numbindings;

    ne = 0;
    if(layer->class[i]->text.type == MS_EXPRESSION) {
      ne = msCountChars(layer->class[i]->text.string, '[');
      if(ne > 0) {
        layer->class[i]->text.items = (char **) calloc(ne, sizeof(char *)); /* should be more than enough space */
        if(!(layer->class[i]->text.items)) {
          msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
          return(MS_FAILURE);
        }
        layer->class[i]->text.indexes = (int *) malloc(ne*sizeof(int));
        if(!(layer->class[i]->text.indexes)) {
          msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
          return(MS_FAILURE);
        }
        layer->class[i]->text.numitems = 0;
        nt += ne;
      }
    }
  }

  /* always retrieve all items in some cases */
  if(layer->connectiontype == MS_INLINE || get_all == MS_TRUE) {
    msLayerGetItems(layer);
    if(nt > 0) /* need to realloc the array to accept the possible new items*/
      layer->items = (char **)realloc(layer->items, sizeof(char *)*(layer->numitems + nt));
  } else {
    rv = layer->vtable->LayerCreateItems(layer, nt);
    if(rv != MS_SUCCESS)
      return rv;
  }

  if(nt > 0) {
    if(layer->classitem) layer->classitemindex = string2list(layer->items, &(layer->numitems), layer->classitem);
    if(layer->filteritem) layer->filteritemindex = string2list(layer->items, &(layer->numitems), layer->filteritem);

    for(i=0; i<layer->numclasses; i++) {
      if(layer->class[i]->expression.type == MS_EXPRESSION) expression2list(layer->items, &(layer->numitems), &(layer->class[i]->expression));
      for(j=0; j<layer->class[i]->numstyles; j++) {
        if(layer->class[i]->styles[j]->rangeitem) layer->class[i]->styles[j]->rangeitemindex = string2list(layer->items, &(layer->numitems), layer->class[i]->styles[j]->rangeitem);
        for(k=0; k<MS_STYLE_BINDING_LENGTH; k++)
          if(layer->class[i]->styles[j]->bindings[k].item) layer->class[i]->styles[j]->bindings[k].index = string2list(layer->items, &(layer->numitems), layer->class[i]->styles[j]->bindings[k].item);
      }
    }

    if(layer->filter.type == MS_EXPRESSION) expression2list(layer->items, &(layer->numitems), &(layer->filter));

    if(layer->labelitem) layer->labelitemindex = string2list(layer->items, &(layer->numitems), layer->labelitem);
    for(i=0; i<layer->numclasses; i++) {
      if(layer->class[i]->text.type == MS_EXPRESSION) expression2list(layer->items, &(layer->numitems), &(layer->class[i]->text));
      for(k=0; k<MS_LABEL_BINDING_LENGTH; k++)
        if(layer->class[i]->label.bindings[k].item) layer->class[i]->label.bindings[k].index = string2list(layer->items, &(layer->numitems), layer->class[i]->label.bindings[k].item);
    }
  }

  if(metadata) {
    char **tokens;
    int n = 0;
    int j;
    int bFound = 0;

    tokens = msStringSplit(metadata, ',', &n);
    if(tokens) {
      for(i=0; i<n; i++) {
        bFound = 0;
        for(j=0; j<layer->numitems; j++) {
          if(strcmp(tokens[i], layer->items[j]) == 0) {
            bFound = 1;
            break;
          }
        }

        if(!bFound) {
          layer->numitems++;
          layer->items =  (char **)realloc(layer->items, sizeof(char *)*(layer->numitems));
          layer->items[layer->numitems-1] = strdup(tokens[i]);
        }
      }
      msFreeCharArray(tokens, n);
    }
  }

  /* populate the iteminfo array */
  if(layer->numitems == 0)
    return(MS_SUCCESS);

  return(msLayerInitItemInfo(layer));
}

/*
** A helper function to set the items to be retrieved with a particular shape. Unused at the moment but will be used
** from within MapScript. Should not need modification.
*/
int msLayerSetItems(layerObj *layer, char **items, int numitems)
{
  int i;
  /* Cleanup any previous item selection */
  msLayerFreeItemInfo(layer);
  if(layer->items) {
    msFreeCharArray(layer->items, layer->numitems);
    layer->items = NULL;
    layer->numitems = 0;
  }

  /* now allocate and set the layer item parameters  */
  layer->items = (char **)malloc(sizeof(char *)*numitems);
  if(!layer->items) {
    msSetError(MS_MEMERR, NULL, "msLayerSetItems()");
    return(MS_FAILURE);
  }

  for(i=0; i<numitems; i++)
    layer->items[i] = strdup(items[i]);
  layer->numitems = numitems;

  /* populate the iteminfo array */
  return(msLayerInitItemInfo(layer));

  return(MS_SUCCESS);
}

/*
** Fills a classObj with style info from the specified shape.  This is used
** with STYLEITEM AUTO when rendering shapes.
** For optimal results, this should be called immediately after 
** GetNextShape() or GetShape() so that the shape doesn't have to be read
** twice.
** 
*/
int msLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c, 
                        int tile, long record)
{
  if ( ! layer->vtable) {
      int rv =  msInitializeVirtualTable(layer);
      if (rv != MS_SUCCESS)
          return rv;
  }
  return layer->vtable->LayerGetAutoStyle(map, layer, c, tile, record);
}


/*
Returns the number of inline feature of a layer
*/
int msLayerGetNumFeatures(layerObj *layer) 
{
    if ( ! layer->vtable) {
        int rv =  msInitializeVirtualTable(layer);
        if (rv != MS_SUCCESS)
            return rv;
    }
    return layer->vtable->LayerGetNumFeatures(layer);
}

void 
msLayerSetProcessingKey( layerObj *layer, const char *key, const char *value)

{
    int len = strlen(key);
    int i;
    char *directive;

    directive = (char *) malloc(strlen(key)+strlen(value)+2);
    sprintf( directive, "%s=%s", key, value );

    for( i = 0; i < layer->numprocessing; i++ )
    {
        /* If the key is found, replace it */
        if( strncasecmp( key, layer->processing[i], len ) == 0 
            && layer->processing[i][len] == '=' )
        {
            free( layer->processing[i] );
            layer->processing[i] = directive;
            return;
        }
    }

    /* otherwise add the directive at the end. */

    msLayerAddProcessing( layer, directive );
    free( directive );
}

void msLayerAddProcessing( layerObj *layer, const char *directive )

{
    layer->numprocessing++;
    if( layer->numprocessing == 1 )
        layer->processing = (char **) malloc(2*sizeof(char *));
    else
        layer->processing = (char **) realloc(layer->processing, sizeof(char*) * (layer->numprocessing+1) );
    layer->processing[layer->numprocessing-1] = strdup(directive);
    layer->processing[layer->numprocessing] = NULL;
}

char *msLayerGetProcessing( layerObj *layer, int proc_index) {
    if (proc_index < 0 || proc_index >= layer->numprocessing) {
        msSetError(MS_CHILDERR, "Invalid processing index.", "msLayerGetProcessing()");
        return NULL;
    }
    else {
        return layer->processing[proc_index];
    }
}

char *msLayerGetProcessingKey( layerObj *layer, const char *key ) 
{
    int i, len = strlen(key);

    for( i = 0; i < layer->numprocessing; i++ )
    {
        if( strncasecmp(layer->processing[i],key,len) == 0 
            && layer->processing[i][len] == '=' )
            return layer->processing[i] + len + 1;
    }
    
    return NULL;
}

int msLayerClearProcessing( layerObj *layer ) {
    if (layer->numprocessing > 0) {
        msFreeCharArray( layer->processing, layer->numprocessing );
        layer->processing = NULL;
        layer->numprocessing = 0;
    }
    return layer->numprocessing;
}


int 
makeTimeFilter(layerObj *lp, 
               const char *timestring, 
               const char *timefield,
               const int addtimebacktics)
{
  
    char **atimes, **tokens = NULL;
    int numtimes,i, ntmp = 0;
    char *pszBuffer = NULL;
    int bOnlyExistingFilter = 0;

    if (!lp || !timestring || !timefield)
      return MS_FALSE;

    /* parse the time string. We support dicrete times (eg 2004-09-21),  */
    /* multiple times (2004-09-21, 2004-09-22, ...) */
    /* and range(s) (2004-09-21/2004-09-25, 2004-09-27/2004-09-29) */

    if (strstr(timestring, ",") == NULL && 
        strstr(timestring, "/") == NULL) /* discrete time */
    {   
        /*
        if(lp->filteritem) free(lp->filteritem);
        lp->filteritem = strdup(timefield);
        if (&lp->filter)
          freeExpression(&lp->filter);
        */

        if (&lp->filter)
        {
            /* if the filter is set and it's a sting type, concatenate it with
               the time. If not just free it */
            if (lp->filter.type == MS_EXPRESSION)
            {
                pszBuffer = msStringConcatenate(pszBuffer, "((");
                pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string);
                pszBuffer = msStringConcatenate(pszBuffer, ") and ");
            }
            else
            {
                freeExpression(&lp->filter);
            }
        }
        
        pszBuffer = msStringConcatenate(pszBuffer, "(");
        if (addtimebacktics)
          pszBuffer = msStringConcatenate(pszBuffer,  "`");

        if (addtimebacktics)
           pszBuffer = msStringConcatenate(pszBuffer, "[");
        pszBuffer = msStringConcatenate(pszBuffer, (char *)timefield);
        if (addtimebacktics)
          pszBuffer = msStringConcatenate(pszBuffer, "]");
        if (addtimebacktics)
          pszBuffer = msStringConcatenate(pszBuffer,  "`");

         
        pszBuffer = msStringConcatenate(pszBuffer, " = ");
        if (addtimebacktics)
          pszBuffer = msStringConcatenate(pszBuffer,  "`");
        else
          pszBuffer = msStringConcatenate(pszBuffer,  "'");

        pszBuffer = msStringConcatenate(pszBuffer, (char *)timestring);
        if (addtimebacktics)
          pszBuffer = msStringConcatenate(pszBuffer,  "`");
        else
          pszBuffer = msStringConcatenate(pszBuffer,  "'");

        pszBuffer = msStringConcatenate(pszBuffer, ")");
        
        /* if there was a filter, It was concatenate with an And ans should be closed*/
        if(&lp->filter && lp->filter.type == MS_EXPRESSION)
        {
            pszBuffer = msStringConcatenate(pszBuffer, ")");
        }

        loadExpressionString(&lp->filter, pszBuffer);

        if (pszBuffer)
          msFree(pszBuffer);

        return MS_TRUE;
    }
    
    atimes = msStringSplit(timestring, ',', &numtimes);
    if (atimes == NULL || numtimes < 1)
      return MS_FALSE;

    if (numtimes >= 1)
    {
        if (&lp->filter && lp->filter.type == MS_EXPRESSION)
        {
            pszBuffer = msStringConcatenate(pszBuffer, "((");
            pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string);
            pszBuffer = msStringConcatenate(pszBuffer, ") and ");
            /*this flag is used to indicate that the buffer contains only the 
              existing filter. It is set to 0 when time filter parts are
              added to the buffer */
            bOnlyExistingFilter = 1;
        }
        else
          freeExpression(&lp->filter);

        /* check to see if we have ranges by parsing the first entry */
        tokens = msStringSplit(atimes[0],  '/', &ntmp);
        if (ntmp == 2) /* ranges */
        {
            msFreeCharArray(tokens, ntmp);
            for (i=0; i<numtimes; i++)
            {
                 tokens = msStringSplit(atimes[i],  '/', &ntmp);
                 if (ntmp == 2)
                 {
                     if (pszBuffer && strlen(pszBuffer) > 0 && bOnlyExistingFilter == 0)
                       pszBuffer = msStringConcatenate(pszBuffer, " OR ");
                     else
                       pszBuffer = msStringConcatenate(pszBuffer, "(");

                     bOnlyExistingFilter = 0;

                     pszBuffer = msStringConcatenate(pszBuffer, "(");
                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer,  "`");

                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer, "[");
                     pszBuffer = msStringConcatenate(pszBuffer, (char *)timefield);
                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer, "]");
                     
                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer,  "`");

                     pszBuffer = msStringConcatenate(pszBuffer, " >= ");
                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer,  "`");
                     else
                       pszBuffer = msStringConcatenate(pszBuffer,  "'");

                     pszBuffer = msStringConcatenate(pszBuffer, tokens[0]);
                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer,  "`");
                     else
                       pszBuffer = msStringConcatenate(pszBuffer,  "'");
                     pszBuffer = msStringConcatenate(pszBuffer, " AND ");

                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer,  "`");

                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer, "[");
                     pszBuffer = msStringConcatenate(pszBuffer, (char *)timefield);
                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer, "]");
                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer,  "`");

                     pszBuffer = msStringConcatenate(pszBuffer, " <= ");
                     
                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer,  "`");
                     else
                       pszBuffer = msStringConcatenate(pszBuffer,  "'");
                     pszBuffer = msStringConcatenate(pszBuffer, tokens[1]);
                     if (addtimebacktics)
                       pszBuffer = msStringConcatenate(pszBuffer,  "`");
                     else
                       pszBuffer = msStringConcatenate(pszBuffer,  "'");
                     pszBuffer = msStringConcatenate(pszBuffer, ")");
                 }
                 
                  msFreeCharArray(tokens, ntmp);
            }
            if (pszBuffer && strlen(pszBuffer) > 0 && bOnlyExistingFilter == 0)
              pszBuffer = msStringConcatenate(pszBuffer, ")");
        }
        else if (ntmp == 1) /* multiple times */
        {
            msFreeCharArray(tokens, ntmp);
            pszBuffer = msStringConcatenate(pszBuffer, "(");
            for (i=0; i<numtimes; i++)
            {
                if (i > 0)
                  pszBuffer = msStringConcatenate(pszBuffer, " OR ");

                pszBuffer = msStringConcatenate(pszBuffer, "(");
                if (addtimebacktics)
                  pszBuffer = msStringConcatenate(pszBuffer, "`");
                  
                if (addtimebacktics)
                  pszBuffer = msStringConcatenate(pszBuffer, "[");
                pszBuffer = msStringConcatenate(pszBuffer, (char *)timefield);
                if (addtimebacktics)
                  pszBuffer = msStringConcatenate(pszBuffer, "]");

                if (addtimebacktics)
                  pszBuffer = msStringConcatenate(pszBuffer, "`");

                pszBuffer = msStringConcatenate(pszBuffer, " = ");
                  
                if (addtimebacktics)
                  pszBuffer = msStringConcatenate(pszBuffer, "`");
                else
                  pszBuffer = msStringConcatenate(pszBuffer,  "'");
                pszBuffer = msStringConcatenate(pszBuffer, atimes[i]);
                if (addtimebacktics)
                  pszBuffer = msStringConcatenate(pszBuffer,  "`");
                else
                  pszBuffer = msStringConcatenate(pszBuffer,  "'");
                pszBuffer = msStringConcatenate(pszBuffer, ")");
            } 
            pszBuffer = msStringConcatenate(pszBuffer, ")");
        }
        else
        {
            msFreeCharArray(atimes, numtimes);
            return MS_FALSE;
        }

        msFreeCharArray(atimes, numtimes);

        /* load the string to the filter */
        if (pszBuffer && strlen(pszBuffer) > 0)
        {
            if(&lp->filter && lp->filter.type == MS_EXPRESSION)
              pszBuffer = msStringConcatenate(pszBuffer, ")");
            /*
            if(lp->filteritem) 
              free(lp->filteritem);
            lp->filteritem = strdup(timefield);
            */     

            loadExpressionString(&lp->filter, pszBuffer);

            if (pszBuffer)
              msFree(pszBuffer);
        }

        return MS_TRUE;
                 
    }
    
     return MS_FALSE;
}

/**
  set the filter parameter for a time filter
**/

int msLayerSetTimeFilter(layerObj *lp, const char *timestring, 
                         const char *timefield) 
{
  if ( ! lp->vtable) {
      int rv =  msInitializeVirtualTable(lp);
      if (rv != MS_SUCCESS)
          return rv;
  }
  return lp->vtable->LayerSetTimeFilter(lp, timestring, timefield);
}   

int 
msLayerMakeBackticsTimeFilter(layerObj *lp, const char *timestring, 
                              const char *timefield) 
{
    return makeTimeFilter(lp, timestring, timefield, MS_TRUE);
}

int 
msLayerMakePlainTimeFilter(layerObj *lp, const char *timestring, 
                              const char *timefield) 
{
    return makeTimeFilter(lp, timestring, timefield, MS_FALSE);
}


/*
 * Dummies / default actions for layers
 */
int LayerDefaultInitItemInfo(layerObj *layer)
{
  return MS_SUCCESS;
}

void LayerDefaultFreeItemInfo(layerObj *layer)
{
  return;
}

int LayerDefaultOpen(layerObj *layer)
{
  return MS_FAILURE;
}

int LayerDefaultIsOpen(layerObj *layer)
{
  return MS_FALSE;
}

int LayerDefaultWhichShapes(layerObj *layer, rectObj rect)
{
  return MS_SUCCESS;
}

int LayerDefaultNextShape(layerObj *layer, shapeObj *shape)
{
  return MS_FAILURE;
}

int LayerDefaultResultsGetShape(layerObj *layer, shapeObj *shape, int tile, long record)
{
  return MS_FAILURE;
}

int LayerDefaultGetShape(layerObj *layer, shapeObj *shape, int tile, long record)
{
  return MS_FAILURE;
}

int LayerDefaultClose(layerObj *layer)
{
  return MS_SUCCESS;
}

int LayerDefaultGetItems(layerObj *layer)
{
  return MS_SUCCESS; /* returning no items is legit */
}

int 
msLayerApplyCondSQLFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                                 int iLayerIndex, int bOnlySpatialFilter)
{
#if USE_OGR
    return FLTLayerApplyCondSQLFilterToLayer(psNode, map, iLayerIndex, 
                                             bOnlySpatialFilter); 
#else
    return MS_FAILURE;
#endif
}

int 
msLayerApplyPlainFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                               int iLayerIndex, int bOnlySpatialFilter)
{
#if USE_OGR
    return FLTLayerApplyPlainFilterToLayer(psNode, map, iLayerIndex, 
                                           bOnlySpatialFilter); 
#else
    return MS_FAILURE;
#endif
}

int LayerDefaultGetExtent(layerObj *layer, rectObj *extent)
{
  return MS_FAILURE;
}

int LayerDefaultGetAutoStyle(mapObj *map, layerObj *layer, classObj *c, int tile, long record)
{
  msSetError(MS_MISCERR, "'STYLEITEM AUTO' not supported for this data source.", "msLayerGetAutoStyle()");
  return MS_FAILURE; 
}

int LayerDefaultCloseConnection(layerObj *layer)
{
  return MS_SUCCESS;
}

int LayerDefaultCreateItems(layerObj *layer, const int nt)
{
  if (nt > 0) {
    layer->items = (char **)calloc(nt, sizeof(char *)); /* should be more than enough space */
    if(!layer->items) {
      msSetError(MS_MEMERR, NULL, "LayerDefaultCreateItems()");
      return(MS_FAILURE);
    }
    layer->numitems = 0;
  }
  return MS_SUCCESS;
}

int LayerDefaultGetNumFeatures(layerObj *layer)
{
  msSetError(MS_SHPERR, "Not an inline layer", "msLayerGetNumFeatures()");
  return MS_FAILURE;
}

/************************************************************************/
/*                          LayerDefaultEscapeSQLParam                  */
/*                                                                      */
/*      Default function used to escape strings and avoid sql           */
/*      injection. Specific drivers should redefine if an escaping      */
/*      function is available in the driver.                            */
/************************************************************************/
char *LayerDefaultEscapeSQLParam(layerObj *layer, const char* pszString)
{
     char *pszEscapedStr=NULL;
     if (pszString)
     {
         int nSrcLen;
         char c;
         int i=0, j=0;
         nSrcLen = (int)strlen(pszString);
         pszEscapedStr = (char*) malloc( 2 * nSrcLen + 1);
         for(i = 0, j = 0; i < nSrcLen; i++)
         {
             c = pszString[i];
             if (c == '\'')
             {
                 pszEscapedStr[j++] = '\'';
                 pszEscapedStr[j++] = '\'';
             }
             else if (c == '\\')
             {
                 pszEscapedStr[j++] = '\\';
                 pszEscapedStr[j++] = '\\';
             }
             else
               pszEscapedStr[j++] = c;
         }
         pszEscapedStr[j] = 0;
     }  
     return pszEscapedStr;
}

/************************************************************************/
/*                          LayerDefaultEscapePropertyName              */
/*                                                                      */
/*      Return the property name in a properly escaped and quoted form. */
/************************************************************************/
char *LayerDefaultEscapePropertyName(layerObj *layer, const char* pszString)
{
     char* pszEscapedStr=NULL;
     int i, j = 0;   

     if (layer && pszString && strlen(pszString) > 0)
     {
         int nLength = strlen(pszString);

         pszEscapedStr = (char*) malloc( 1 + 2 * nLength + 1 + 1);
         pszEscapedStr[j++] = '"';

         for (i=0; i<nLength; i++)
         {
             char c = pszString[i];
             if (c == '"')
             {
                 pszEscapedStr[j++] = '"';
                 pszEscapedStr[j++] ='"';
             }
             else if (c == '\\')
             {
                 pszEscapedStr[j++] = '\\';
                 pszEscapedStr[j++] = '\\';
             }
             else
               pszEscapedStr[j++] = c;
         }
         pszEscapedStr[j++] = '"';
         pszEscapedStr[j++] = 0;
        
     }
     return pszEscapedStr;
}


/*
 * msConnectLayer
 *
 * This will connect layer object to the new layer type.
 * Caller is responsible to close previous layer correctly.
 * For Internal types the library_str is ignored, for PLUGIN it's
 * define what plugin to use. Returns MS_FAILURE or MS_SUCCESS.
 */
int msConnectLayer(layerObj *layer,
                   const int connectiontype,
                   const char *library_str)
{
    layer->connectiontype = connectiontype;
    /* For internal types, library_str is ignored */
    if (connectiontype == MS_PLUGIN) {
        int rv;
        msFree(layer->plugin_library);
        msFree(layer->plugin_library_original);

        layer->plugin_library_original = strdup(library_str);
        rv = msBuildPluginLibraryPath(&layer->plugin_library, 
                                      layer->plugin_library_original, 
                                      layer->map);
        if (rv != MS_SUCCESS) {
            return rv;
        }
    }
    return msInitializeVirtualTable(layer) ;   
}

static int populateVirtualTable(layerVTableObj *vtable)
{
  assert(vtable != NULL);

  vtable->LayerInitItemInfo = LayerDefaultInitItemInfo;
  vtable->LayerFreeItemInfo = LayerDefaultFreeItemInfo;
  vtable->LayerOpen = LayerDefaultOpen;
  vtable->LayerIsOpen = LayerDefaultIsOpen;
  vtable->LayerWhichShapes = LayerDefaultWhichShapes;

  vtable->LayerNextShape = LayerDefaultNextShape;
  vtable->LayerResultsGetShape = LayerDefaultResultsGetShape;
  vtable->LayerGetShape = LayerDefaultGetShape;
  vtable->LayerClose = LayerDefaultClose;
  vtable->LayerGetItems = LayerDefaultGetItems;
  vtable->LayerGetExtent = LayerDefaultGetExtent;

  vtable->LayerGetAutoStyle = LayerDefaultGetAutoStyle;
  vtable->LayerCloseConnection = LayerDefaultCloseConnection;
  vtable->LayerSetTimeFilter = msLayerMakePlainTimeFilter;

  vtable->LayerApplyFilterToLayer = msLayerApplyPlainFilterToLayer;

  vtable->LayerCreateItems = LayerDefaultCreateItems;
    
  vtable->LayerGetNumFeatures = LayerDefaultGetNumFeatures;

  vtable->LayerEscapeSQLParam = LayerDefaultEscapeSQLParam;

  vtable->LayerEscapePropertyName = LayerDefaultEscapePropertyName;

  return MS_SUCCESS;
}

static int createVirtualTable(layerVTableObj **vtable)
{
  *vtable = malloc(sizeof(**vtable));
  if ( ! *vtable) {
    return MS_FAILURE;
  }
  return populateVirtualTable(*vtable);
}

static int destroyVirtualTable(layerVTableObj **vtable)
{
  memset(*vtable, 0, sizeof(**vtable));
  msFree(*vtable);
  *vtable = NULL;
  return MS_SUCCESS;
}

int msInitializeVirtualTable(layerObj *layer)
{
  if (layer->vtable) {
    destroyVirtualTable(&layer->vtable);
  }
  createVirtualTable(&layer->vtable);
   
  if(layer->features && layer->connectiontype != MS_GRATICULE ) 
    layer->connectiontype = MS_INLINE;

  if(layer->tileindex && layer->connectiontype == MS_SHAPEFILE)
    layer->connectiontype = MS_TILED_SHAPEFILE;

  if(layer->type == MS_LAYER_RASTER && layer->connectiontype != MS_WMS)
    layer->connectiontype = MS_RASTER;

  switch(layer->connectiontype) {
    case(MS_INLINE):
      return(msINLINELayerInitializeVirtualTable(layer));
      break;
    case(MS_SHAPEFILE):
      return(msShapeFileLayerInitializeVirtualTable(layer));
      break;
    case(MS_TILED_SHAPEFILE):
      return(msTiledSHPLayerInitializeVirtualTable(layer));
      break;
    case(MS_SDE):
      return(msSDELayerInitializeVirtualTable(layer));
      break;
    case(MS_OGR):
      return(msOGRLayerInitializeVirtualTable(layer));
      break;
    case(MS_POSTGIS):
      return(msPostGISLayerInitializeVirtualTable(layer));
      break;
    case(MS_WMS):
      /* WMS should be treated as a raster layer */
      return(msRASTERLayerInitializeVirtualTable(layer));
      break;
    case(MS_ORACLESPATIAL):
      return(msOracleSpatialLayerInitializeVirtualTable(layer));
      break;
    case(MS_WFS):
      return(msWFSLayerInitializeVirtualTable(layer));
      break;
    case(MS_GRATICULE):
      return(msGraticuleLayerInitializeVirtualTable(layer));
      break;
    case(MS_MYGIS):
      return(msMYGISLayerInitializeVirtualTable(layer));
      break;         
    case(MS_RASTER):
      return(msRASTERLayerInitializeVirtualTable(layer));
      break;
    case(MS_PLUGIN):
      return(msPluginLayerInitializeVirtualTable(layer));
      break;
    default:
      msSetError(MS_MISCERR, "Unknown connectiontype, it was %d", "msInitializeVirtualTable()", layer->connectiontype);
      return MS_FAILURE;
      break;
    }

    /* not reached */
    return MS_FAILURE;
}

/* 
 * INLINE: Virtual table functions 
 */

int msINLINELayerIsOpen(layerObj *layer)
{
    if (layer->currentfeature)
        return(MS_TRUE);
    else
        return(MS_FALSE);
}


int msINLINELayerOpen(layerObj *layer)
{
    layer->currentfeature = layer->features; /* point to the begining of the feature list */
    return(MS_SUCCESS);
}

/* Author: Cristoph Spoerri and Sean Gillies */
int msINLINELayerGetShape(layerObj *layer, shapeObj *shape, int tile, long shapeindex) 
{
    int i=0;
    featureListNodeObjPtr current;

    /* tile ; */ /* Not used -- commented out to silence compiler warning.  hobu*/
    current = layer->features;
    while (current!=NULL && i!=shapeindex) {
        i++;
        current = current->next;
    }
    if (current == NULL) {
        msSetError(MS_SHPERR, "No inline feature with this index.",
                   "msINLINELayerGetShape()");
        return MS_FAILURE;
    } 
    
    if (msCopyShape(&(current->shape), shape) != MS_SUCCESS) {
        msSetError(MS_SHPERR, "Cannot retrieve inline shape. There some problem with the shape", "msLayerGetShape()");
        return MS_FAILURE;
    }
    /* check for the expected size of the values array */
    if (layer->numitems > shape->numvalues) {
        shape->values = (char **)realloc(shape->values, sizeof(char *)*(layer->numitems));
        for (i = shape->numvalues; i < layer->numitems; i++)
            shape->values[i] = strdup("");
    }
    return MS_SUCCESS;
}

int msINLINELayerNextShape(layerObj *layer, shapeObj *shape) 
{
    if( ! (layer->currentfeature)) {
        /* out of features */
        return(MS_DONE); 
    }

    msCopyShape(&(layer->currentfeature->shape), shape);

    layer->currentfeature = layer->currentfeature->next;

    /* check for the expected size of the values array */
    if (layer->numitems > shape->numvalues) {
        int i;
        shape->values = (char **)realloc(shape->values, sizeof(char *)*(layer->numitems));
        for (i = shape->numvalues; i < layer->numitems; i++)
            shape->values[i] = strdup("");
    }

    return(MS_SUCCESS);
}

int msINLINELayerGetNumFeatures(layerObj *layer)
{
    int i = 0;
    featureListNodeObjPtr current;

    current = layer->features;
    while (current != NULL) {
        i++;
        current = current->next;
    }
    return i;
}



/*
Returns an escaped string
*/
char  *msLayerEscapeSQLParam(layerObj *layer, const char*pszString) 
{
    if ( ! layer->vtable) {
        int rv =  msInitializeVirtualTable(layer);
        if (rv != MS_SUCCESS)
            return "";
    }
    return layer->vtable->LayerEscapeSQLParam(layer, pszString);
}

char  *msLayerEscapePropertyName(layerObj *layer, const char*pszString) 
{
    if ( ! layer->vtable) {
        int rv =  msInitializeVirtualTable(layer);
        if (rv != MS_SUCCESS)
            return "";
    }
    return layer->vtable->LayerEscapePropertyName(layer, pszString);
}


int
msINLINELayerInitializeVirtualTable(layerObj *layer)
{
    assert(layer != NULL);
    assert(layer->vtable != NULL);

    /* layer->vtable->LayerInitItemInfo, use default */
    /* layer->vtable->LayerFreeItemInfo, use default */
    layer->vtable->LayerOpen = msINLINELayerOpen;
    layer->vtable->LayerIsOpen = msINLINELayerIsOpen;
    /* layer->vtable->LayerWhichShapes, use default */
    layer->vtable->LayerNextShape = msINLINELayerNextShape;
    layer->vtable->LayerResultsGetShape = msINLINELayerGetShape; /* no special version, use ...GetShape() */
    layer->vtable->LayerGetShape = msINLINELayerGetShape;
    /* layer->vtable->LayerClose, use default */
    /* layer->vtable->LayerGetItems, use default */

    /* 
     * Original code contained following 
     * TODO: need to compute extents
     */
    /* layer->vtable->LayerGetExtent, use default */

    /* layer->vtable->LayerGetAutoStyle, use default */
    /* layer->vtable->LayerCloseConnection, use default */
    layer->vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;

    /* layer->vtable->LayerApplyFilterToLayer, use default */

    /* layer->vtable->LayerCreateItems, use default */
    layer->vtable->LayerGetNumFeatures = msINLINELayerGetNumFeatures;

    /*layer->vtable->LayerEscapeSQLParam, use default*/
    /*layer->vtable->LayerEscapePropertyName, use default*/
    return MS_SUCCESS;
}
