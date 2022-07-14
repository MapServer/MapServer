#include "mapserver.h"
#include <include/core/SkBitmap.h>

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
    SkBitmap *bm = new SkBitmap();
    image->img.plugin = (void*) bm;
  } else {
    msSetError(MS_RENDERERERR, "Cannot create Skia bitmap of size %dx%d.",
               "skia2CreateImage()", width, height);
  }

  return image;
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

  renderer->createImage = &skia2CreateImage;

  return MS_SUCCESS;
}