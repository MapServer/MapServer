/******************************************************************************
 * Project:  MapServer
 * Purpose:  Python-specific extensions to MapScript objects
 * Author:   Sean Gillies, sgillies@frii.com
 * Author:   Seth Girvin, sethg@geographika.co.uk
 *
 ******************************************************************************
 *
 * Python-specific mapscript code has been moved into this 
 * SWIG interface file to improve the readability of the main
 * interface file.  The main mapscript.i file includes this
 * file when SWIGPYTHON is defined (via 'swig -python ...').
 *
 *****************************************************************************/

// required for Python 3.10+ - see https://bugs.python.org/issue40943
%begin %{
#define PY_SSIZE_T_CLEAN
%}

/* fromstring: Factory for mapfile objects */

%pythoncode %{
def fromstring(data, mappath=None, configpath=None):
    """Creates map objects from mapfile strings.

    Parameters
    ==========
    data : string
        Mapfile in a string.
    mappath : string
        Optional root map path, enabling relative paths in mapfile.

    Example
    =======
    >>> mo = fromstring("MAP\nNAME 'test'\nEND")
    >>> mo.name
    'test'
    """
    import re
    if re.search(r"^\s*MAP", data, re.I): 
        # create a config object if a path is supplied
        if configpath:
             config = configObj(configpath):
        else:
             config = None
        return msLoadMapFromString(data, mappath, config)
    elif re.search(r"^\s*LAYER", data, re.I):
        ob = layerObj()
        ob.updateFromString(data)
        return ob
    elif re.search(r"^\s*CLASS", data, re.I):
        ob = classObj()
        ob.updateFromString(data)
        return ob
    elif re.search(r"^\s*STYLE", data, re.I):
        ob = styleObj()
        ob.updateFromString(data)
        return ob
    else:
        raise ValueError("No map, layer, class, or style found. Can not load from provided string")
%}


/* ===========================================================================
   Python rectObj extensions
   ======================================================================== */
   
%extend rectObj {

%pythoncode %{

    def __str__(self):
        return self.toString()

    def __contains__(self, item):
        item_type = item.__class__.__name__
        if item_type == "pointObj":
            if item.x >= self.minx and item.x <= self.maxx \
            and item.y >= self.miny and item.y <= self.maxy:
                return True
            else:
                return False
        else:
            raise TypeError('__contains__ does not yet handle %s' % (item_type))

%}
}

/* ===========================================================================
   Python pointObj extensions
   ======================================================================== */

%extend pointObj {

%pythoncode %{

    def __str__(self):
        return self.toString()

    @property
    def __geo_interface__(self):

        if hasattr(self, "z"):
            coords = (self.x, self.y, self.z)
        else:
            coords = (self.x, self.y)

        return {"type": "Point", "coordinates": coords}

%}
}


/* ===========================================================================
   Python lineObj extensions
   ======================================================================== */
   
%extend lineObj {

%pythoncode %{

    @property
    def __geo_interface__(self):

        coords = []

        for idx in range(0, self.numpoints):
            pt = self.get(idx)
            geom = pt.__geo_interface__
            coords.append(geom["coordinates"])

        return {"type": "LineString", "coordinates": coords}

%}
}


/* ===========================================================================
   Python shapeObj extensions
   ======================================================================== */

