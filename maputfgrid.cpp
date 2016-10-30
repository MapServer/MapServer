/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  UTFGrid rendering functions (using AGG)
 * Author:   Francois Desjarlais
 *
 ******************************************************************************
 * Copyright (c) 1996-2007 Regents of the University of Minnesota.
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
 *****************************************************************************/


#include "mapserver.h"
#include "maputfgrid.h"
#include "mapagg.h"
#include "renderers/agg/include/agg_rasterizer_scanline_aa.h"
#include "renderers/agg/include/agg_basics.h"
#include "renderers/agg/include/agg_renderer_scanline.h"
#include "renderers/agg/include/agg_scanline_bin.h"
#include "renderers/agg/include/agg_gamma_functions.h"
#include "renderers/agg/include/agg_conv_stroke.h"
#include "renderers/agg/include/agg_ellipse.h"

typedef mapserver::int32u band_type;
typedef mapserver::row_ptr_cache<band_type> rendering_buffer;
typedef pixfmt_utf<utfpix32, rendering_buffer> pixfmt_utf32;
typedef mapserver::renderer_base<pixfmt_utf32> renderer_base;
typedef mapserver::rasterizer_scanline_aa<> rasterizer_scanline;
typedef mapserver::renderer_scanline_bin_solid<renderer_base> renderer_scanline;

static utfpix32 UTF_WATER = utfpix32(32);

#define utfitem(c) utfpix32(c)

struct shapeData
{
  char *datavalues;
  char *itemvalue;
  band_type utfvalue;
  int serialid;
};

class lookupTable {
public:
  lookupTable()
  {
    table = (shapeData*) msSmallMalloc(sizeof(shapeData));
    table->datavalues = NULL;
    table->itemvalue = NULL;
    table->utfvalue = 0;
    table->serialid = 0;
    size = 1;
    counter = 0;
  }

  ~lookupTable()
  {
    int i;

    for(i=0; i<size; i++)
    {
      if(table[i].datavalues)
        msFree(table[i].datavalues);
      if(table[i].itemvalue)
        msFree(table[i].itemvalue);
    }
    msFree(table);
  }

  shapeData  *table;
  int size;
  int counter;
};

/*
 * UTFGrid specific polygon adaptor.
 */
class polygon_adaptor_utf:public polygon_adaptor
{
public:
  polygon_adaptor_utf(shapeObj *shape,int utfres_in):polygon_adaptor(shape)
  {
    utfresolution = utfres_in;
  }

  virtual unsigned vertex(double* x, double* y)
  {
    if(m_point < m_pend) {
      bool first = m_point == m_line->point;
      *x = m_point->x/utfresolution;
      *y = m_point->y/utfresolution;
      m_point++;
      return first ? mapserver::path_cmd_move_to : mapserver::path_cmd_line_to;
    }
    *x = *y = 0.0;
    if(!m_stop) {

      m_line++;
      if(m_line>=m_lend) {
        m_stop=true;
        return mapserver::path_cmd_end_poly;
      }

      m_point=m_line->point;
      m_pend=&(m_line->point[m_line->numpoints]);
      return mapserver::path_cmd_end_poly;
    }
    return mapserver::path_cmd_stop;
  }

private:
  int utfresolution;
};

/*
 * UTFGrid specific line adaptor.
 */
class line_adaptor_utf:public line_adaptor
{
public:
  line_adaptor_utf(shapeObj *shape,int utfres_in):line_adaptor(shape)
  {
    utfresolution = utfres_in;
  }

  virtual unsigned vertex(double* x, double* y)
  {
    if(m_point < m_pend) {
      bool first = m_point == m_line->point;
      *x = m_point->x/utfresolution;
      *y = m_point->y/utfresolution;
      m_point++;
      return first ? mapserver::path_cmd_move_to : mapserver::path_cmd_line_to;
    }
    m_line++;
    *x = *y = 0.0;
    if(m_line>=m_lend)
      return mapserver::path_cmd_stop;

    m_point=m_line->point;
    m_pend=&(m_line->point[m_line->numpoints]);

    return vertex(x,y);
  }

private:
  int utfresolution;
};

