#include "maptemplate.h"
#include "cgiutil.h"

/*
** Is a particular layer or group on, that is was it requested explicitly by the user.
*/
int isOn(mapservObj *msObj, char *name, char *group)
{
  int i;

  for(i=0;i<msObj->NumLayers;i++) {
    if(name && strcmp(msObj->Layers[i], name) == 0)  return(MS_TRUE);
    if(group && strcmp(msObj->Layers[i], group) == 0) return(MS_TRUE);
  }

  return(MS_FALSE);
}

char *processLine(mapservObj* msObj, char* instr, int mode)
{
  int i, j;
  //char repstr[1024], substr[1024], *outstr; // repstr = replace string, substr = sub string
  char repstr[5120], substr[5120], *outstr;
  struct hashObj *tp=NULL;
  char *encodedstr;
   
#ifdef USE_PROJ
  rectObj llextent;
  pointObj llpoint;
#endif

  outstr = strdup(instr); // work from a copy
  
 outstr = gsub(outstr, "[version]",  msGetVersion());

  sprintf(repstr, "%s%s%s.%s", msObj->Map->web.imageurl, msObj->Map->name, msObj->Id, MS_IMAGE_EXTENSION(msObj->Map->imagetype));
  outstr = gsub(outstr, "[img]", repstr);
  sprintf(repstr, "%s%sref%s.%s", msObj->Map->web.imageurl, msObj->Map->name, msObj->Id, MS_IMAGE_EXTENSION(msObj->Map->imagetype));
  outstr = gsub(outstr, "[ref]", repstr);
  sprintf(repstr, "%s%sleg%s.%s", msObj->Map->web.imageurl, msObj->Map->name, msObj->Id, MS_IMAGE_EXTENSION(msObj->Map->imagetype));
  outstr = gsub(outstr, "[legend]", repstr);
  sprintf(repstr, "%s%ssb%s.%s", msObj->Map->web.imageurl, msObj->Map->name, msObj->Id, MS_IMAGE_EXTENSION(msObj->Map->imagetype));
  outstr = gsub(outstr, "[scalebar]", repstr);

  if(msObj->SaveQuery) {
    sprintf(repstr, "%s%s%s%s", msObj->Map->web.imagepath, msObj->Map->name, msObj->Id, MS_QUERY_EXTENSION);
    outstr = gsub(outstr, "[queryfile]", repstr);
  }
  
  if(msObj->SaveMap) {
    sprintf(repstr, "%s%s%s.map", msObj->Map->web.imagepath, msObj->Map->name, msObj->Id);
    outstr = gsub(outstr, "[map]", repstr);
  }

  sprintf(repstr, "%s", getenv("HTTP_HOST")); 
  outstr = gsub(outstr, "[host]", repstr);
  sprintf(repstr, "%s", getenv("SERVER_PORT"));
  outstr = gsub(outstr, "[port]", repstr);
  
  sprintf(repstr, "%s", msObj->Id);
  outstr = gsub(outstr, "[id]", repstr);
  
  strcpy(repstr, ""); // Layer list for a "GET" request
  for(i=0;i<msObj->NumLayers;i++)    
    sprintf(repstr, "%s&layer=%s", repstr, msObj->Layers[i]);
  outstr = gsub(outstr, "[get_layers]", repstr);
  
  strcpy(repstr, ""); // Layer list for a "POST" request
  for(i=0;i<msObj->NumLayers;i++)
    sprintf(repstr, "%s%s ", repstr, msObj->Layers[i]);
  trimBlanks(repstr);
  outstr = gsub(outstr, "[layers]", repstr);

  encodedstr = encode_url(repstr);
  outstr = gsub(outstr, "[layers_esc]", encodedstr);
  free(encodedstr);

  for(i=0;i<msObj->Map->numlayers;i++) { // Set form widgets (i.e. checkboxes, radio and select lists), note that default layers don't show up here
    if(isOn(msObj, msObj->Map->layers[i].name, msObj->Map->layers[i].group) == MS_TRUE) {
      if(msObj->Map->layers[i].group) {
	sprintf(substr, "[%s_select]", msObj->Map->layers[i].group);
	outstr = gsub(outstr, substr, "selected");
	sprintf(substr, "[%s_check]", msObj->Map->layers[i].group);
	outstr = gsub(outstr, substr, "checked");
      }
      sprintf(substr, "[%s_select]", msObj->Map->layers[i].name);
      outstr = gsub(outstr, substr, "selected");
      sprintf(substr, "[%s_check]", msObj->Map->layers[i].name);
      outstr = gsub(outstr, substr, "checked");
    } else {
      if(msObj->Map->layers[i].group) {
	sprintf(substr, "[%s_select]", msObj->Map->layers[i].group);
	outstr = gsub(outstr, substr, "");
	sprintf(substr, "[%s_check]", msObj->Map->layers[i].group);
	outstr = gsub(outstr, substr, "");
      }
      sprintf(substr, "[%s_select]", msObj->Map->layers[i].name);
      outstr = gsub(outstr, substr, "");
      sprintf(substr, "[%s_check]", msObj->Map->layers[i].name);
      outstr = gsub(outstr, substr, "");
    }
  }

  for(i=-1;i<=1;i++) { /* make zoom direction persistant */
    if(msObj->ZoomDirection == i) {
      sprintf(substr, "[zoomdir_%d_select]", i);
      outstr = gsub(outstr, substr, "selected");
      sprintf(substr, "[zoomdir_%d_check]", i);
      outstr = gsub(outstr, substr, "checked");
    } else {
      sprintf(substr, "[zoomdir_%d_select]", i);
      outstr = gsub(outstr, substr, "");
      sprintf(substr, "[zoomdir_%d_check]", i);
      outstr = gsub(outstr, substr, "");
    }
  }
  
  for(i=MINZOOM;i<=MAXZOOM;i++) { /* make zoom persistant */
    if(msObj->Zoom == i) {
      sprintf(substr, "[zoom_%d_select]", i);
      outstr = gsub(outstr, substr, "selected");
      sprintf(substr, "[zoom_%d_check]", i);
      outstr = gsub(outstr, substr, "checked");
    } else {
      sprintf(substr, "[zoom_%d_select]", i);
      outstr = gsub(outstr, substr, "");
      sprintf(substr, "[zoom_%d_check]", i);
      outstr = gsub(outstr, substr, "");
    }
  }

  // allow web object metadata access in template
  if(msObj->Map->web.metadata && strstr(outstr, "web_")) {
    for(j=0; j<MS_HASHSIZE; j++) {
      if (msObj->Map->web.metadata[j] != NULL) {
	for(tp=msObj->Map->web.metadata[j]; tp!=NULL; tp=tp->next) {            
	  sprintf(substr, "[web_%s]", tp->key);
	  outstr = gsub(outstr, substr, tp->data);  
	  sprintf(substr, "[web_%s_esc]", tp->key);
       
      encodedstr = encode_url(tp->data);
	  outstr = gsub(outstr, substr, encodedstr);
      free(encodedstr);
	}
      }
    }
  }

  // allow layer metadata access in template
  for(i=0;i<msObj->Map->numlayers;i++) {
    if(msObj->Map->layers[i].metadata && strstr(outstr, msObj->Map->layers[i].name)) {
      for(j=0; j<MS_HASHSIZE; j++) {
	if (msObj->Map->layers[i].metadata[j] != NULL) {
	  for(tp=msObj->Map->layers[i].metadata[j]; tp!=NULL; tp=tp->next) {            
	    sprintf(substr, "[%s_%s]", msObj->Map->layers[i].name, tp->key);
	    if(msObj->Map->layers[i].status == MS_ON)
	      outstr = gsub(outstr, substr, tp->data);  
	    else
	      outstr = gsub(outstr, substr, "");
	    sprintf(substr, "[%s_%s_esc]", msObj->Map->layers[i].name, tp->key);
	    if(msObj->Map->layers[i].status == MS_ON) {
          encodedstr = encode_url(tp->data);
          outstr = gsub(outstr, substr, encodedstr);
          free(encodedstr);
        }
	    else
	      outstr = gsub(outstr, substr, "");
	  }
	}
      }
    }
  }


  sprintf(repstr, "%f", msObj->MapPnt.x);
  outstr = gsub(outstr, "[mapx]", repstr);
  sprintf(repstr, "%f", msObj->MapPnt.y);
  outstr = gsub(outstr, "[mapy]", repstr);
  
  sprintf(repstr, "%f", msObj->Map->extent.minx); // Individual mapextent elements for spatial query building 
  outstr = gsub(outstr, "[minx]", repstr);
  sprintf(repstr, "%f", msObj->Map->extent.maxx);
  outstr = gsub(outstr, "[maxx]", repstr);
  sprintf(repstr, "%f", msObj->Map->extent.miny);
  outstr = gsub(outstr, "[miny]", repstr);
  sprintf(repstr, "%f", msObj->Map->extent.maxy);
  outstr = gsub(outstr, "[maxy]", repstr);
  sprintf(repstr, "%f %f %f %f", msObj->Map->extent.minx, msObj->Map->extent.miny,  msObj->Map->extent.maxx, msObj->Map->extent.maxy);
  outstr = gsub(outstr, "[mapext]", repstr);
   
  encodedstr =  encode_url(repstr);
  outstr = gsub(outstr, "[mapext_esc]", encodedstr);
  free(encodedstr);
  
  sprintf(repstr, "%f", msObj->RawExt.minx); // Individual raw extent elements for spatial query building
  outstr = gsub(outstr, "[rawminx]", repstr);
  sprintf(repstr, "%f", msObj->RawExt.maxx);
  outstr = gsub(outstr, "[rawmaxx]", repstr);
  sprintf(repstr, "%f", msObj->RawExt.miny);
  outstr = gsub(outstr, "[rawminy]", repstr);
  sprintf(repstr, "%f", msObj->RawExt.maxy);
  outstr = gsub(outstr, "[rawmaxy]", repstr);
  sprintf(repstr, "%f %f %f %f", msObj->RawExt.minx, msObj->RawExt.miny,  msObj->RawExt.maxx, msObj->RawExt.maxy);
  outstr = gsub(outstr, "[rawext]", repstr);
  
  encodedstr = encode_url(repstr);
  outstr = gsub(outstr, "[rawext_esc]", encodedstr);
  free(encodedstr);
    
#ifdef USE_PROJ
  if((strstr(outstr, "lat]") || strstr(outstr, "lon]") || strstr(outstr, "lon_esc]"))
     && msObj->Map->projection.proj != NULL
     && !pj_is_latlong(msObj->Map->projection.proj) ) {
    llextent=msObj->Map->extent;
    llpoint=msObj->MapPnt;
    msProjectRect(&(msObj->Map->projection), &(msObj->Map->latlon), &llextent);
    msProjectPoint(&(msObj->Map->projection), &(msObj->Map->latlon), &llpoint);

    sprintf(repstr, "%f", llpoint.x);
    outstr = gsub(outstr, "[maplon]", repstr);
    sprintf(repstr, "%f", llpoint.y);
    outstr = gsub(outstr, "[maplat]", repstr);
    
    sprintf(repstr, "%f", llextent.minx); /* map extent as lat/lon */
    outstr = gsub(outstr, "[minlon]", repstr);
    sprintf(repstr, "%f", llextent.maxx);
    outstr = gsub(outstr, "[maxlon]", repstr);
    sprintf(repstr, "%f", llextent.miny);
    outstr = gsub(outstr, "[minlat]", repstr);
    sprintf(repstr, "%f", llextent.maxy);
    outstr = gsub(outstr, "[maxlat]", repstr);    
    sprintf(repstr, "%f %f %f %f", llextent.minx, llextent.miny,  llextent.maxx, llextent.maxy);
    outstr = gsub(outstr, "[mapext_latlon]", repstr);
     
    encodedstr = encode_url(repstr);
    outstr = gsub(outstr, "[mapext_latlon_esc]", encodedstr);
    free(encodedstr);
  }
#endif

  sprintf(repstr, "%d %d", msObj->Map->width, msObj->Map->height);
  outstr = gsub(outstr, "[mapsize]", repstr);
   
  encodedstr = encode_url(repstr);
  outstr = gsub(outstr, "[mapsize_esc]", encodedstr);
  free(encodedstr);

  sprintf(repstr, "%d", msObj->Map->width);
  outstr = gsub(outstr, "[mapwidth]", repstr);
  sprintf(repstr, "%d", msObj->Map->height);
  outstr = gsub(outstr, "[mapheight]", repstr);
  
  sprintf(repstr, "%f", msObj->Map->scale);
  outstr = gsub(outstr, "[scale]", repstr);
  
  sprintf(repstr, "%.1f %.1f", (msObj->Map->width-1)/2.0, (msObj->Map->height-1)/2.0);
  outstr = gsub(outstr, "[center]", repstr);
  sprintf(repstr, "%.1f", (msObj->Map->width-1)/2.0);
  outstr = gsub(outstr, "[center_x]", repstr);
  sprintf(repstr, "%.1f", (msObj->Map->height-1)/2.0);
  outstr = gsub(outstr, "[center_y]", repstr);      

  // These are really for situations with multiple result sets only, but often used in header/footer  
  sprintf(repstr, "%d", msObj->NR); // total number of results
  outstr = gsub(outstr, "[nr]", repstr);  
  sprintf(repstr, "%d", msObj->NL); // total number of layers with results
  outstr = gsub(outstr, "[nl]", repstr);

  if(msObj->ResultLayer) {
    sprintf(repstr, "%d", msObj->NLR); // total number of results within this layer
    outstr = gsub(outstr, "[nlr]", repstr);
    sprintf(repstr, "%d", msObj->RN); // sequential (eg. 1..n) result number within all layers
    outstr = gsub(outstr, "[rn]", repstr);
    sprintf(repstr, "%d", msObj->LRN); // sequential (eg. 1..n) result number within this layer
    outstr = gsub(outstr, "[lrn]", repstr);
    outstr = gsub(outstr, "[cl]", msObj->ResultLayer->name); // current layer name    
    // if(ResultLayer->description) outstr = gsub(outstr, "[cd]", ResultLayer->description); // current layer description
  }

  if(mode == QUERY) { // return shape and/or values	
    
    sprintf(repstr, "%f %f", (msObj->ResultShape.bounds.maxx + msObj->ResultShape.bounds.minx)/2, (msObj->ResultShape.bounds.maxy + msObj->ResultShape.bounds.miny)/2); 
    outstr = gsub(outstr, "[shpmid]", repstr);
    sprintf(repstr, "%f", (msObj->ResultShape.bounds.maxx + msObj->ResultShape.bounds.minx)/2);
    outstr = gsub(outstr, "[shpmidx]", repstr);
    sprintf(repstr, "%f", (msObj->ResultShape.bounds.maxy + msObj->ResultShape.bounds.miny)/2);
    outstr = gsub(outstr, "[shpmidy]", repstr);
    
    sprintf(repstr, "%f %f %f %f", msObj->ResultShape.bounds.minx, msObj->ResultShape.bounds.miny,  msObj->ResultShape.bounds.maxx, msObj->ResultShape.bounds.maxy);
    outstr = gsub(outstr, "[shpext]", repstr);
     
    encodedstr = encode_url(repstr);
    outstr = gsub(outstr, "[shpext_esc]", encodedstr);
    free(encodedstr);
     
    sprintf(repstr, "%f", msObj->ResultShape.bounds.minx);
    outstr = gsub(outstr, "[shpminx]", repstr);
    sprintf(repstr, "%f", msObj->ResultShape.bounds.miny);
    outstr = gsub(outstr, "[shpminy]", repstr);
    sprintf(repstr, "%f", msObj->ResultShape.bounds.maxx);
    outstr = gsub(outstr, "[shpmaxx]", repstr);
    sprintf(repstr, "%f", msObj->ResultShape.bounds.maxy);
    outstr = gsub(outstr, "[shpmaxy]", repstr);
    
    sprintf(repstr, "%ld", msObj->ResultShape.index);
    outstr = gsub(outstr, "[shpidx]", repstr);
    sprintf(repstr, "%d", msObj->ResultShape.tileindex);
    outstr = gsub(outstr, "[tileidx]", repstr);  

    for(i=0;i<msObj->ResultLayer->numitems;i++) {	 
      sprintf(substr, "[%s]", msObj->ResultLayer->items[i]);
      if(strstr(outstr, substr) != NULL)
	outstr = gsub(outstr, substr, msObj->ResultShape.values[i]);
      sprintf(substr, "[%s_esc]", msObj->ResultLayer->items[i]);
      if(strstr(outstr, substr) != NULL) {
        encodedstr = encode_url(msObj->ResultShape.values[i]);
        outstr = gsub(outstr, substr, encodedstr);
        free(encodedstr);
      }
    }
    
    // FIX: need to re-incorporate JOINS at some point
  }
  
  for(i=0;i<msObj->NumParams;i++) { 
    sprintf(substr, "[%s]", msObj->ParamNames[i]);
    outstr = gsub(outstr, substr, msObj->ParamValues[i]);
    sprintf(substr, "[%s_esc]", msObj->ParamNames[i]);
    
    encodedstr = encode_url(msObj->ParamValues[i]);
    outstr = gsub(outstr, substr, encodedstr);
    free(encodedstr);
  }

  return(outstr);
}

