#include "geometryreader.h"

using namespace flatbuffers;
using namespace FlatGeobuf;

void GeometryReader::readPoint(shapeObj *shape)
{
    lineObj *l = (lineObj *) malloc(sizeof(lineObj));
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
    shape->numlines = 1;
    shape->line = l;
    shape->type = MS_SHAPE_POINT;
}

void GeometryReader::readLineObj(lineObj *line)
{
    const double *xy = m_geometry->xy()->data();
    const double *z = m_has_z ? m_geometry->z()->data() : nullptr;
    const double *m = m_has_m ? m_geometry->m()->data() : nullptr;

    line->point = (pointObj *) malloc(m_length * sizeof(pointObj));
    line->numpoints = m_length;

    for (uint32_t i = m_offset; i < m_offset + m_length; i++) {
        pointObj *point = &line->point[i];
        memcpy(point, &xy[i * 2], 2 * sizeof(double));
        if (m_has_z)
            point->z = z[i];
        if (m_has_m)
            point->m = m[i];
    }
}

void GeometryReader::readMultiPoint(shapeObj *shape)
{
    readLineString(shape);
    shape->type = MS_SHAPE_POINT;
}

void GeometryReader::readLineString(shapeObj *shape)
{
    lineObj *line = (lineObj *) malloc(sizeof(lineObj));
    readLineObj(line);
    shape->numlines = 1;
    shape->line = line;
    shape->type = MS_SHAPE_LINE;
}

void GeometryReader::readMultiLineString(shapeObj *shape)
{
    readPolygon(shape);
    shape->type = MS_SHAPE_LINE;
}

void GeometryReader::readPolygon(shapeObj *shape)
{
    const auto ends = m_geometry->ends();

    uint32_t nrings = 1;
    if (ends != nullptr && ends->size() > 1)
        nrings = ends->size();

    lineObj *line = (lineObj *) malloc(nrings * sizeof(lineObj));
    if (nrings > 1) {
        for (uint32_t i = 0; i < nrings; i++) {
            const auto e = ends->Get(i);
            m_length = e - m_offset;
            readLineObj(&line[i]);
            m_offset = e;
        }
    } else {
        readLineObj(line);
    }
    shape->numlines = nrings;
    shape->line = line;
    shape->type = MS_SHAPE_POLYGON;
}

/*void GeometryReader::readMultiPolygon(shapeObj *shape)
{
    // TODO
    return;
}

void GeometryReader::readGeometryCollection(shapeObj *shape)
{
    // TODO
    return;
}*/

void GeometryReader::read(shapeObj *shape)
{
    // nested types
    switch (m_geometry_type) {
        //case GeometryType::GeometryCollection: return readGeometryCollection(shape);
        //case GeometryType::MultiPolygon: return readMultiPolygon(shape);
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
        case GeometryType::Point: return readPoint(shape);
        case GeometryType::MultiPoint: return readMultiPoint(shape);
        case GeometryType::LineString: return readLineString(shape);
        case GeometryType::MultiLineString: return readMultiLineString(shape);
        case GeometryType::Polygon: return readPolygon(shape);
        /*
        case GeometryType::CircularString: return readSimpleCurve<OGRCircularString>(true);
        case GeometryType::Triangle: return readTriangle();
        case GeometryType::TIN: return readTIN();
        */
        default: break;
    }
}