class UTFGridRenderer {
public:
  UTFGridRenderer()
  {
    stroke = NULL;
  }
  ~UTFGridRenderer()
  {
    if(stroke)
      delete stroke;
    delete data;
  }

  lookupTable *data;
  int utfresolution;
  int layerwatch;
  int renderlayer;
  int useutfitem;
  int useutfdata;
  int duplicates;
  band_type utfvalue;
  layerObj *utflayer;
  band_type *buffer;
  rendering_buffer m_rendering_buffer;
  pixfmt_utf32 m_pixel_format;
  renderer_base m_renderer_base;
  rasterizer_scanline m_rasterizer;
  renderer_scanline m_renderer_scanline;
  mapserver::scanline_bin sl_utf;
  mapserver::conv_stroke<line_adaptor_utf> *stroke;
};

#define UTFGRID_RENDERER(image) ((UTFGridRenderer*) (image)->img.plugin)

/*
 * Encode to avoid unavailable char in the JSON
 */
unsigned int encodeForRendering(unsigned int toencode)
{
  unsigned int encoded;
  encoded = toencode + 32;
  /* 34 => " */
  if(encoded >= 34) {
    encoded = encoded +1;
  }
  /* 92 => \ */
  if (encoded >= 92) {
    encoded = encoded +1;
  }
  return encoded;
}

/*
 * Decode value to have the initial one
 */
unsigned int decodeRendered(unsigned int todecode)
{
  unsigned int decoded;
  
  if(todecode >= 92)
    todecode --;

  if(todecode >= 34)
    todecode --;

  decoded = todecode-32;

  return decoded;
}

/*
 * Allocate more memory to the table if necessary.
 */
int growTable(lookupTable *data)
{
  int i;

  data->table = (shapeData*) msSmallRealloc(data->table,sizeof(*data->table)*data->size*2);
  data->size = data->size*2;

  for(i=data->counter; i<data->size; i++)
  {
    data->table[i].datavalues = NULL;
    data->table[i].itemvalue = NULL;
    data->table[i].utfvalue = 0;
    data->table[i].serialid = 0;
  }
  return MS_SUCCESS;
}

/*
 * Add the shapeObj UTFDATA and UTFITEM to the lookup table.
 */
band_type addToTable(UTFGridRenderer *r, shapeObj *p)
{
  band_type utfvalue;

  /* Looks for duplicates. */
  if(r->duplicates==0 && r->useutfitem==1) {
    int i;
    for(i=0; i<r->data->counter; i++) {
      if(!strcmp(p->values[r->utflayer->utfitemindex],r->data->table[i].itemvalue)) {
        /* Found a copy of the values in the table. */
        utfvalue = r->data->table[i].utfvalue;

        return utfvalue;
      }
    }
  }

  /* Grow size of table if necessary */
  if(r->data->size == r->data->counter)
    growTable(r->data);

  utfvalue = (r->data->counter+1);

  /* Simple operation so we don't have unavailable char in the JSON */
  utfvalue = encodeForRendering(utfvalue);

  /* Data added to the table */
  r->data->table[r->data->counter].datavalues = msEvalTextExpressionJSonEscape(&r->utflayer->utfdata, p);

  /* If UTFITEM is set in the mapfile we add its value to the table */
  if(r->useutfitem)
    r->data->table[r->data->counter].itemvalue =  msStrdup(p->values[r->utflayer->utfitemindex]);

  r->data->table[r->data->counter].serialid = r->data->counter+1;

  r->data->table[r->data->counter].utfvalue = utfvalue;

  r->data->counter++;

  return utfvalue;
}

/*
 * Use AGG to render any path.
 */