int msReturnPage(mapservObj* msObj, char* html, int mode)
{
  FILE *stream;
  char line[MS_BUFFER_LENGTH], *tmpline;

  regex_t re; /* compiled regular expression to be matched */ 

  if(regcomp(&re, MS_TEMPLATE_EXPR, REG_EXTENDED|REG_NOSUB) != 0) {
    msSetError(MS_REGEXERR, NULL, "msReturnPage()");
    return MS_FAILURE;
//    writeError();
  }

  if(regexec(&re, html, 0, NULL, 0) != 0) { /* no match */
    regfree(&re);
    msSetError(MS_WEBERR, "Malformed template name.", "msReturnPage()");
    return MS_FAILURE;
//    writeError();
  }
  regfree(&re);

  if((stream = fopen(html, "r")) == NULL) {
    msSetError(MS_IOERR, html, "msReturnPage()");
    return MS_FAILURE;
//    writeError();
  } 

  while(fgets(line, MS_BUFFER_LENGTH, stream) != NULL) { /* now on to the end of the file */

    if(strchr(line, '[') != NULL) {
      tmpline = processLine(msObj, line, mode);
      printf("%s", tmpline);
      free(tmpline);
    } else
      printf("%s", line);

   fflush(stdout);
  } // next line

  fclose(stream);

  return MS_SUCCESS;
}

