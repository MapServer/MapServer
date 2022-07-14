#include "mapserver.h"
#include <include/core/SkSurface.h>

int skia2StartNewLayer(imageObj *img, mapObj* /*map*/, layerObj *layer)
{
  return MS_SUCCESS;
}

int skia2CloseNewLayer(imageObj * /*img*/, mapObj * /*map*/, layerObj * /*layer*/)
{
  return MS_SUCCESS;
}

imageObj *skia2CreateImage(int width, int height, outputFormatObj *format, colorObj * bg)
{
  imageObj *image = NULL;
  if (format->imagemode != MS_IMAGEMODE_RGB && format->imagemode != MS_IMAGEMODE_RGBA) {
    msSetError(MS_MISCERR,
               "Skia driver only supports RGB or RGBA pixel models.", "skia2CreateImage()");
    return image;
  }
  if (width > 0 && height > 0) {
    image = (imageObj *) calloc(1, sizeof (imageObj));
    MS_CHECK_ALLOC(image, sizeof (imageObj), NULL);
    SkAlphaType alpha = format->transparent ? SkAlphaType::kPremul_SkAlphaType : SkAlphaType::kOpaque_SkAlphaType;
    SkImageInfo imageInfo = SkImageInfo::MakeN32(width, height, alpha);
    SkSurface *s = SkSurface::MakeRaster(imageInfo).get();
    image->img.plugin = (void*) s;
  } else {
    msSetError(MS_RENDERERERR, "Cannot create Skia surface of size %dx%d.",
               "skia2CreateImage()", width, height);
  }

  return image;
}

int skia2SaveImage(imageObj * /*img*/, mapObj* /*map*/, FILE * /*fp*/, outputFormatObj * /*format*/)
{
  return MS_FAILURE;
}

int skia2FreeImage(imageObj * image)
{
  SkSurface *s = (SkSurface *) image->img.plugin;
  delete s;
  image->img.plugin = NULL;
  return MS_SUCCESS;
}

int skia2RenderLine(imageObj *image, shapeObj *p, strokeStyleObj *style)
{
  SkSurface *s = (SkSurface *) image->img.plugin;
  SkCanvas *c = s->getCanvas();
  //c->drawPoints();
  return MS_SUCCESS;
}

int skia2RenderPolygon(imageObj *image, shapeObj *p, colorObj * color)
{
  SkSurface *s = (SkSurface *) image->img.plugin;
  SkCanvas *c = s->getCanvas();
  //c->drawPoints();
  return MS_SUCCESS;
}

int msPopulateRendererVTableSkia(rendererVTableObj * renderer)
{
  /*renderer->compositeRasterBuffer = &aggCompositeRasterBuffer;
  renderer->supports_pixel_buffer = 1;
  renderer->use_imagecache = 0;
  renderer->supports_clipping = 0;
  renderer->supports_svg = 0;
  renderer->default_transform_mode = MS_TRANSFORM_SIMPLIFY;
  agg2InitCache(&(MS_RENDERER_CACHE(renderer)));
  renderer->cleanup = agg2Cleanup;
  renderer->renderLine = &agg2RenderLine;

  renderer->renderPolygon = &agg2RenderPolygon;
  renderer->renderPolygonTiled = &agg2RenderPolygonTiled;
  renderer->renderLineTiled = &agg2RenderLineTiled;

  renderer->renderGlyphs = &agg2RenderGlyphsPath;

  renderer->renderVectorSymbol = &agg2RenderVectorSymbol;

  renderer->renderPixmapSymbol = &agg2RenderPixmapSymbol;

  renderer->renderEllipseSymbol = &agg2RenderEllipseSymbol;

  renderer->renderTile = &agg2RenderTile;

  renderer->getRasterBufferHandle = &aggGetRasterBufferHandle;
  renderer->getRasterBufferCopy = aggGetRasterBufferCopy;
  renderer->initializeRasterBuffer = aggInitializeRasterBuffer;

  renderer->mergeRasterBuffer = &agg2MergeRasterBuffer;
  renderer->loadImageFromFile = msLoadMSRasterBufferFromFile;
  renderer->createImage = &agg2CreateImage;
  renderer->saveImage = &agg2SaveImage;

  renderer->startLayer = &agg2StartNewLayer;
  renderer->endLayer = &agg2CloseNewLayer;

  renderer->freeImage = &agg2FreeImage;
  renderer->freeSymbol = &agg2FreeSymbol;*/

  renderer->renderLine = &skia2RenderLine;
  renderer->renderPolygon = &skia2RenderPolygon;

  renderer->startLayer = &skia2StartNewLayer;
  renderer->endLayer = &skia2CloseNewLayer;

  renderer->freeImage = &skia2FreeImage;
  renderer->createImage = &skia2CreateImage;
  renderer->saveImage = &skia2SaveImage;

  return MS_SUCCESS;
}