template<class vertex_source>
int utfgridRenderPath(imageObj *img, vertex_source &path)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);
  r->m_rasterizer.reset();
  r->m_rasterizer.filling_rule(mapserver::fill_even_odd);
  r->m_rasterizer.add_path(path);
  r->m_renderer_scanline.color(utfitem(r->utfvalue));
  mapserver::render_scanlines(r->m_rasterizer, r->sl_utf, r->m_renderer_scanline);
  return MS_SUCCESS;
}

/*
 * Initialize the renderer, create buffer, allocate memory.
 */
imageObj *utfgridCreateImage(int width, int height, outputFormatObj *format, colorObj * bg)
{
  UTFGridRenderer *r;
  r = new UTFGridRenderer;

  r->data = new lookupTable;

  r->utfresolution = atof(msGetOutputFormatOption(format, "UTFRESOLUTION", "4"));
  if(r->utfresolution < 1) {
    msSetError(MS_MISCERR, "UTFRESOLUTION smaller that 1 in the mapfile.", "utfgridCreateImage()");
    return NULL;
  }

  r->layerwatch = 0;

  r->renderlayer = 0;

  r->useutfitem = 0;

  r->useutfdata = 0;

  r->duplicates = EQUAL("true", msGetOutputFormatOption(format, "DUPLICATES", "true"));

  r->utfvalue = 0;

  r->buffer = (band_type*)msSmallMalloc(width/r->utfresolution * height/r->utfresolution * sizeof(band_type));

  /* AGG specific operations */
  r->m_rendering_buffer.attach(r->buffer, width/r->utfresolution, height/r->utfresolution, width/r->utfresolution);
  r->m_pixel_format.attach(r->m_rendering_buffer);
  r->m_renderer_base.attach(r->m_pixel_format);
  r->m_renderer_scanline.attach(r->m_renderer_base);
  r->m_renderer_base.clear(UTF_WATER);
  r->m_rasterizer.gamma(mapserver::gamma_none());

  r->utflayer = NULL;

  imageObj *image = NULL;
  image = (imageObj *) msSmallCalloc(1,sizeof(imageObj));
  image->img.plugin = (void*) r;

  return image;
}

/*
 * Free all the memory used by the renderer.
 */
int utfgridFreeImage(imageObj *img)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  msFree(r->buffer);
  delete r;

  img->img.plugin = NULL;

  return MS_SUCCESS;
}

/*
 * Update a character in the utfgrid.
*/

int utfgridUpdateChar(imageObj *img, band_type oldChar, band_type newChar)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);
  int i,bufferLength;

  bufferLength = (img->height/r->utfresolution) * (img->width/r->utfresolution);

  for(i=0;i<bufferLength;i++){
    if(r->buffer[i] == oldChar)
      r->buffer[i] = newChar;
  }

  return MS_SUCCESS;
}

/*
 * Remove unnecessary data that didn't made it to the final grid.
 */

int utfgridCleanData(imageObj *img)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);
  unsigned char* usedChar;
  int i,bufferLength,itemFound,dataCounter;
  shapeData* updatedData;
  band_type utfvalue;

  bufferLength = (img->height/r->utfresolution) * (img->width/r->utfresolution);

  usedChar =(unsigned char*) msSmallMalloc(r->data->counter*sizeof(unsigned char));

  for(i=0;i<r->data->counter;i++){
    usedChar[i]=0;
  }

  itemFound=0;

  for(i=0;i<bufferLength;i++)
  {
    if(decodeRendered(r->buffer[i]) != 0 && usedChar[decodeRendered(r->buffer[i])-1]==0)
    {
      itemFound++;
      usedChar[decodeRendered(r->buffer[i])-1] = 1;
    }
  }

  updatedData = (shapeData*) msSmallMalloc(itemFound * sizeof(shapeData));
  dataCounter = 0;

  for(i=0; i< r->data->counter; i++){
    if(usedChar[decodeRendered(r->data->table[i].utfvalue)-1]==1){
        updatedData[dataCounter] = r->data->table[i];

        updatedData[dataCounter].serialid=dataCounter+1;

        utfvalue=encodeForRendering(dataCounter+1);

        utfgridUpdateChar(img,updatedData[dataCounter].utfvalue,utfvalue);
        updatedData[dataCounter].utfvalue = utfvalue;

      dataCounter++;
    }
    else {
      if(r->data->table[i].datavalues)
        msFree(r->data->table[i].datavalues);
      if(r->data->table[i].itemvalue)
        msFree(r->data->table[i].itemvalue);
    }
  }

  msFree(usedChar);

  msFree(r->data->table);

  r->data->table = updatedData;
  r->data->counter = dataCounter;
  r->data->size = dataCounter;

  return MS_SUCCESS;
}

