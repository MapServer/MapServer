/* $Id$ */

%extend rectObj {

    rectObj(double minx=-1.0, double miny=-1.0, 
            double maxx=-1.0, double maxy=-1.0) 
    {	
        rectObj *rect;
    
        // Check bounds
        if (minx > maxx || miny > maxy) {
            msSetError(MS_MISCERR, "Invalid bounds.", "rectObj()");
            return NULL;
        }
    
        rect = (rectObj *)calloc(1, sizeof(rectObj));
        if (!rect)
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

    int draw(mapObj *map, layerObj *layer, imageObj *image, 
             int classindex, char *text) 
    {
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
    shapeObj *toPolygon() 
    {
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

    /* to be completed after hobu works up a msRectIsValid function
    int isValid()
    {
        return MS_VALID_EXTENT(self);
    }
    */
    
}