int msReturnURL(mapservObj* msObj, char* url, int mode)
{
  char *tmpurl;

  if(url == NULL) {
    msSetError(MS_WEBERR, "Empty URL.", "msReturnURL()");
    return MS_FAILURE;
//    writeError();
  }

  tmpurl = processLine(msObj, url, mode);
  redirect(tmpurl);
  free(tmpurl);
   
  return MS_SUCCESS;
}


int msReturnQuery(mapservObj* msObj)
{
  int status;
  int i,j;

  char *template;

  layerObj *lp=NULL;

  msInitShape(&(msObj->ResultShape)); // ResultShape is a global var define in mapserv.h

  if((msObj->Mode == ITEMQUERY) || (msObj->Mode == QUERY)) { // may need to handle a URL result set

    for(i=(msObj->Map->numlayers-1); i>=0; i--) {
      lp = &(msObj->Map->layers[i]);

      if(!lp->resultcache) continue;
      if(lp->resultcache->numresults > 0) break;
    }

    if(lp->class[(int)(lp->resultcache->results[0].classindex)].template) 
      template = lp->class[(int)(lp->resultcache->results[0].classindex)].template;
    else 
      template = lp->template;

    if(TEMPLATE_TYPE(template) == MS_URL) {
      msObj->ResultLayer = lp;

      status = msLayerOpen(lp, msObj->Map->shapepath);
      if(status != MS_SUCCESS)
         return status;

      // retrieve all the item names
      status = msLayerGetItems(lp);
      if(status != MS_SUCCESS)
         return status;

      status = msLayerGetShape(lp, &(msObj->ResultShape), lp->resultcache->results[0].tileindex, lp->resultcache->results[0].shapeindex);
      if(status != MS_SUCCESS)
         return status;

      if (msReturnURL(msObj, template, QUERY) != MS_SUCCESS)
           return MS_FAILURE;
      
      msFreeShape(&(msObj->ResultShape));
      msLayerClose(lp);
      msObj->ResultLayer = NULL;

      return MS_SUCCESS;
    }
  }

  msObj->NR = msObj->NL = 0;
  for(i=0; i<msObj->Map->numlayers; i++) { // compute some totals
    lp = &(msObj->Map->layers[i]);

    if(!lp->resultcache) continue;

    if(lp->resultcache->numresults > 0) { 
      msObj->NL++;
      msObj->NR += lp->resultcache->numresults;
    }
  }

  printf("Content-type: text/html%c%c", 10, 10); // write MIME header
  printf("<!-- %s -->\n", msGetVersion());
  fflush(stdout);
  
  if(msObj->Map->web.header)
     if (msReturnPage(msObj, msObj->Map->web.header, BROWSE) != MS_SUCCESS)
       return MS_FAILURE;

  msObj->RN = 1; // overall result number
  for(i=(msObj->Map->numlayers-1); i>=0; i--) {
    msObj->ResultLayer = lp = &(msObj->Map->layers[i]);

    if(!lp->resultcache) continue;
    if(lp->resultcache->numresults <= 0) continue;

    msObj->NLR = lp->resultcache->numresults; 

    if(lp->header) 
       if (msReturnPage(msObj, lp->header, BROWSE) != MS_SUCCESS)
         return MS_FAILURE;

    // open this layer
    status = msLayerOpen(lp, msObj->Map->shapepath);
    if(status != MS_SUCCESS)
       return status;

    // retrieve all the item names
    status = msLayerGetItems(lp);
    if(status != MS_SUCCESS)
       return status;

    msObj->LRN = 1; // layer result number
    for(j=0; j<lp->resultcache->numresults; j++) {
      status = msLayerGetShape(lp, &(msObj->ResultShape), lp->resultcache->results[j].tileindex, lp->resultcache->results[j].shapeindex);
      if(status != MS_SUCCESS)
         return status;
      
      if(lp->class[(int)(lp->resultcache->results[j].classindex)].template) 
	template = lp->class[(int)(lp->resultcache->results[j].classindex)].template;
      else 
	template = lp->template;

      if (msReturnPage(msObj, template, QUERY) != MS_SUCCESS)
         return MS_FAILURE;

      msFreeShape(&(msObj->ResultShape)); // init too

      msObj->RN++; // increment counters
      msObj->LRN++;
    }

    if(lp->footer) 
       if (msReturnPage(msObj, lp->footer, BROWSE) != MS_SUCCESS)
         return MS_FAILURE;

    msLayerClose(lp);
    msObj->ResultLayer = NULL;
  }

  if(msObj->Map->web.footer) 
     return msReturnPage(msObj, msObj->Map->web.footer, BROWSE);

  return MS_SUCCESS;
}