/*
 * Print the renderer data as JSON.
 */
int utfgridSaveImage(imageObj *img, mapObj *map, FILE *fp, outputFormatObj *format)
{
  int row, col, i, imgheight, imgwidth;
  band_type pixelid;
  char* pszEscaped;

  utfgridCleanData(img);

  UTFGridRenderer *renderer = UTFGRID_RENDERER(img);

  if(renderer->layerwatch>1)
    return MS_FAILURE;

  imgheight = img->height/renderer->utfresolution;
  imgwidth = img->width/renderer->utfresolution;

  msIO_fprintf(fp,"{\"grid\":[");

  /* Print the buffer */
  for(row=0; row<imgheight; row++) {

    wchar_t *string = (wchar_t*) msSmallMalloc ((imgwidth + 1) * sizeof(wchar_t));
    wchar_t *stringptr;
    stringptr = string;
    /* Need a comma between each line but JSON must not start with a comma. */
    if(row!=0)
      msIO_fprintf(fp,",");
    msIO_fprintf(fp,"\"");
    for(col=0; col<img->width/renderer->utfresolution; col++) {
      /* Get the data from buffer. */
      pixelid = renderer->buffer[(row*imgwidth)+col];

      *stringptr = pixelid;
      stringptr++;
    }

    /* Conversion to UTF-8 encoding */
    *stringptr = '\0';
    char * utf8;
#if defined(_WIN32) && !defined(__CYGWIN__)
	const char* encoding = "UCS-2LE";
#else
	const char* encoding = "UCS-4LE";
#endif

	  utf8 = msConvertWideStringToUTF8(string, encoding);
    msIO_fprintf(fp,"%s", utf8);
    msFree(utf8);
    msFree(string);
    msIO_fprintf(fp,"\"");
  }

  msIO_fprintf(fp,"],\"keys\":[\"\"");

  /* Print the specified key */
  for(i=0;i<renderer->data->counter;i++) {
      msIO_fprintf(fp,",");

    if(renderer->useutfitem)
    {
      pszEscaped = msEscapeJSonString(renderer->data->table[i].itemvalue);
      msIO_fprintf(fp,"\"%s\"", pszEscaped);
      msFree(pszEscaped);
    }
    /* If no UTFITEM specified use the serial ID as the key */
    else
      msIO_fprintf(fp,"\"%i\"", renderer->data->table[i].serialid);
  }

  msIO_fprintf(fp,"],\"data\":{");

  /* Print the data */
  if(renderer->useutfdata) {
    for(i=0;i<renderer->data->counter;i++) {
      if(i!=0)
        msIO_fprintf(fp,",");

      if(renderer->useutfitem)
      {
        pszEscaped = msEscapeJSonString(renderer->data->table[i].itemvalue);
        msIO_fprintf(fp,"\"%s\":", pszEscaped);
        msFree(pszEscaped);
      }
      /* If no UTFITEM specified use the serial ID as the key */
      else
        msIO_fprintf(fp,"\"%i\":", renderer->data->table[i].serialid);

      msIO_fprintf(fp,"%s", renderer->data->table[i].datavalues);
    }
  }
  msIO_fprintf(fp,"}}");

  return MS_SUCCESS;
}

