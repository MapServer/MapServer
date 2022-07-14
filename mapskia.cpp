#include "mapserver.h"

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

  return MS_SUCCESS;
}