%extend shapeObj {

%pythoncode %{

        def _convert_item_values(self, property_values, property_types):
            """
            **Python MapScript only**

            Convert an item value, which is always stored as a string, into a
            Python type, based on an attributes GML metadata type. These can be one
            of the following:

            ``Integer|Long|Real|Character|Date|Boolean``
            """

            typed_values = []

            for value, type_ in zip(property_values, property_types):
                try:
                    if type_.lower() == "integer":
                        value = int(value)
                    elif type_.lower() == "long":
                        value = long(value)
                    elif type_.lower() == "real":
                        value = float(value)
                    else:
                        pass
                except ValueError:
                    pass

                typed_values.append(value)

            return typed_values

        @property
        def __geo_interface__(self):

            bounds = self.bounds
            ms_geom_type = self.type

            # see https://tools.ietf.org/html/rfc7946 for GeoJSON types

            if ms_geom_type == MS_SHAPE_POINT or ms_geom_type == MS_SHP_POINTZ or ms_geom_type == MS_SHP_POINTM:
                geom_type = "Point"
            elif ms_geom_type == MS_SHP_MULTIPOINTZ or ms_geom_type == MS_SHP_MULTIPOINTM:
                geom_type = "MultiPoint"
            elif ms_geom_type == MS_SHAPE_LINE or ms_geom_type == MS_SHP_ARCZ or ms_geom_type == MS_SHP_ARCM:
                if self.numlines == 1:
                    geom_type = "LineString"
                else:
                    geom_type = "MultiLineString"
            elif ms_geom_type == MS_SHAPE_POLYGON or ms_geom_type == MS_SHP_POLYGONZ or ms_geom_type == MS_SHP_POLYGONM:
                if self.numlines == 1:
                    geom_type = "Polygon"
                else:
                    geom_type = "MultiPolygon"
            elif ms_geom_type == MS_SHAPE_NULL:
                return None
            else:
                raise TypeError("Shape type {} not supported".format(geom_type))

            properties = {}
            coords = []

            # property names are stored at the layer level
            # https://github.com/mapserver/mapserver/issues/130

            property_values = [self.getValue(idx) for idx in range(0, self.numvalues)]

            if hasattr(self, "_item_definitions"):
                property_names, property_types = zip(*self._item_definitions)
                property_values = self._convert_item_values(property_values, property_types)
            else:
                property_names = [str(idx) for idx in range(0, self.numvalues)]

            properties = dict(zip(property_names, property_values))

            for idx in range(0, self.numlines):
                line = self.get(idx)
                geom = line.__geo_interface__
                coords.append(geom["coordinates"])

            return {
                    "type": "Feature",
                    "bbox": (bounds.minx, bounds.miny, bounds.maxx, bounds.maxy),
                    "properties": properties,
                    "geometry": {
                        "type": geom_type,
                        "coordinates": coords
                        }
                    }

        @property
        def itemdefinitions(self):
            return self._item_definitions

        @itemdefinitions.setter
        def itemdefinitions(self, item_definitions):
            self._item_definitions = item_definitions

%}
}

/******************************************************************************
 * Extensions to mapObj
 *****************************************************************************/

%extend mapObj {
  
    /**
    \**Python MapScript only**  - returns the map layer order as a native sequence
    */
    PyObject *getLayerOrder() {
        int i;
        PyObject *order;
        order = PyTuple_New(self->numlayers);
        for (i = 0; i < self->numlayers; i++) {
            PyTuple_SetItem(order,i,PyInt_FromLong((long)self->layerorder[i]));
        }
        return order;
    } 

    /**
    \**Python MapScript only** - sets the map layer order using a native sequence
    */
    int setLayerOrder(PyObject *order) {
        int i;
        Py_ssize_t size = PyTuple_Size(order);
        for (i = 0; i < size; i++) {
            self->layerorder[i] = (int)PyInt_AsLong(PyTuple_GetItem(order, i));
        }
        return MS_SUCCESS;
    }
    
    /**
    \**Python MapScript only** - gets the map size as a tuple
    */
    PyObject* getSize()
    {
        PyObject* output ;
        output = PyTuple_New(2);
        PyTuple_SetItem(output,0,PyInt_FromLong((long)self->width));
        PyTuple_SetItem(output,1,PyInt_FromLong((long)self->height));
        return output;
    }

%pythoncode %{

    def get_height(self):
        """
        **Python MapScript only**
        Return the map height from the map size
        """
        return self.getSize()[1] # <-- second member is the height

    def get_width(self):
        """
        **Python MapScript only**
        Return the map width from the map size
        """
        return self.getSize()[0] # <-- first member is the width

    def set_height(self, value):
        """
        **Python MapScript only**
        Set the map height value of the map size
        """
        return self.setSize(self.getSize()[0], value)

    def set_width(self, value):
        """
        **Python MapScript only**
        Set the map width value of the map size
        """
        return self.setSize(value, self.getSize()[1])

    width = property(get_width, set_width, doc="See :ref:`SIZE <mapfile-map-size>`")
    height = property(get_height, set_height, doc = "See :ref:`SIZE <mapfile-map-size>`")

%}
}