/*
 * Starts a layer for UTFGrid renderer.
 */
int utfgridStartLayer(imageObj *img, mapObj *map, layerObj *layer)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* Look if the layer uses the UTFGrid output format */
  if(layer->utfdata.string!=0) {
    r->useutfdata = 1;
  }

    /* layerwatch is set to 1 on first layer treated. Doesn't allow multiple layers. */
    if(!r->layerwatch) {
      r->layerwatch++;

      r->renderlayer = 1;
      r->utflayer = layer;
      layer->refcount++;

      /* Verify if renderer needs to use UTFITEM */
      if(r->utflayer->utfitem)
        r->useutfitem = 1;
    }
    /* If multiple layers, send error */
    else {
      r->layerwatch++;
      msSetError(MS_MISCERR, "MapServer does not support multiple UTFGrid layers rendering simultaneously.", "utfgridStartLayer()");
      return MS_FAILURE;
    }

  return MS_SUCCESS;
}

/*
 * Tell renderer the layer is done.
 */
int utfgridEndLayer(imageObj *img, mapObj *map, layerObj *layer)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* Look if the layer was rendered, if it was then turn off rendering. */
  if(r->renderlayer) {
    r->utflayer = NULL;
    layer->refcount--;
    r->renderlayer = 0;
  }

  return MS_SUCCESS;
}

/*
 * Do the table operations on the shapes. Allow multiple types of data to be rendered.
 */
int utfgridStartShape(imageObj *img, shapeObj *shape)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  if(!r->renderlayer)
    return MS_FAILURE;

  /* Table operations */
  r->utfvalue = addToTable(r, shape);

  return MS_SUCCESS;
}

/*
 * Tells the renderer that the shape's rendering is done.
 */
int utfgridEndShape(imageObj *img, shapeObj *shape)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  r->utfvalue = 0;
  return MS_SUCCESS;
}

/*
 * Function that renders polygons into UTFGrid.
 */
int utfgridRenderPolygon(imageObj *img, shapeObj *polygonshape, colorObj *color)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if(r->utfvalue == 0) {
    return MS_FAILURE;
  }

  /* Render the polygon */
  polygon_adaptor_utf polygons(polygonshape, r->utfresolution);
  utfgridRenderPath(img, polygons);

  return MS_SUCCESS;
}

/*
 * Function that renders lines into UTFGrid. Starts by looking if the line is a polygon
 * outline, draw it if it's not.
 */
int utfgridRenderLine(imageObj *img, shapeObj *lineshape, strokeStyleObj *linestyle)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if(r->utfvalue == 0) {
    return MS_FAILURE;
  }

  /* Render the line */
  line_adaptor_utf lines(lineshape, r->utfresolution);

  if(!r->stroke) {
    r->stroke = new mapserver::conv_stroke<line_adaptor_utf>(lines);
  } else {
    r->stroke->attach(lines);
  }
  r->stroke->width(linestyle->width/r->utfresolution);
  utfgridRenderPath(img, *r->stroke);

  return MS_SUCCESS;
}

/*
 * Function that render vector type symbols into UTFGrid.
 */
int utfgridRenderVectorSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj * style)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);
  double ox = symbol->sizex * 0.5;
  double oy = symbol->sizey * 0.5;

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if(r->utfvalue == 0) {
    return MS_FAILURE;
  }

  /* Pathing the symbol */
  mapserver::path_storage path = imageVectorSymbol(symbol);

  /* Transformation to the right size/scale. */
  mapserver::trans_affine mtx;
  mtx *= mapserver::trans_affine_translation(-ox,-oy);
  mtx *= mapserver::trans_affine_scaling(style->scale/r->utfresolution);
  mtx *= mapserver::trans_affine_rotation(-style->rotation);
  mtx *= mapserver::trans_affine_translation(x/r->utfresolution, y/r->utfresolution);
  path.transform(mtx);

  /* Rendering the symbol. */
  utfgridRenderPath(img, path);

  return MS_SUCCESS;
}

