/******************************************************************************
 * $id: mapoglrenderer.h 7725 2011-04-09 15:56:58Z toby $
 *
 * Project:  MapServer
 * Purpose:  Various template processing functions.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2008 Regents of the University of Minnesota.
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

#ifndef OGL_H
#define OGL_H

#ifdef USE_OGL

#include "mapoglcontext.h"
#include "mapserver.h"

#include <vector>
#include <map>
#include <memory>

#include <FTGL/ftgl.h>
#include <FTGL/FTGLTextureFont.h>


class OglCache
{
public:
  ~OglCache() {
    glDeleteTextures(1, &texture);
  }
  GLuint texture;
  ms_uint32 width;
  ms_uint32 height;
  ms_uint32 patternDistance;
};

typedef OglCache* OglCachePtr;

class OglRenderer
{
public:
  OglRenderer(ms_uint32 width, ms_uint32 height, colorObj* color = NULL);
  virtual ~OglRenderer();

  void renderPolyline(shapeObj *p, colorObj *c, double width, int patternlength, double* pattern, int lineCap = MS_CJC_ROUND, int joinStyle = MS_CJC_ROUND, colorObj *outlinecolor = NULL, double outlinewidth = 0);
  void renderPolygon(shapeObj*, colorObj *color, colorObj *outlinecolor, double outlinewidth, const OglCachePtr& tile = OglCachePtr(), int lineCap=MS_CJC_ROUND, int joinStyle=MS_CJC_ROUND);
  void renderGlyphs(double x, double y, colorObj *color, colorObj *outlinecolor, double size, const char* font, char *thechars, double angle, colorObj *shadowcolor, double shdx, double shdy);
  void renderPixmap(symbolObj *symbol, double x, double y, double angle, double scale);
  void renderEllipse(double x, double y, double angle, double w, double h, colorObj *color, colorObj *outlinecolor, double outlinewidth);
  void renderVectorSymbol(double x, double y, symbolObj *symbol, double scale, double angle, colorObj *c, colorObj *oc, double ow);
  void renderTile(const OglCachePtr& tile, double x, double y, double angle);
  void renderPolylineTile(shapeObj *p, const OglCachePtr& tile);

  static bool getStringBBox(char *font, double size, char *string, rectObj *rect, double** advances);
  void setTransparency(double transparency);

  void readRasterBuffer(rasterBufferObj * rb);
  void drawRasterBuffer(rasterBufferObj *overlay, double opacity, int srcX, int srcY, int dstX, int dstY, int width, int height);
  static void initializeRasterBuffer(rasterBufferObj * rb, int width, int height, bool useAlpha);

  OglCachePtr getTexture();
  int getWidth() const {
    return width;
  }
  int getHeight() const {
    return height;
  }
  bool isValid() const {
    return valid && context->isValid();
  }
protected:
  OglCachePtr texture;

  ms_uint32 width;
  ms_uint32 height;
  double transparency;
  bool valid;

  GLint viewportX;
  GLint viewportY;
  GLsizei viewportWidth;
  GLsizei viewportHeight;

  typedef std::map<const char*,std::map<double,FTFont*> > fontCache_t;
  typedef std::map<symbolObj*,std::map<double,GLuint> > dashCache_t;

  static FTFont* getFTFont(const char* font, double size);
  bool loadLine(shapeObj* shape, double width, int patternlength, double *pattern);
  double drawQuad(pointObj *p1, pointObj *p2, double width, double tilelength = 0.0, double textureStart = 0.0);
  double drawTriangles(pointObj *p1, pointObj *p2, double width, double tilelength = 0.0, double textureStart = 0.0);
  void drawVectorLineStrip(symbolObj *symbol, double width);
  void drawFan(pointObj* center, pointObj* from, pointObj* to, int resolution);
  void createShapes();

  GLuint createTexture(ms_uint32 x, ms_uint32 y);

  void makeCurrent();
  void setColor(colorObj *color);

  GLUtesselator *tess;
  enum shapes_t { circle = 0};

  OglContext* context;

  static dashCache_t dashCache;
  static fontCache_t fontCache;
  static std::vector<GLuint> shapes;
  static std::vector<symbolObj*> testSymbols;
  static ms_uint32 OUTLINE_WIDTH;
  static ms_uint32 FONT_SIZE;
  static ms_uint32 FONT_RES;
  static double OGL_PI;
  static ms_uint32 SHAPE_CIRCLE_RES;
  static double SHAPE_CIRCLE_RADIUS;
  static double SIZE_RES;
};

#endif /* USE_OGL */
#endif

