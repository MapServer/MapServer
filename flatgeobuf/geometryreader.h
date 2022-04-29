#ifndef FLATGEOBUF_GEOMETRYREADER_H
#define FLATGEOBUF_GEOMETRYREADER_H

#include "../../mapserver.h"
#include "../../maperror.h"
#include "../../mapprimitive.h"

#include "flatgeobuf_c.h"
#include "feature_generated.h"

namespace FlatGeobuf {

class GeometryReader {
    private:
        flatgeobuf_ctx *m_ctx;
        const FlatGeobuf::Geometry *m_geometry;
        FlatGeobuf::GeometryType m_geometry_type;
        bool m_has_z;
        bool m_has_m;

        uint32_t m_length = 0;
        uint32_t m_offset = 0;

        void readPoint(shapeObj *);
        void readMultiPoint(shapeObj *);
        void readLineString(shapeObj *);
        void readMultiLineString(shapeObj *);
        void readPolygon(shapeObj *);
        void readMultiPolygon(shapeObj *);
        void readGeometryCollection(shapeObj *);

        void readLineObj(lineObj *line);

    public:
        GeometryReader(
            flatgeobuf_ctx *ctx,
            const FlatGeobuf::Geometry *geometry) :
            m_ctx (ctx),
            m_geometry (geometry),
            m_geometry_type ((FlatGeobuf::GeometryType) ctx->geometry_type),
            m_has_z (ctx->has_z),
            m_has_m (ctx->has_m)
            { }
        void read(shapeObj *);
};

}


#endif /* FLATGEOBUF_GEOMETRYREADER_H */