/*
 * Function that renders Pixmap type symbols into UTFGrid.
 */
int utfgridRenderPixmapSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj * style)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);
  rasterBufferObj *pixmap = symbol->pixmap_buffer;

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if(r->utfvalue == 0) {
    return MS_FAILURE;
  }

  /* Pathing the symbol BBox */
  mapserver::path_storage pixmap_bbox;
  double w, h;
  w = pixmap->width*style->scale/(2.0);
  h= pixmap->height*style->scale/(2.0);
  pixmap_bbox.move_to((x-w)/r->utfresolution,(y-h)/r->utfresolution);
  pixmap_bbox.line_to((x+w)/r->utfresolution,(y-h)/r->utfresolution);
  pixmap_bbox.line_to((x+w)/r->utfresolution,(y+h)/r->utfresolution);
  pixmap_bbox.line_to((x-w)/r->utfresolution,(y+h)/r->utfresolution);

  /* Rendering the symbol */
  utfgridRenderPath(img, pixmap_bbox);

  return MS_SUCCESS;
}

/*
 * Function that render ellipse type symbols into UTFGrid.
 */
int utfgridRenderEllipseSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj * style)
{
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if(r->utfvalue == 0) {
    return MS_FAILURE;
  }

  /* Pathing the symbol. */
  mapserver::path_storage path;
  mapserver::ellipse ellipse(x/r->utfresolution,y/r->utfresolution,symbol->sizex*style->scale/2/r->utfresolution,symbol->sizey*style->scale/2/r->utfresolution);
  path.concat_path(ellipse);

  /* Rotation if necessary. */
  if( style->rotation != 0) {
    mapserver::trans_affine mtx;
    mtx *= mapserver::trans_affine_translation(-x/r->utfresolution,-y/r->utfresolution);
    mtx *= mapserver::trans_affine_rotation(-style->rotation);
    mtx *= mapserver::trans_affine_translation(x/r->utfresolution,y/r->utfresolution);
    path.transform(mtx);
  }

  /* Rendering the symbol. */
  utfgridRenderPath(img, path);

  return MS_SUCCESS;
}

int utfgridRenderGlyphs(imageObj *img, textPathObj *tp, colorObj *c, colorObj *oc, int ow) {
   return MS_SUCCESS;
}

int utfgridFreeSymbol(symbolObj * symbol)
{
  return MS_SUCCESS;
}

/*
 * Add the necessary functions for UTFGrid to the renderer VTable.
 */
int msPopulateRendererVTableUTFGrid( rendererVTableObj *renderer )
{
  renderer->default_transform_mode = MS_TRANSFORM_SIMPLIFY;

  renderer->createImage = &utfgridCreateImage;
  renderer->freeImage = &utfgridFreeImage;
  renderer->saveImage = &utfgridSaveImage;

  renderer->startLayer = &utfgridStartLayer;
  renderer->endLayer = &utfgridEndLayer;

  renderer->startShape = &utfgridStartShape;
  renderer->endShape = &utfgridEndShape;

  renderer->renderPolygon = &utfgridRenderPolygon;
  renderer->renderGlyphs = &utfgridRenderGlyphs;
  renderer->renderLine = &utfgridRenderLine;
  renderer->renderVectorSymbol = &utfgridRenderVectorSymbol;
  renderer->renderPixmapSymbol = &utfgridRenderPixmapSymbol;
  renderer->renderEllipseSymbol = &utfgridRenderEllipseSymbol;

  renderer->freeSymbol = &utfgridFreeSymbol;

  renderer->loadImageFromFile = msLoadMSRasterBufferFromFile;

  return MS_SUCCESS;
}
