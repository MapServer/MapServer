#include "geometryreader.h"

using namespace flatbuffers;
using namespace FlatGeobuf;

shapeObj *GeometryReader::readPoint()
{
    if (m_geometry->xy() == nullptr || m_geometry->xy()->size() == 0)
        return NULL;

    shapeObj *s = (shapeObj *) malloc(sizeof(shapeObj));
    lineObj *l = (lineObj *) malloc(sizeof(shapeObj));
    pointObj *p = (pointObj *) malloc(sizeof(pointObj));

	const auto xy = m_geometry->xy()->data();
	p->x = xy[m_offset + 0];
	p->y = xy[m_offset + 1];
    if (m_has_z)
        p->z = m_geometry->z()->data()[m_offset];
    if (m_has_m)
        p->m = m_geometry->m()->data()[m_offset];

    l[0].numpoints = 1;
    l[0].point = p;
    s->line = l;

    return s;
}

/*POINTARRAY *GeometryReader::readPA()
{
    POINTARRAY *pa;
	POINT4D pt;
	uint32_t npoints;

    const double *xy = m_geometry->xy()->data();
    const double *z = m_has_z ? m_geometry->z()->data() : nullptr;
    const double *m = m_has_m ? m_geometry->m()->data() : nullptr;

	pa = ptarray_construct_empty(m_has_z, m_has_m, m_length);

    for (uint32_t i = m_offset; i < m_offset + m_length; i++) {
        double xv = xy[i * 2 + 0];
        double yv = xy[i * 2 + 1];
        double zv = 0;
        double mv = 0;
        if (m_has_z)
            zv = z[i];
        if (m_has_m)
            mv = m[i];
        pt = (POINT4D) { xv, yv, zv, mv };
        ptarray_append_point(pa, &pt, LW_TRUE);
    }

	return pa;
}*/

shapeObj *GeometryReader::readMultiPoint()
{
    return NULL;
}

shapeObj *GeometryReader::readLineString()
{
    return NULL;
}

shapeObj *GeometryReader::readMultiLineString()
{
    return NULL;
    /*auto ends = m_geometry->ends();

    uint32_t ngeoms = 1;
    if (ends != nullptr && ends->size() > 1)
		ngeoms = ends->size();

    auto *lwmline = lwmline_construct_empty(0, m_has_z, m_has_m);
    if (ngeoms > 1) {
        for (uint32_t i = 0; i < ngeoms; i++) {
			const auto e = ends->Get(i);
			m_length = e - m_offset;
			POINTARRAY *pa = readPA();
			lwmline_add_lwline(lwmline, lwline_construct(0, NULL, pa));
			m_offset = e;
		}
    } else {
        POINTARRAY *pa = readPA();
		lwmline_add_lwline(lwmline, lwline_construct(0, NULL, pa));
    }

    return lwmline;*/
}

shapeObj *GeometryReader::readPolygon()
{
    return NULL;
    /*const auto ends = m_geometry->ends();

    uint32_t nrings = 1;
    if (ends != nullptr && ends->size() > 1)
        nrings = ends->size();

    auto **ppa = (POINTARRAY **) lwalloc(sizeof(POINTARRAY *) * nrings);
    if (nrings > 1) {
        for (uint32_t i = 0; i < nrings; i++) {
            const auto e = ends->Get(i);
            m_length = e - m_offset;
            ppa[i] = readPA();
            m_offset = e;
        }
    } else {
        ppa[0] = readPA();
    }

    return lwpoly_construct(0, NULL, nrings, ppa);*/
}

shapeObj *GeometryReader::readMultiPolygon()
{
    return NULL;
    /*
    auto parts = m_geometry->parts();
    auto *mp = lwmpoly_construct_empty(0, m_has_z, m_has_m);
    for (uoffset_t i = 0; i < parts->size(); i++) {
        GeometryReader reader { parts->Get(i), GeometryType::Polygon, m_has_z, m_has_m };
        const auto p = (LWPOLY *) reader.read();
        lwmpoly_add_lwpoly(mp, p);
    }
    return mp;*/
}

/*LWCOLLECTION *GeometryReader::readGeometryCollection()
{
    auto parts = m_geometry->parts();
    auto *gc = lwcollection_construct_empty(COLLECTIONTYPE, 0, m_has_z, m_has_m);
    for (uoffset_t i = 0; i < parts->size(); i++) {
        auto part = parts->Get(i);
        GeometryReader reader { part, part->type(), m_has_z, m_has_m };
        const auto g = reader.read();
        lwcollection_add_lwgeom(gc, g);
    }
    return gc;
}*/

shapeObj *GeometryReader::read()
{
    // nested types
    switch (m_geometry_type) {
        //case GeometryType::GeometryCollection: return (shapeObj *) readGeometryCollection();
        case GeometryType::MultiPolygon: return (shapeObj *) readMultiPolygon();
        /*case GeometryType::CompoundCurve: return readCompoundCurve();
        case GeometryType::CurvePolygon: return readCurvePolygon();
        case GeometryType::MultiCurve: return readMultiCurve();
        case GeometryType::MultiSurface: return readMultiSurface();
        case GeometryType::PolyhedralSurface: return readPolyhedralSurface();*/
        default: break;
    }

    // if not nested must have geometry data
    const auto pXy = m_geometry->xy();
    const auto xySize = pXy->size();
    m_length = xySize / 2;

    switch (m_geometry_type) {
        case GeometryType::Point: return (shapeObj *) readPoint();
        case GeometryType::MultiPoint: return (shapeObj *) readMultiPoint();
        case GeometryType::LineString: return (shapeObj *) readLineString();
        case GeometryType::MultiLineString: return (shapeObj *) readMultiLineString();
        case GeometryType::Polygon: return (shapeObj *) readPolygon();
        /*
        case GeometryType::CircularString: return readSimpleCurve<OGRCircularString>(true);
        case GeometryType::Triangle: return readTriangle();
        case GeometryType::TIN: return readTIN();
        */
        default:
            msSetError(MS_FGBERR, "Unknown type %d", "GeometryReader::read", (int) m_geometry_type);
    }
    return nullptr;
}