/******************************************************************************
 * Extensions to layerObj
 *****************************************************************************/

%extend layerObj {
  
%pythoncode %{

    def getItemDefinitions(self):
        """
        **Python MapScript only**
        
        Return item (field) names and their types if available.
        Field types are specified using GML metadata and can be one of the following:

        ``Integer|Long|Real|Character|Date|Boolean``

        """
        item_names = [self.getItem(idx) for idx in range(0, self.numitems)]
        item_types = [self.getItemType(idx) for idx in range(0, self.numitems)]
        return zip(item_names, item_types)

%}
}

/******************************************************************************
 * Extensions to imageObj
 *****************************************************************************/
    
%extend imageObj {

    /**
    Write image data to an open file handle. Replaces
    the removed saveToString function.  See ``python/pyextend.i`` for the Python specific
    version of this method.
    */
    int write( PyObject *file=Py_None )
    {
        unsigned char *imgbuffer=NULL;
        int imgsize;
        PyObject *noerr;
        int retval=MS_FAILURE;

        /* Return immediately if image driver is not GD */
        if ( !MS_RENDERER_PLUGIN(self->format) )
        {
            msSetError(MS_IMGERR, "Writing of %s format not implemented",
                       "imageObj::write", self->format->driver);
            return MS_FAILURE;
        }

        if (file == Py_None) /* write to stdout */
            retval = msSaveImage(NULL, self, NULL);
        else /* presume a Python file-like object */
        {
            imgbuffer = msSaveImageBuffer(self, &imgsize,
                                          self->format);
            if (imgsize == 0)
            {
                msSetError(MS_IMGERR, "failed to get image buffer", "write()");
                return MS_FAILURE;
            }

%#if PY_MAJOR_VERSION >= 3
            // https://docs.python.org/3/c-api/arg.html
            noerr = PyObject_CallMethod(file, "write", "y#", imgbuffer, (Py_ssize_t)imgsize);
%#else
            // https://docs.python.org/2/c-api/arg.html
            noerr = PyObject_CallMethod(file, "write", "s#", imgbuffer, imgsize);
%#endif

            free(imgbuffer);
            if (noerr == NULL)
                return MS_FAILURE;
            else
                Py_DECREF(noerr);
            retval = MS_SUCCESS;
        }

        return retval;
    }
}


/******************************************************************************
 * Extensions to styleObj
 *****************************************************************************/

%extend styleObj {

    /// **Python Only** Set the pattern for the style.
    void pattern_set(int nListSize, double* pListValues)
    {
        if( nListSize < 2 )
        {
            msSetError(MS_SYMERR, "Not enough pattern elements. A minimum of 2 are required", "pattern_set()");
            return;
        }
        if( nListSize > MS_MAXPATTERNLENGTH )
        {
            msSetError(MS_MISCERR, "Too many elements", "pattern_set()");
            return;
        }
        memcpy( self->pattern, pListValues, sizeof(double) * nListSize);
        self->patternlength = nListSize;
    }

    /// **Python Only** Get the pattern for the style.
    void pattern_get(double** argout, int* pnListSize)
    {
        *pnListSize = self->patternlength;
        *argout = (double*) malloc(sizeof(double) * *pnListSize);
        memcpy( *argout, self->pattern, sizeof(double) * *pnListSize);
    }


%pythoncode %{

pattern = property(pattern_get, pattern_set, doc=r"""pattern : list **Python Only**""")

%}
}

/******************************************************************************
 * Extensions to hashTableObj - add dict methods
 *****************************************************************************/

%extend hashTableObj{

    %pythoncode %{

    def __getitem__(self, key):
        return self.get(key)

    def __setitem__(self, key, value):
        return self.set(key, value)

    def __delitem__(self, key) :
        return self.remove(key)

    def __contains__(self, key):
        return key.lower() in [k.lower() for k in self.keys()]

    def __len__(self):
        return self.numitems

    def keys(self):
        """
        **Python-only**. In Python MapScript the ``hashTableObj`` can be used and accessed
        as a dictionary. The ``keys`` method returns a view of all the keys in the ``hashTableObj``.
        """

        keys = []
        k = None

        while True :
            k = self.nextKey(k)
            if k :
                keys.append(k)
            else :
                break

        return keys
            
%}
}
