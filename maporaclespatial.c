#include "map.h"

#ifdef USE_ORACLESPATIAL

typedef struct {
  int tmp;
}
msOracleSpatialLayerInfo;

int msOracleSpatialLayerOpen(layerObj *layer)
{ 
  msSetError(MS_QUERYERR, "Function not implemented yet", "msOracleSpatialLayerOpen()");
  return(MS_FAILURE); 
}

int msOracleSpatialLayerClose(layerObj *layer)
{ 
  msSetError( MS_QUERYERR, "Function not implemented yet", "msOracleSpatialLayerClose()" );
  return(MS_FAILURE); 
}

int msOracleSpatialLayerWhichShapes(layerObj *layer, rectObj rect)
{ 
  msSetError(MS_QUERYERR, "Function not implemented yet", "msOracleSpatialLayerWhichShapes()");
  return(MS_FAILURE); 
}

int msOracleSpatialLayerNextShape(layerObj *layer, shapeObj *shape)
{ 
  msSetError( MS_QUERYERR, "Function not implemented yet", "msOracleSpatialLayerNextShape()");
  return(MS_FAILURE); 
}

int msOracleSpatialLayerGetItems(layerObj *layer)
{ 
  msSetError(MS_QUERYERR, "Function not implemented yet", "msOracleSpatialLayerGetItems()" );
  return(MS_FAILURE); 
}

int msOracleSpatialLayerGetShape( layerObj *layer, shapeObj *shape, long record )
{ 
  msSetError(MS_QUERYERR, "Function not implemented yet", "msOracleSpatialLayerGetShape()");
  return(MS_FAILURE); 
}

int msOracleSpatialLayerGetExtent(layerObj *layer, rectObj *extent)
{ 
  msSetError(MS_QUERYERR, "Function not implemented yet", "msOracleSpatialLayerGetExtent()");
  return(MS_FAILURE); 
}

int msOracleSpatialLayerInitItemInfo(layerObj *layer)
{ 
  msSetError(MS_QUERYERR, "Function not implemented yet", "msOracleSpatialLayerInitItemInfo()");
  return(MS_FAILURE); 
}

void msOracleSpatialLayerFreeItemInfo(layerObj *layer)
{ 
  msSetError(MS_QUERYERR, "Function not implemented yet", "msOracleSpatialLayerFreeItemInfo()"); 
}

int msOracleSpatialLayerGetAutoStyle( mapObj *map, layerObj *layer, classObj *c, int tile, long record )
{ 
  msSetError( MS_QUERYERR, "Function not implemented yet", "msLayerGetAutoStyle()" );
  return MS_FAILURE; 
}

#else // OracleSpatial "not-supported" procedures

int msOracleSpatialLayerOpen(layerObj *layer)
{ 
  msSetError(MS_QUERYERR, "OracleSpatial is not supported", "msOracleSpatialLayerOpen()");
  return(MS_FAILURE); 
}

int msOracleSpatialLayerClose(layerObj *layer)
{ 
  msSetError( MS_QUERYERR, "OracleSpatial is not supported", "msOracleSpatialLayerClose()" );
  return(MS_FAILURE); 
}

int msOracleSpatialLayerWhichShapes(layerObj *layer, rectObj rect)
{ 
  msSetError(MS_QUERYERR, "OracleSpatial is not supported", "msOracleSpatialLayerWhichShapes()");
  return(MS_FAILURE); 
}

int msOracleSpatialLayerNextShape(layerObj *layer, shapeObj *shape)
{ 
  msSetError( MS_QUERYERR, "OracleSpatial is not supported", "msOracleSpatialLayerNextShape()");
  return(MS_FAILURE); 
}

int msOracleSpatialLayerGetItems(layerObj *layer)
{ 
  msSetError(MS_QUERYERR, "OracleSpatial is not supported", "msOracleSpatialLayerGetItems()" );
  return(MS_FAILURE); 
}

int msOracleSpatialLayerGetShape( layerObj *layer, shapeObj *shape, long record )
{ 
  msSetError(MS_QUERYERR, "OracleSpatial is not supported", "msOracleSpatialLayerGetShape()");
  return(MS_FAILURE); 
}

int msOracleSpatialLayerGetExtent(layerObj *layer, rectObj *extent)
{ 
  msSetError(MS_QUERYERR, "OracleSpatial is not supported", "msOracleSpatialLayerGetExtent()");
  return(MS_FAILURE); 
}

int msOracleSpatialLayerInitItemInfo(layerObj *layer)
{ 
  msSetError(MS_QUERYERR, "OracleSpatial is not supported", "msOracleSpatialLayerInitItemInfo()");
  return(MS_FAILURE); 
}

void msOracleSpatialLayerFreeItemInfo(layerObj *layer)
{ 
  msSetError(MS_QUERYERR, "OracleSpatial is not supported", "msOracleSpatialLayerFreeItemInfo()"); 
}

int msOracleSpatialLayerGetAutoStyle( mapObj *map, layerObj *layer, classObj *c, int tile, long record )
{ 
  msSetError( MS_QUERYERR, "OracleSpatial is not supported", "msLayerGetAutoStyle()" );
  return MS_FAILURE; 
}

#endif
