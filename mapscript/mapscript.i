/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for the MapServer mapscript module
   Author:   Steve Lime 
             Sean Gillies, sgillies@frii.com
             
   ===========================================================================
   Copyright (c) 1996-2001 Regents of the University of Minnesota.
   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
 
   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.
 
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ===========================================================================

   Keep an eye out for changes in this file -- we are beginning to move
   class extension defintions to their own files in mapscript/swiginc.  See
   for example hashtable.i and owsrequest.i.

   ===========================================================================
*/
   
// language specific initialization
#ifdef SWIGTCL8
%module Mapscript
%{

/* static global copy of Tcl interp */
static Tcl_Interp *SWIG_TCL_INTERP;

%}

%init %{
#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
    return TCL_ERROR;
  }
  /* save Tcl interp pointer to be used in getImageToVar() */
  SWIG_TCL_INTERP = interp;
#endif
%}
#endif

%module mapscript
%{
#include "../../map.h"
#include "../../maptemplate.h"
#include "../../mapogcsld.h"
#include "../../mapows.h"
#include "../../cgiutil.h"
#include "../../mapcopy.h"
%}

%include typemaps.i
%include constraints.i


/******************************************************************************
   Supporting 'None' as an argument to attribute accessor functions
 ******************************************************************************
 *
   Typemaps to support NULL in attribute accessor functions
   provided to Steve Lime by David Beazley and tested for Python
   only by Sean Gillies.
 *
   With the use of these typemaps and some filtering on the mapscript
   wrapper code to change the argument parsing of *_set() methods from
   "Os" to "Oz", we can execute statements like
 *
     layer.group = None
 *
 *****************************************************************************/

