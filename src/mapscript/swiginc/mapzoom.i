/******************************************************************************
 * Project:  MapServer
 * Purpose:  Map zooming convenience methods for MapScript
 * Author:   Sean Gillies, sgillies@frii.com
 *
 ******************************************************************************
 *
 * These functions are adapted from the code in php_mapscript.c.
 *
 *****************************************************************************/

%extend mapObj {

    /**
    Zoom by the given factor to a pixel position within the width
    and height bounds. If max_extent is not NULL, the zoom is 
    constrained to the max_extents

    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`

    */
    int zoomPoint(int zoomfactor, pointObj *poPixPos, int width, int height, rectObj *poGeorefExt, rectObj *poMaxGeorefExt)
    {
        double      dfGeoPosX, dfGeoPosY;
        double      dfDeltaX, dfDeltaY;
        rectObj     oNewGeorefExt;    
        double      dfNewScale;
        int         bMaxExtSet;
        double      dfDeltaExt;
        double dX, dY;
        
        dfNewScale = 0.0;
        bMaxExtSet = 0;
        dfDeltaExt = -1.0;

        if (poMaxGeorefExt != NULL) { bMaxExtSet = 1; }

        /* ----------------------------------------------------------- */
        /*      check the validity of the parameters.                  */
        /* ----------------------------------------------------------- */
        if (zoomfactor == 0 || width <= 0 || height <= 0 || poGeorefExt == NULL || poPixPos == NULL) {
            msSetError(MS_MISCERR, "Incorrect arguments", "mapscript::mapObj::zoomPoint()");
            return MS_FAILURE;
        }

        /* ----------------------------------------------------------- */
        /*      check if the values passed are consistent min > max.   */
        /* ----------------------------------------------------------- */
        if (poGeorefExt->minx >= poGeorefExt->maxx)  {
            msSetError(MS_MISCERR, "Georeferenced coordinates minx >= maxx",  "mapscript::mapObj::zoomPoint()");
            return MS_FAILURE;
        }
        if (poGeorefExt->miny >= poGeorefExt->maxy)  {
            msSetError(MS_MISCERR, "Georeferenced coordinates miny >= maxy", "mapscript::mapObj::zoomPoint()");
            return MS_FAILURE;
        }

        if (bMaxExtSet == 1) {
            if (poMaxGeorefExt->minx >= poMaxGeorefExt->maxx) {
                msSetError(MS_MISCERR, "Max Georeferenced coordinates minx >= maxx", "mapscript::mapObj::zoomPoint()");
                return MS_FAILURE;
            }
            if (poMaxGeorefExt->miny >= poMaxGeorefExt->maxy) {
                msSetError(MS_MISCERR, "Max Georeferenced coordinates miny >= maxy", "mapscript::mapObj::zoomPoint()");
            }
        }
   
        dfDeltaX = poGeorefExt->maxx - poGeorefExt->minx;
        dfDeltaY = poGeorefExt->maxy - poGeorefExt->miny;
        dX = dfDeltaX/((double)width);
        dY = dfDeltaY/((double)height);
        dfGeoPosX = poGeorefExt->minx + dX * (double)poPixPos->x;
        dfGeoPosY = poGeorefExt->maxy - dY * (double)poPixPos->y;

        if (self->gt.rotation_angle != 0) {
            dfGeoPosX = self->gt.geotransform[0] + self->gt.geotransform[1] * (double)poPixPos->x + self->gt.geotransform[2] * (double)poPixPos->y;
            dfGeoPosY = self->gt.geotransform[3] + self->gt.geotransform[4] * (double)poPixPos->x + self->gt.geotransform[5] * (double)poPixPos->y;
        }
        
        /* --- -------------------------------------------------------- */
        /*      zoom in                                                 */
        /* ------------------------------------------------------------ */
        if (zoomfactor > 1) {
            oNewGeorefExt.minx = dfGeoPosX - (dfDeltaX/(2*zoomfactor));        
            oNewGeorefExt.miny = dfGeoPosY - (dfDeltaY/(2*zoomfactor));        
            oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaX/(2*zoomfactor));        
            oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaY/(2*zoomfactor));
        }

        if (zoomfactor < 0) {
            oNewGeorefExt.minx = dfGeoPosX - (dfDeltaX/2)*(abs(zoomfactor));    
            oNewGeorefExt.miny = dfGeoPosY - (dfDeltaY/2)*(abs(zoomfactor));    
            oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaX/2)*(abs(zoomfactor));    
            oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaY/2)*(abs(zoomfactor));
        }

        if (zoomfactor == 1) {
            oNewGeorefExt.minx = dfGeoPosX - (dfDeltaX/2);
            oNewGeorefExt.miny = dfGeoPosY - (dfDeltaY/2);
            oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaX/2);
            oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaY/2);
        }

        /* ------------------------------------------------------------ */
        /*   if the min and max scale are set in the map file, we will  */
        /*   use them to test before zooming.                           */
        /* ------------------------------------------------------------ */
        msAdjustExtent(&oNewGeorefExt, self->width, self->height);
        msCalculateScale(oNewGeorefExt, self->units, self->width, self->height, self->resolution, &dfNewScale);
    
        if (self->web.maxscaledenom > 0) {
            if (zoomfactor < 0 && dfNewScale > self->web.maxscaledenom) {
                return MS_FAILURE;
            }
        }

        /* ============================================================ */
        /*  we do a special case for zoom in : we try to zoom as much as */
        /*  possible using the mincale set in the .map.                 */
        /* ============================================================ */
        if (self->web.minscaledenom > 0 && dfNewScale < self->web.minscaledenom && zoomfactor > 1) {
            /* To be consistent in swig mapscript and PHP mapscript, 
               use the same function to calculate the delta extents. */
            dfDeltaExt = GetDeltaExtentsUsingScale(self->web.minscaledenom, self->units, dfGeoPosY, 
                                                   self->width, self->resolution);

            if (dfDeltaExt > 0.0) {
                oNewGeorefExt.minx = dfGeoPosX - (dfDeltaExt/2);
                oNewGeorefExt.miny = dfGeoPosY - (dfDeltaExt/2);
                oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaExt/2);
                oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaExt/2);
            } else
                return MS_FAILURE;
        }

        /* ------------------------------------------------------------ */
        /*  If the buffer is set, make sure that the extents do not go  */
        /*  beyond the buffer.                                          */
        /* ------------------------------------------------------------ */
        if (bMaxExtSet) {
            dfDeltaX = oNewGeorefExt.maxx - oNewGeorefExt.minx;
            dfDeltaY = oNewGeorefExt.maxy - oNewGeorefExt.miny;
        
            /* Make sure Current georef extents is not bigger than 
             * max extents */
            if (dfDeltaX > (poMaxGeorefExt->maxx-poMaxGeorefExt->minx))
                dfDeltaX = poMaxGeorefExt->maxx-poMaxGeorefExt->minx;
            if (dfDeltaY > (poMaxGeorefExt->maxy-poMaxGeorefExt->miny))
                dfDeltaY = poMaxGeorefExt->maxy-poMaxGeorefExt->miny;

            if (oNewGeorefExt.minx < poMaxGeorefExt->minx) {
                oNewGeorefExt.minx = poMaxGeorefExt->minx;
                oNewGeorefExt.maxx =  oNewGeorefExt.minx + dfDeltaX;
            }
            if (oNewGeorefExt.maxx > poMaxGeorefExt->maxx) {
                oNewGeorefExt.maxx = poMaxGeorefExt->maxx;
                oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
            }
            if (oNewGeorefExt.miny < poMaxGeorefExt->miny) {
                oNewGeorefExt.miny = poMaxGeorefExt->miny;
                oNewGeorefExt.maxy =  oNewGeorefExt.miny + dfDeltaY;
            }
            if (oNewGeorefExt.maxy > poMaxGeorefExt->maxy) {
                oNewGeorefExt.maxy = poMaxGeorefExt->maxy;
                oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
            }
        }
    
        /* ------------------------------------------------------------ */
        /*      set the map extents with new values.                    */
        /* ------------------------------------------------------------ */
        self->extent.minx = oNewGeorefExt.minx;
        self->extent.miny = oNewGeorefExt.miny;
        self->extent.maxx = oNewGeorefExt.maxx;
        self->extent.maxy = oNewGeorefExt.maxy;
    
        self->cellsize = msAdjustExtent(&(self->extent), self->width, self->height);      
        dfDeltaX = self->extent.maxx - self->extent.minx;
        dfDeltaY = self->extent.maxy - self->extent.miny; 

        if (bMaxExtSet) {
            if (self->extent.minx < poMaxGeorefExt->minx) {
                self->extent.minx = poMaxGeorefExt->minx;
                self->extent.maxx = self->extent.minx + dfDeltaX;
            }
            if (self->extent.maxx > poMaxGeorefExt->maxx) {
                self->extent.maxx = poMaxGeorefExt->maxx;
                oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
            }
            if (self->extent.miny < poMaxGeorefExt->miny) {
                self->extent.miny = poMaxGeorefExt->miny;
                self->extent.maxy =  self->extent.miny + dfDeltaY;
            }
            if (self->extent.maxy > poMaxGeorefExt->maxy) {
                self->extent.maxy = poMaxGeorefExt->maxy;
                oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
            }
        }
    
        msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &(self->scaledenom));

        return MS_SUCCESS;
    }

    /**
    Set the map extents to a given extents. 
    Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE` on error
    */
    int zoomRectangle(rectObj *poPixRect, int width, int height, rectObj *poGeorefExt, rectObj *poMaxGeorefExt)
    {
        double      dfDeltaX, dfDeltaY;
        rectObj     oNewGeorefExt;    
        double      dfNewScale;
        double      dfDeltaExt;
        double dfMiddleX, dfMiddleY;
        int bMaxExtSet;
        
        bMaxExtSet = 0;
        dfNewScale = 0.0;
        dfDeltaExt = -1.0;

        if (poMaxGeorefExt != NULL) bMaxExtSet = 1;

        /* ----------------------------------------------------------- */
        /*      check the validity of the parameters.                  */
        /* ----------------------------------------------------------- */
        if (poPixRect == 0 || width <= 0 || height <= 0 || poGeorefExt == NULL) {
            msSetError(MS_MISCERR, "Incorrect arguments", "mapscript::mapObj::zoomRectangle");
            return MS_FAILURE;
        }

        /* ----------------------------------------------------------- */
        /*      check if the values passed are consistent min <= max.   */
        /* ----------------------------------------------------------- */
        if (poPixRect->minx >= poPixRect->maxx) {
            msSetError(MS_MISCERR, "image rectangle minx >= maxx", "mapscript::mapObj::zoomRectangle()");
            return MS_FAILURE;
        }
        /* This is not a typo: "maxx >= minx". For historical reason, we
         * keep this as it is. See documentation for more info about this check. */
        if (poPixRect->maxy >= poPixRect->miny) {
            msSetError(MS_MISCERR, "image rectangle maxy >= miny", "mapscript::mapObj::zoomRectangle()");
            return MS_FAILURE;
        }

        if (poGeorefExt->minx >= poGeorefExt->maxx) {
            msSetError(MS_MISCERR, "Georeferenced coordinates minx >= maxx", "mapscript::mapObj::zoomRectangle()");
            return MS_FAILURE;
        }
        if (poGeorefExt->miny >= poGeorefExt->maxy) {
            msSetError(MS_MISCERR, "Georeferenced coordinates miny >= maxy", "mapscript::mapObj::zoomRectangle()");
            return MS_FAILURE;
        }

        if (bMaxExtSet == 1) {
            if (poMaxGeorefExt->minx >= poMaxGeorefExt->maxx) {
                msSetError(MS_MISCERR,  "Max Georeferenced coordinates minx >= maxx", "mapscript::mapObj::zoomRectangle()");
                return MS_FAILURE;
            }
            if (poMaxGeorefExt->miny >= poMaxGeorefExt->maxy) {
                msSetError(MS_MISCERR, "Max Georeferenced coordinates miny >= maxy", "mapscript::mapObj::zoomRectangle()");
                return MS_FAILURE;
            }
        }

  
        /* ----------------------------------------------------------- */
        /*   Convert pixel rectangle to georeferenced rectangle        */
        /* ----------------------------------------------------------- */
        
        dfDeltaX = poGeorefExt->maxx - poGeorefExt->minx;
        dfDeltaY = poGeorefExt->maxy - poGeorefExt->miny;

        oNewGeorefExt.minx = Pix2Georef((int)poPixRect->minx, 0, width, poGeorefExt->minx, poGeorefExt->maxx, 0); 
        oNewGeorefExt.maxx = Pix2Georef((int)poPixRect->maxx, 0, width, poGeorefExt->minx, poGeorefExt->maxx, 0); 
        oNewGeorefExt.miny = Pix2Georef((int)poPixRect->miny, 0, height, poGeorefExt->miny, poGeorefExt->maxy, 1); 
        oNewGeorefExt.maxy = Pix2Georef((int)poPixRect->maxy, 0, height, poGeorefExt->miny, poGeorefExt->maxy, 1); 

        msAdjustExtent(&oNewGeorefExt, self->width, self->height);

        /* ------------------------------------------------------------ */
        /*   if the min and max scale are set in the map file, we will  */
        /*   use them to test before setting extents.                   */
        /* ------------------------------------------------------------ */
        msCalculateScale(oNewGeorefExt, self->units, self->width, self->height, self->resolution, &dfNewScale);

        if (self->web.maxscaledenom > 0 &&  dfNewScale > self->web.maxscaledenom)
            return MS_FAILURE;

        if (self->web.minscaledenom > 0 && dfNewScale <  self->web.minscaledenom) {
            dfMiddleX = oNewGeorefExt.minx + ((oNewGeorefExt.maxx - oNewGeorefExt.minx)/2);
            dfMiddleY = oNewGeorefExt.miny + ((oNewGeorefExt.maxy - oNewGeorefExt.miny)/2);

            /* To be consistent in swig mapscript and PHP mapscript, 
               use the same function to calculate the delta extents. */
            dfDeltaExt = GetDeltaExtentsUsingScale(self->web.minscaledenom, self->units, dfMiddleY, self->width, self->resolution);

            if (dfDeltaExt > 0.0) {
                oNewGeorefExt.minx = dfMiddleX - (dfDeltaExt/2);
                oNewGeorefExt.miny = dfMiddleY - (dfDeltaExt/2);
                oNewGeorefExt.maxx = dfMiddleX + (dfDeltaExt/2);
                oNewGeorefExt.maxy = dfMiddleY + (dfDeltaExt/2);
            } else
                return MS_FAILURE;
        }

        /* ------------------------------------------------------------ */
        /*  If the buffer is set, make sure that the extents do not go  */
        /*  beyond the buffer.                                          */
        /* ------------------------------------------------------------ */
        if (bMaxExtSet) {
            dfDeltaX = oNewGeorefExt.maxx - oNewGeorefExt.minx;
            dfDeltaY = oNewGeorefExt.maxy - oNewGeorefExt.miny;
        
            /* Make sure Current georef extents is not bigger 
             * than max extents */
            if (dfDeltaX > (poMaxGeorefExt->maxx-poMaxGeorefExt->minx))
                dfDeltaX = poMaxGeorefExt->maxx-poMaxGeorefExt->minx;
            if (dfDeltaY > (poMaxGeorefExt->maxy-poMaxGeorefExt->miny))
                dfDeltaY = poMaxGeorefExt->maxy-poMaxGeorefExt->miny;

            if (oNewGeorefExt.minx < poMaxGeorefExt->minx) {
                oNewGeorefExt.minx = poMaxGeorefExt->minx;
                oNewGeorefExt.maxx =  oNewGeorefExt.minx + dfDeltaX;
            }
            if (oNewGeorefExt.maxx > poMaxGeorefExt->maxx) {
                oNewGeorefExt.maxx = poMaxGeorefExt->maxx;
                oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
            }
            if (oNewGeorefExt.miny < poMaxGeorefExt->miny) {
                oNewGeorefExt.miny = poMaxGeorefExt->miny;
                oNewGeorefExt.maxy =  oNewGeorefExt.miny + dfDeltaY;
            }
            if (oNewGeorefExt.maxy > poMaxGeorefExt->maxy) {
                oNewGeorefExt.maxy = poMaxGeorefExt->maxy;
                oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
            }
        }

        self->extent.minx = oNewGeorefExt.minx;
        self->extent.miny = oNewGeorefExt.miny;
        self->extent.maxx = oNewGeorefExt.maxx;
        self->extent.maxy = oNewGeorefExt.maxy;

        self->cellsize = msAdjustExtent(&(self->extent), self->width, self->height);    
        dfDeltaX = self->extent.maxx - self->extent.minx;
        dfDeltaY = self->extent.maxy - self->extent.miny; 

        if (bMaxExtSet) {
            if (self->extent.minx < poMaxGeorefExt->minx) {
                self->extent.minx = poMaxGeorefExt->minx;
                self->extent.maxx = self->extent.minx + dfDeltaX;
            }
            if (self->extent.maxx > poMaxGeorefExt->maxx) {
                self->extent.maxx = poMaxGeorefExt->maxx;
                oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
            }
            if (self->extent.miny < poMaxGeorefExt->miny) {
                self->extent.miny = poMaxGeorefExt->miny;
                self->extent.maxy =  self->extent.miny + dfDeltaY;
            }
            if (self->extent.maxy > poMaxGeorefExt->maxy) {
                self->extent.maxy = poMaxGeorefExt->maxy;
                oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
            }
        }

        msCalculateScale(self->extent, self->units, self->width,  self->height, self->resolution, &(self->scaledenom));

        return MS_SUCCESS;
    }

    /**
     Zoom by the given factor to a pixel position within the width
     and height bounds.  If max_extent is not NULL, the zoom is 
     constrained to the max_extents
    */
    int zoomScale(double scale, pointObj *poPixPos, int width, int height,
                  rectObj *poGeorefExt, rectObj *poMaxGeorefExt)
    {
        double      dfGeoPosX, dfGeoPosY;
        double      dfDeltaX, dfDeltaY;
        rectObj     oNewGeorefExt;    
        double      dfNewScale, dfCurrentScale;
        int         bMaxExtSet;
        int nTmp;
        double      dfDeltaExt;
        double dX, dY;
        
        dfNewScale = 0.0;
        dfCurrentScale = 0.0;
        nTmp = 0;
        bMaxExtSet = 0;
        dfDeltaExt = -1.0;

        if (poMaxGeorefExt != NULL) { bMaxExtSet = 1; }

        /* ----------------------------------------------------------- */
        /*      check the validity of the parameters.                  */
        /* ----------------------------------------------------------- */
        if (scale <= 0.0 || width <= 0 || height <= 0 || poGeorefExt == NULL || poPixPos == NULL ) {
            msSetError(MS_MISCERR, "Incorrect arguments", "mapscript::mapObj::zoomScale");
            return MS_FAILURE;
        }

        /* ----------------------------------------------------------- */
        /*      check if the values passed are consistent min > max.   */
        /* ----------------------------------------------------------- */
        if (poGeorefExt->minx >= poGeorefExt->maxx) {
            msSetError(MS_MISCERR, "Georeferenced coordinates minx >= maxx", "mapscript::mapObj::zoomScale()");
            return MS_FAILURE;
        }
        if (poGeorefExt->miny >= poGeorefExt->maxy) {
            msSetError(MS_MISCERR, "Georeferenced coordinates miny >= maxy", "mapscript::mapObj::zoomScale()");
            return MS_FAILURE;
        }

        if (bMaxExtSet == 1) {
            if (poMaxGeorefExt->minx >= poMaxGeorefExt->maxx) {
                msSetError(MS_MISCERR, "Max Georeferenced coordinates minx >= maxx", "mapscript::mapObj::zoomScale()");
                return MS_FAILURE;
            }
            if (poMaxGeorefExt->miny >= poMaxGeorefExt->maxy) {
                msSetError(MS_MISCERR,  "Max Georeferenced coordinates miny >= maxy", "mapscript::mapObj::zoomScale()");
            }
        }
   
        dfDeltaX = poGeorefExt->maxx - poGeorefExt->minx;
        dfDeltaY = poGeorefExt->maxy - poGeorefExt->miny;
        dX = dfDeltaX/((double)width);
        dY = dfDeltaY/((double)height);
        dfGeoPosX = poGeorefExt->minx + dX * (double)poPixPos->x;
        dfGeoPosY = poGeorefExt->maxy - dY * (double)poPixPos->y;

        if (self->gt.rotation_angle != 0) {
            dfGeoPosX = self->gt.geotransform[0] + self->gt.geotransform[1] * (double)poPixPos->x + self->gt.geotransform[2] * (double)poPixPos->y;
            dfGeoPosY = self->gt.geotransform[3] + self->gt.geotransform[4] * (double)poPixPos->x + self->gt.geotransform[5] * (double)poPixPos->y;
        }
        
        /* ------------------------------------------------------------ */
        /*  Calculate new extents based on the scale.                   */
        /* ------------------------------------------------------------ */

        /* ============================================================ */
        /*  make sure to take the smallest size because this is the one */
        /*  that will be used to ajust the scale.                       */
        /* ============================================================ */

        if (self->width <  self->height)
            nTmp = self->width;
        else
            nTmp = self->height;

        /* To be consistent in swig mapscript and PHP mapscript, 
           use the same function to calculate the delta extents. */
        dfDeltaExt = GetDeltaExtentsUsingScale(scale, self->units, dfGeoPosY, nTmp, self->resolution);

        if (dfDeltaExt > 0.0) {
            oNewGeorefExt.minx = dfGeoPosX - (dfDeltaExt/2);
            oNewGeorefExt.miny = dfGeoPosY - (dfDeltaExt/2);
            oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaExt/2);
            oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaExt/2);
        } else
            return MS_FAILURE;

        /* ------------------------------------------------------------ */
        /*   get current scale.                                         */
        /* ------------------------------------------------------------ */
        msCalculateScale(*poGeorefExt, self->units, self->width, self->height, self->resolution, &dfCurrentScale);

        /* ------------------------------------------------------------ *
         *   if the min and max scale are set in the map file, we will  *
         *   use them to test before zooming.                           *
         *                                                              *
         *   This function has the same effect as zoomin or zoom out.
         *   If the current scale is > newscale we zoom in; else it is
         *   a zoom out.
         * ------------------------------------------------------------ */
        msAdjustExtent(&oNewGeorefExt, self->width, self->height);
        msCalculateScale(oNewGeorefExt, self->units, self->width, self->height, self->resolution, &dfNewScale);

        if (self->web.maxscaledenom > 0) {
            if (dfCurrentScale < dfNewScale && dfNewScale >  self->web.maxscaledenom) {
                return MS_FAILURE;
            }
        }

        /* ============================================================ */
        /* we do a special case for zoom in : we try to zoom as much as */
        /* possible using the mincale set in the .map.                  */
        /* ============================================================ */
        if (self->web.minscaledenom > 0 && dfNewScale <  self->web.minscaledenom && dfCurrentScale > dfNewScale) {
            /* To be consistent in swig mapscript and PHP mapscript, 
               use the same function to calculate the delta extents. */
            dfDeltaExt = GetDeltaExtentsUsingScale(scale, self->units, dfGeoPosY, nTmp, self->resolution);
            if (dfDeltaExt > 0.0) {
                oNewGeorefExt.minx = dfGeoPosX - (dfDeltaExt/2);
                oNewGeorefExt.miny = dfGeoPosY - (dfDeltaExt/2);
                oNewGeorefExt.maxx = dfGeoPosX + (dfDeltaExt/2);
                oNewGeorefExt.maxy = dfGeoPosY + (dfDeltaExt/2);
            } else
                return MS_FAILURE;
        }

        /* ------------------------------------------------------------ */
        /*  If the buffer is set, make sure that the extents do not go  */
        /*  beyond the buffer.                                          */
        /* ------------------------------------------------------------ */
        if (bMaxExtSet) {
            dfDeltaX = oNewGeorefExt.maxx - oNewGeorefExt.minx;
            dfDeltaY = oNewGeorefExt.maxy - oNewGeorefExt.miny;
        
            /* Make sure Current georef extents is not bigger 
             * than max extents */
            if (dfDeltaX > (poMaxGeorefExt->maxx-poMaxGeorefExt->minx))
                dfDeltaX = poMaxGeorefExt->maxx-poMaxGeorefExt->minx;
            if (dfDeltaY > (poMaxGeorefExt->maxy-poMaxGeorefExt->miny))
                dfDeltaY = poMaxGeorefExt->maxy-poMaxGeorefExt->miny;

            if (oNewGeorefExt.minx < poMaxGeorefExt->minx) {
                oNewGeorefExt.minx = poMaxGeorefExt->minx;
                oNewGeorefExt.maxx =  oNewGeorefExt.minx + dfDeltaX;
            }
            if (oNewGeorefExt.maxx > poMaxGeorefExt->maxx) {
                oNewGeorefExt.maxx = poMaxGeorefExt->maxx;
                oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
            }
            if (oNewGeorefExt.miny < poMaxGeorefExt->miny) {
                oNewGeorefExt.miny = poMaxGeorefExt->miny;
                oNewGeorefExt.maxy =  oNewGeorefExt.miny + dfDeltaY;
            }
            if (oNewGeorefExt.maxy > poMaxGeorefExt->maxy) {
                oNewGeorefExt.maxy = poMaxGeorefExt->maxy;
                oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
            }
        }

        self->extent.minx = oNewGeorefExt.minx;
        self->extent.miny = oNewGeorefExt.miny;
        self->extent.maxx = oNewGeorefExt.maxx;
        self->extent.maxy = oNewGeorefExt.maxy;
    
        self->cellsize = msAdjustExtent(&(self->extent), self->width, self->height);    
        dfDeltaX = self->extent.maxx - self->extent.minx;
        dfDeltaY = self->extent.maxy - self->extent.miny; 

        if (bMaxExtSet) {
            if (self->extent.minx < poMaxGeorefExt->minx) {
                self->extent.minx = poMaxGeorefExt->minx;
                self->extent.maxx = self->extent.minx + dfDeltaX;
            }
            if (self->extent.maxx > poMaxGeorefExt->maxx) {
                self->extent.maxx = poMaxGeorefExt->maxx;
                oNewGeorefExt.minx = oNewGeorefExt.maxx - dfDeltaX;
            }
            if (self->extent.miny < poMaxGeorefExt->miny) {
                self->extent.miny = poMaxGeorefExt->miny;
                self->extent.maxy =  self->extent.miny + dfDeltaY;
            }
            if (self->extent.maxy > poMaxGeorefExt->maxy) {
                self->extent.maxy = poMaxGeorefExt->maxy;
                oNewGeorefExt.miny = oNewGeorefExt.maxy - dfDeltaY;
            }
        }

        msCalculateScale(self->extent, self->units, self->width, self->height, self->resolution, &(self->scaledenom));

        return MS_SUCCESS;
    }
}