mapservObj*  msAllocMapServObj()
{
   mapservObj* msObj = malloc(sizeof(mapservObj));
   
   msObj->SaveMap=MS_FALSE;
   msObj->SaveQuery=MS_FALSE; // should the query and/or map be saved 

   msObj->ParamNames=NULL;
   msObj->ParamValues=NULL;
   msObj->NumParams=0;

   msObj->Map=NULL;

   msObj->NumLayers=0; /* number of layers specfied by a user */


   msObj->RawExt.minx=-1;
   msObj->RawExt.miny=-1;
   msObj->RawExt.maxx=-1;
   msObj->RawExt.maxy=-1;

   msObj->fZoom=1;
   msObj->Zoom=1; /* default for browsing */
   
   msObj->ResultLayer=NULL;
   

   msObj->MapPnt.x=-1;
   msObj->MapPnt.y=-1;

   msObj->ZoomDirection=0; /* whether zooming in or out, default is pan or 0 */

   msObj->Mode=BROWSE; /* can be BROWSE, QUERY, etc. */
   msObj->Id[0]='\0'; /* big enough for time + pid */

   /* 
    ** variables for multiple query results processing 
    */
   msObj->RN=0; /* overall result number */
   msObj->LRN=0; /* result number within a layer */
   msObj->NL=0; /* total number of layers with results */
   msObj->NR=0; /* total number or results */
   msObj->NLR=0; /* number of results in a layer */
   
   return msObj;
}

void msFreeMapServObj(mapservObj* msObj)
{
   free(msObj);
}