#ifdef __cplusplus
%typemap(memberin) char * {
  if ($1) delete [] $1;
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(memberin,warning="451:Setting const char * member may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin) char * {
  if ($1) delete [] $1;
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin,warning="451:Setting const char * variable may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
#else
%typemap(memberin) char * {
  if ($1) free((char*)$1);
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(memberin,warning="451:Setting const char * member may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin) char * {
  if ($1) free((char*)$1);
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin,warning="451:Setting const char * variable may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
#endif // __cplusplus

// Python-specific module code included here
#ifdef SWIGPYTHON
%{
#include "pygdioctx/pygdioctx.h"
%}
%include "pymodule.i"
#endif // SWIGPYTHON

// Ruby-specific module code included here
#ifdef SWIGRUBY
%include "rbmodule.i"
#endif

// Next Generation class names
#ifdef NEXT_GENERATION_NAMES
%rename(Map) map_obj;
%rename(Layer) layer_obj;
%rename(Class) class_obj;
%rename(Style) styleObj;
%rename(Image) imageObj;
%rename(Point) pointObj;
%rename(Line) lineObj;
%rename(Shape) shapeObj;
%rename(OutputFormat) outputFormatObj;
%rename(Symbol) symbolObj;
%rename(Color) colorObj;
%rename(Rect) rectObj;
%rename(Projection) projectionObj;
%rename(Shapefile) shapefileObj;
%rename(SymbolSet) symbolSetObj;
%rename(FontSet) fontSetObj;
#endif

// grab mapserver declarations to wrap
%include "../../mapprimitive.h"
%include "../../mapshape.h"
%include "../../mapproject.h"
%include "../../map.h"

// try wrapping mapsymbol.h
%include "../../mapsymbol.h"

// wrap the errorObj and a few functions
%include "../../maperror.h"

%apply Pointer NONNULL { mapObj *map };
%apply Pointer NONNULL { layerObj *layer };

// Map zooming convenience methods included here
%include "../swiginc/mapzoom.i"

// Language-specific extensions to mapserver classes are included here

#ifdef SWIGPYTHON
%include "pyextend.i"
#endif //SWIGPYTHON

#ifdef SWIGRUBY
%include "rbextend.i"
#endif

// A few things neccessary for automatically wrapped functions
%newobject msGetErrorString;

//
// class extensions for errorObj
//
%extend errorObj {
  errorObj() {    
    return msGetErrorObj();
  }

  ~errorObj() {}
 
  errorObj *next() {
    errorObj *ep;	

    if(self == NULL || self->next == NULL) return NULL;

    ep = msGetErrorObj();
    while(ep != self) {
      // We reached end of list of active errorObj and didn't find the errorObj... this is bad!
      if(ep->next == NULL) return NULL;
      ep = ep->next;
    }
    
    return ep->next;
  }
}

// mapObj extensions have been moved to swiginc/map.i
%include "../swiginc/map.i"


/* Full support for symbols and addition of them to the map symbolset
   is done to resolve MapServer bug 579
   http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=579 */

%extend symbolObj {
    symbolObj(char *symbolname, const char *imagefile=NULL) {
        symbolObj *symbol;
        symbol = (symbolObj *) malloc(sizeof(symbolObj));
        initSymbol(symbol);
        symbol->name = strdup(symbolname);
        if (imagefile) {
            msLoadImageSymbol(symbol, imagefile);
        }
        return symbol;
    }

    ~symbolObj() {
        if (self->name) free(self->name);
        if (self->img) gdImageDestroy(self->img);
        if (self->font) free(self->font);
        if (self->imagepath) free(self->imagepath);
    }

    int setPoints(lineObj *line) {
        int i;
        for (i=0; i<line->numpoints; i++) {
            MS_COPYPOINT(&(self->points[i]), &(line->point[i]));
        }
        self->numpoints = line->numpoints;
        return self->numpoints;
    }

    %newobject getPoints;
    lineObj *getPoints() {
        int i;
        lineObj *line;
        line = (lineObj *) malloc(sizeof(lineObj));
        line->point = (pointObj *) malloc(sizeof(pointObj)*(self->numpoints));
        for (i=0; i<self->numpoints; i++) {
            line->point[i].x = self->points[i].x;
            line->point[i].y = self->points[i].y;
        }
        line->numpoints = self->numpoints;
        return line;
    }

    int setStyle(int index, int value) {
        if (index < 0 || index > MS_MAXSTYLELENGTH) {
            msSetError(MS_SYMERR, "Can't set style at index %d.", "setStyle()", index);
            return MS_FAILURE;
        }
        self->style[index] = value;
        return MS_SUCCESS;
    }

}

%extend symbolSetObj {

    symbolSetObj(const char *symbolfile=NULL) {
        symbolSetObj *symbolset;
        mapObj *temp_map=NULL;
        symbolset = (symbolSetObj *) malloc(sizeof(symbolSetObj));
        msInitSymbolSet(symbolset);
        if (symbolfile) {
            symbolset->filename = strdup(symbolfile);
            temp_map = msNewMapObj();
            msLoadSymbolSet(symbolset, temp_map);
            symbolset->map = NULL;
            msFreeMap(temp_map);
        }
        return symbolset;
    }
   
    ~symbolSetObj() {
        msFreeSymbolSet(self);
    }

    symbolObj *getSymbol(int i) {
        if(i >= 0 && i < self->numsymbols)	
            return (symbolObj *) &(self->symbol[i]);
        else
            return NULL;
    }

    symbolObj *getSymbolByName(char *symbolname) {
        int i;

        if (!symbolname) return NULL;

        i = msGetSymbolIndex(self, symbolname, MS_TRUE);
        if (i == -1)
            return NULL; // no such symbol
        else
            return (symbolObj *) &(self->symbol[i]);
    }

    int index(char *symbolname) {
        return msGetSymbolIndex(self, symbolname, MS_TRUE);
    }

    int appendSymbol(symbolObj *symbol) {
        return msAppendSymbol(self, symbol);
    }
 
    %newobject removeSymbol;
    symbolObj *removeSymbol(int index) {
        return (symbolObj *) msRemoveSymbol(self, index);
    }

    int save(const char *filename) {
        return msSaveSymbolSet(self, filename);
    }

}

// class extensions for layerObj, always within the context of a map,
// have been moved to swiginc/layer.i

%include "../swiginc/layer.i"
%include "../swiginc/class.i"
%include "../swiginc/style.i"

//
// class extensions for pointObj, useful many places
//
%extend pointObj {
  pointObj(double x=0.0, double y=0.0, double m=2e-38) {
      pointObj *p;
      p = (pointObj *)malloc(sizeof(pointObj));
      if (!p) return NULL;
      p->x = x;
      p->y = y;
      if (m > 1e-38)
          p->m = m;
      return p;
  }

  ~pointObj() {
    free(self);
  }

  int project(projectionObj *in, projectionObj *out) {
    return msProjectPoint(in, out, self);
  }	

  int draw(mapObj *map, layerObj *layer, imageObj *image, int classindex, char *text) {
    return msDrawPoint(map, layer, self, image, classindex, text);
  }

  double distanceToPoint(pointObj *point) {
    return msDistancePointToPoint(self, point);
  }

  double distanceToSegment(pointObj *a, pointObj *b) {
    return msDistancePointToSegment(self, a, b);
  }

  double distanceToShape(shapeObj *shape) {
    return msDistancePointToShape(self, shape);
  }

  int setXY(double x, double y, double m=2e-38) {
    self->x = x;
    self->y = y;
    if (m > 1e-38)
      self->m = m;
    return MS_SUCCESS;
  }
}

//
// class extensions for lineObj (eg. a line or group of points), useful many places
//
%extend lineObj {
  lineObj() {
    lineObj *line;

    line = (lineObj *)malloc(sizeof(lineObj));
    if(!line)
      return(NULL);

    line->numpoints=0;
    line->point=NULL;

    return line;
  }

  ~lineObj() {
    free(self->point);
    free(self);		
  }

  int project(projectionObj *in, projectionObj *out) {
    return msProjectLine(in, out, self);
  }

#ifdef NEXT_GENERATION_API
  pointObj *getPoint(int i) {
#else
  pointObj *get(int i) {
#endif
    if(i<0 || i>=self->numpoints)
      return NULL;
    else
      return &(self->point[i]);
  }

#ifdef NEXT_GENERATION_API
  int addPoint(pointObj *p) {
#else
  int add(pointObj *p) {
#endif
    if(self->numpoints == 0) { /* new */	
      self->point = (pointObj *)malloc(sizeof(pointObj));      
      if(!self->point)
	return MS_FAILURE;
    } else { /* extend array */
      self->point = (pointObj *)realloc(self->point, sizeof(pointObj)*(self->numpoints+1));
      if(!self->point)
	return MS_FAILURE;
    }

    self->point[self->numpoints].x = p->x;
    self->point[self->numpoints].y = p->y;
    self->numpoints++;

    return MS_SUCCESS;
  }

#ifdef NEXT_GENERATION_API
  int setPoint(int i, pointObj *p) {
#else
  int set(int i, pointObj *p) {
#endif
    if(i<0 || i>=self->numpoints) // invalid index
      return MS_FAILURE;

    self->point[i].x = p->x;
    self->point[i].y = p->y;
    return MS_SUCCESS;    
  }
}

//
// class extensions for shapeObj
//
%extend shapeObj {
  shapeObj(int type) {
    shapeObj *shape;

    shape = (shapeObj *)malloc(sizeof(shapeObj));
    if(!shape)
      return NULL;

    msInitShape(shape);
    if(type >= 0) shape->type = type;

    return shape;
  }

  ~shapeObj() {
    msFreeShape(self);
    free(self);		
  }

  int project(projectionObj *in, projectionObj *out) {
    return msProjectShape(in, out, self);
  }

#ifdef NEXT_GENERATION_API
  lineObj *getLine(int i) {
#else
  lineObj *get(int i) {
#endif
    if(i<0 || i>=self->numlines)
      return NULL;
    else
      return &(self->line[i]);
  }

#ifdef NEXT_GENERATION_API
  int addLine(lineObj *line) {
#else
  int add(lineObj *line) {
#endif
    return msAddLine(self, line);
  }

  int draw(mapObj *map, layerObj *layer, imageObj *image) {
    return msDrawShape(map, layer, self, image, MS_TRUE);
  }

  void setBounds() {    
    msComputeBounds(self);
    return;
  }

#ifdef NEXT_GENERATION_API
    %newobject copy;
    shapeObj *copy() {
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;
        msInitShape(shape);
        shape->type = self->type;
        msCopyShape(self, shape);
        return shape;
    }
#else
    int copy(shapeObj *dest) {
        return(msCopyShape(self, dest));
    }
#endif

  char *getValue(int i) { // returns an EXISTING value
    if(i >= 0 && i < self->numvalues)
      return (self->values[i]);
    else
      return NULL;
  }

  int contains(pointObj *point) {
    if(self->type == MS_SHAPE_POLYGON)
      return msIntersectPointPolygon(point, self);

    return -1;
  }

  double distanceToPoint(pointObj *point) {
    return msDistancePointToShape(point, self);
  }

  double distanceToShape(shapeObj *shape) {
    return msDistanceShapeToShape(self, shape);
  }

  int intersects(shapeObj *shape) {
    switch(self->type) {
    case(MS_SHAPE_LINE):
      switch(shape->type) {
      case(MS_SHAPE_LINE):
	return msIntersectPolylines(self, shape);
      case(MS_SHAPE_POLYGON):
	return msIntersectPolylinePolygon(self, shape);
      }
      break;
    case(MS_SHAPE_POLYGON):
      switch(shape->type) {
      case(MS_SHAPE_LINE):
	return msIntersectPolylinePolygon(shape, self);
      case(MS_SHAPE_POLYGON):
	return msIntersectPolygons(self, shape);
      }
      break;
    }

    return -1;
  }
}

//
// class extensions for rectObj
//
%extend rectObj {
  rectObj(double minx=0.0, double miny=0.0, double maxx=0.0, double maxy=0.0) {	
    rectObj *rect;
    
    // Check bounds
    if (minx > maxx || miny > maxy) {
        msSetError(MS_MISCERR, "Invalid bounds.", "rectObj()");
        return NULL;
    }
    
    rect = (rectObj *)calloc(1, sizeof(rectObj));
    if(!rect)
      return(NULL);
    
    rect->minx = minx;
    rect->miny = miny;
    rect->maxx = maxx;
    rect->maxy = maxy;

    return(rect);    	
  }

  ~rectObj() {
    free(self);
  }

  int project(projectionObj *in, projectionObj *out) {
    return msProjectRect(in, out, self);
  }

  double fit(int width, int height) {
    return  msAdjustExtent(self, width, height);
  } 

  /*
  int contrain(rectObj *bounds, double overlay) {
    return msConstrainRect(bounds,self, overlay);
  }
  */

  int draw(mapObj *map, layerObj *layer, imageObj *image, int classindex, char *text) {
    shapeObj shape;

    msInitShape(&shape);
    msRectToPolygon(*self, &shape);
    shape.classindex = classindex;
    shape.text = strdup(text);

    msDrawShape(map, layer, &shape, image, MS_TRUE);

    msFreeShape(&shape);
    
    return MS_SUCCESS;
  }

    %newobject toPolygon;
    shapeObj *toPolygon() {
        lineObj line = {0,NULL};
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;
        msInitShape(shape);
        shape->type = MS_SHAPE_POLYGON;
  
        line.point = (pointObj *)malloc(sizeof(pointObj)*5);
        line.point[0].x = self->minx;
        line.point[0].y = self->miny;
        line.point[1].x = self->minx;
        line.point[1].y = self->maxy;
        line.point[2].x = self->maxx;
        line.point[2].y = self->maxy;
        line.point[3].x = self->maxx;
        line.point[3].y = self->miny;
        line.point[4].x = line.point[0].x;
        line.point[4].y = line.point[0].y;
  
        line.numpoints = 5;
  
        msAddLine(shape, &line);
        msComputeBounds(shape);
        
        free(line.point);

        return shape;
    }
    
}

//
// class extensions for shapefileObj
//
%extend shapefileObj {
  shapefileObj(char *filename, int type) {    
    shapefileObj *shapefile;
    int status;

    shapefile = (shapefileObj *)malloc(sizeof(shapefileObj));
    if(!shapefile)
      return NULL;

    if(type == -1)
      status = msSHPOpenFile(shapefile, "rb", filename);
    else if(type == -2)
      status = msSHPOpenFile(shapefile, "rb+", filename);
    else
      status = msSHPCreateFile(shapefile, filename, type);

    if(status == -1) {
      msSHPCloseFile(shapefile);
      free(shapefile);
      return NULL;
    }
 
    return(shapefile);
  }

  ~shapefileObj() {
    msSHPCloseFile(self);
    free(self);  
  }

  int get(int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return MS_FAILURE;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);

    return MS_SUCCESS;
  }

  int getPoint(int i, pointObj *point) {
    if(i<0 || i>=self->numshapes)
      return MS_FAILURE;

    msSHPReadPoint(self->hSHP, i, point);
    return MS_SUCCESS;
  }

  int getTransformed(mapObj *map, int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return MS_FAILURE;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);
    msTransformShapeToPixel(shape, map->extent, map->cellsize);

    return MS_SUCCESS;
  }

  void getExtent(int i, rectObj *rect) {
    msSHPReadBounds(self->hSHP, i, rect);
  }

  int add(shapeObj *shape) {
    return msSHPWriteShape(self->hSHP, shape);	
  }	

  int addPoint(pointObj *point) {    
    return msSHPWritePoint(self->hSHP, point);	
  }
}

//
// class extensions for imageObj
//
//TODO : should take image type as argument ??
%extend imageObj {
   
    /* imageObj constructor now takes filename as an optional argument.
     * If the target language is Python, we ignore this constructor and
     * instead use the one in python/pymodule.i. */
#ifndef SWIGPYTHON
    imageObj(int width, int height, const char *driver=NULL,
             const char *file=NULL)
    {
        imageObj *image=NULL;
        outputFormatObj *format;

        if (file) {
            return (imageObj *) msImageLoadGD(file);
        }
        else {
            if (driver) {
                format = msCreateDefaultOutputFormat(NULL, driver);
            }
            else {
                format = msCreateDefaultOutputFormat(NULL, "GD/GIF");
                if (format == NULL)
                    format = msCreateDefaultOutputFormat(NULL, "GD/PNG");
                if (format == NULL)
                    format = msCreateDefaultOutputFormat(NULL, "GD/JPEG");
                if (format == NULL)
                    format = msCreateDefaultOutputFormat(NULL, "GD/WBMP");
            }
            if (format == NULL) {
                msSetError(MS_IMGERR, "Could not create output format %s",
                           "imageObj()", driver);
                return NULL;
            }
            image = msImageCreate(width, height, format, NULL, NULL);
            return image;
        }
    }
#endif // SWIGPYTHON

  ~imageObj() {
    msFreeImage(self);    
  }

  void free() {
    msFreeImage(self);    
  }

    /* saveGeo - see Bugzilla issue 549 */ 
    void save(char *filename, mapObj *map=NULL) {
        msSaveImage(map, self, filename );
    }

  // Method saveToString renders the imageObj into image data and returns
  // it as a string. Questions and comments to Sean Gillies <sgillies@frii.com>

#if defined SWIGTCL8

  Tcl_Obj *saveToString() {

    unsigned char *imgbytes;
    int size;
    Tcl_Obj *imgstring;

#if GD2_VERS > 1
    if(self->format->imagemode == MS_IMAGEMODE_RGBA) {
      gdImageSaveAlpha(self->img.gd, 1);
    } else if (self->format->imagemode == MS_IMAGEMODE_RGB) {
      gdImageSaveAlpha(self->img.gd, 0);
    }
#endif 

    if(strcasecmp("ON", msGetOutputFormatOption(self->format, "INTERLACE", "ON" )) == 0) {
      gdImageInterlace(self->img.gd, 1);
    }

    if(self->format->transparent) {
      gdImageColorTransparent(self->img.gd, 0);
    }

    if(strcasecmp(self->format->driver, "gd/gif") == 0) {

#ifdef USE_GD_GIF
      imgbytes = gdImageGifPtr(self->img.gd, &size);
#else
      msSetError(MS_MISCERR, "GIF output is not available.", "saveToString()");
      return(MS_FAILURE);
#endif

    } else if (strcasecmp(self->format->driver, "gd/png") == 0) {

#ifdef USE_GD_PNG
      imgbytes = gdImagePngPtr(self->img.gd, &size);
#else
      msSetError(MS_MISCERR, "PNG output is not available.", "saveToString()");
      return(MS_FAILURE);
#endif

    } else if (strcasecmp(self->format->driver, "gd/jpeg") == 0) {

#ifdef USE_GD_JPEG
       imgbytes = gdImageJpegPtr(self->img.gd, &size, atoi(msGetOutputFormatOption(self->format, "QUALITY", "75" )));
#else
       msSetError(MS_MISCERR, "JPEG output is not available.", "saveToString()");
       return(MS_FAILURE);
#endif

    } else if (strcasecmp(self->format->driver, "gd/wbmp") == 0) {

#ifdef USE_GD_WBMP
       imgbytes = gdImageWBMPPtr(self->img.gd, &size, 1);
#else
       msSetError(MS_MISCERR, "WBMP output is not available.", "saveToString()");
       return(MS_FAILURE);
#endif
        
    } else {
       msSetError(MS_MISCERR, "Unknown output image type driver: %s.", "saveToString()", self->format->driver );
       return(MS_FAILURE);
    } 

    // Tcl implementation to create string
    imgstring = Tcl_NewByteArrayObj(imgbytes, size);    
    
    gdFree(imgbytes); // The gd docs recommend gdFree()

    return imgstring;
  }
#endif

}

// 
// class extensions for outputFormatObj
//
%extend outputFormatObj {
    outputFormatObj(const char *driver, char *name=NULL) {
    outputFormatObj *format;

    format = msCreateDefaultOutputFormat(NULL, driver);
    if( format != NULL ) 
        format->refcount++;
    if (name != NULL)
        format->name = strdup(name);
    
    return format;
  }

  ~outputFormatObj() {
    if( --self->refcount < 1 )
      msFreeOutputFormat( self );
  }

  void setExtension( const char *extension ) {
    msFree( self->extension );
    self->extension = strdup(extension);
  }

  void setMimetype( const char *mimetype ) {
    msFree( self->mimetype );
    self->mimetype = strdup(mimetype);
  }

  void setOption( const char *key, const char *value ) {
    msSetOutputFormatOption( self, key, value );
  }

    %newobject getOption;
    char *getOption(const char *key, const char *value="") {
        const char *retval;
        retval = msGetOutputFormatOption(self, key, value);
        return strdup(retval);
    }
}

//
// class extensions for projectionObj
//
%extend projectionObj {
  projectionObj(char *string) {
    int status;
    projectionObj *proj=NULL;

    proj = (projectionObj *)malloc(sizeof(projectionObj));
    if(!proj) return NULL;
    msInitProjection(proj);

    status = msLoadProjectionString(proj, string);
    if(status == -1) {
      msFreeProjection(proj);
      free(proj);
      return NULL;
    }

    return proj;
  }

  ~projectionObj() {
    msFreeProjection(self);
    free(self);		
  }
}


//
// class extensions for labelCacheObj - TP mods
//
%extend labelCacheObj {
  void freeCache() {
    msFreeLabelCache(self);    
  }
}

//
// class extensions for DBFInfo - TP mods
//
%extend DBFInfo {
    char *getFieldName(int iField) {
        static char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pszFieldName;
    }

    int getFieldWidth(int iField) {
        char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pnWidth;
    }

    int getFieldDecimals(int iField) {
        char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pnDecimals;
    }

    int getFieldType(int iField) {
	return msDBFGetFieldInfo(self, iField, NULL, NULL, NULL);
    }    
}

// extension to colorObj

%extend colorObj {
  
    colorObj(int red=0, int green=0, int blue=0, int pen=MS_PEN_UNSET) {
        colorObj *color;
        
        // Check colors
        if (red > 255 || green > 255 || blue > 255) {
            msSetError(MS_MISCERR, "Invalid color index.", "colorObj()");
            return NULL;
        }
    
        color = (colorObj *)calloc(1, sizeof(colorObj));
        if (!color)
            return(NULL);
    
        MS_INIT_COLOR(*color, red, green, blue);

        return(color);    	
    }

    ~colorObj() {
        free(self);
    }
 
    int setRGB(int red, int green, int blue) {
        // Check colors
        if (red > 255 || green > 255 || blue > 255) {
            msSetError(MS_MISCERR, "Invalid color index.", "setRGB()");
            return MS_FAILURE;
        }
    
        MS_INIT_COLOR(*self, red, green, blue);
        return MS_SUCCESS;
    }

    int setHex(char *psHexColor) {
        int red, green, blue;
        if (psHexColor && strlen(psHexColor)== 7 && psHexColor[0] == '#') {
            red = hex2int(psHexColor+1);
            green = hex2int(psHexColor+3);
            blue= hex2int(psHexColor+5);
            if (red > 255 || green > 255 || blue > 255) {
                msSetError(MS_MISCERR, "Invalid color index.", "setHex()");
                return MS_FAILURE;
            }

            MS_INIT_COLOR(*self, red, green, blue);
            return MS_SUCCESS;
        }
        else {
            msSetError(MS_MISCERR, "Invalid hex color.", "setHex()");
            return MS_FAILURE;
        }
    }   
    
    %newobject toHex;
    char *toHex() {
        char hexcolor[7];
        sprintf(hexcolor, "#%02x%02x%02x", self->red, self->green, self->blue);
        return strdup(hexcolor);
    }
}

// OWSRequest
%include "../swiginc/owsrequest.i"

// HashTable
%include "../swiginc/hashtable.i"

