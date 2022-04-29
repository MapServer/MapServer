#ifndef FLATGEOBUF_GEOMETRYREADER_H
#define FLATGEOBUF_GEOMETRYREADER_H

#include "../../mapserver.h"
#include "../../maperror.h"
#include "../../mapprimitive.h"

#include "feature_generated.h"

namespace FlatGeobuf {

class GeometryReader {
    private:
        const FlatGeobuf::Geometry *m_geometry;
        FlatGeobuf::GeometryType m_geometry_type;
        bool m_has_z;
        bool m_has_m;

        uint32_t m_length = 0;
        uint32_t m_offset = 0;

        shapeObj *readPoint();
        shapeObj *readMultiPoint();
        shapeObj *readLineString();
        shapeObj *readMultiLineString();
        shapeObj *readPolygon();
        shapeObj *readMultiPolygon();
        shapeObj *readGeometryCollection();

    public:
        GeometryReader(
            const FlatGeobuf::Geometry *geometry,
            FlatGeobuf::GeometryType geometry_type,
            bool has_z,
            bool has_m) :
            m_geometry (geometry),
            m_geometry_type (geometry_type),
            m_has_z (has_z),
            m_has_m (has_m)
            { }
        shapeObj *read();
};

}


#endif /* FLATGEOBUF_GEOMETRYREADER_H */