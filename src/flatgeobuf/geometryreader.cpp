#include "geometryreader.h"

using namespace flatbuffers;
using namespace FlatGeobuf;

void GeometryReader::readPoint(shapeObj *shape)
{
    if (m_offset >= m_xy_count) {
        msSetError(MS_FGBERR,
                   "Corrupt FlatGeobuf geometry: point offset out of bounds",
                   "GeometryReader::readPoint");
        return;
    }

    lineObj *l = (lineObj *) malloc(sizeof(lineObj));
    pointObj *p = (pointObj *) malloc(sizeof(pointObj));

	p->x = m_xy[m_offset * 2 + 0];
	p->y = m_xy[m_offset * 2 + 1];
    if (m_has_z && m_geometry->z() != nullptr && m_offset < m_geometry->z()->size())
        p->z = m_geometry->z()->data()[m_offset];
    if (m_has_m && m_geometry->m() != nullptr && m_offset < m_geometry->m()->size())
        p->m = m_geometry->m()->data()[m_offset];

    l[0].numpoints = 1;
    l[0].point = p;
    shape->numlines = 1;
    shape->line = l;
    shape->type = MS_SHAPE_POINT;
}

/* Reads the current m_offset/m_length coordinate range into line. Returns
   false and leaves line untouched if that range is not entirely within the
   xy array, in which case the caller must drop the part rather than keep an
   empty one: shapeObj consumers such as msIsOuterRing() index point[0]
   without looking at numpoints. */
bool GeometryReader::readLineObj(lineObj *line)
{
    /* m_offset and m_length are derived from the geometry part "ends" taken
       from the (untrusted) file and are not validated by the flatbuffer
       accessors. Reject a range that does not fit the xy array before any
       read, so a crafted end index cannot walk off the coordinate buffer. */
    if (m_offset > m_xy_count || m_length > m_xy_count - m_offset) {
        msSetError(MS_FGBERR,
                   "Corrupt FlatGeobuf geometry: coordinate range out of bounds",
                   "GeometryReader::readLineObj");
        return false;
    }

    const double *z = nullptr;
    const double *m = nullptr;
    if (m_has_z) {
        const auto zv = m_geometry->z();
        if (zv == nullptr || m_offset + m_length > zv->size()) {
            msSetError(MS_FGBERR,
                       "Corrupt FlatGeobuf geometry: z range out of bounds",
                       "GeometryReader::readLineObj");
            return false;
        }
        z = zv->data();
    }
    if (m_has_m) {
        const auto mv = m_geometry->m();
        if (mv == nullptr || m_offset + m_length > mv->size()) {
            msSetError(MS_FGBERR,
                       "Corrupt FlatGeobuf geometry: m range out of bounds",
                       "GeometryReader::readLineObj");
            return false;
        }
        m = mv->data();
    }

    line->point = (pointObj *) malloc(m_length * sizeof(pointObj));
    line->numpoints = m_length;

    for (uint32_t i = m_offset; i < m_offset + m_length; i++) {
        pointObj *point = &line->point[i - m_offset];
        memcpy(point, &m_xy[i * 2], 2 * sizeof(double));
        if (m_has_z)
            point->z = z[i];
        if (m_has_m)
            point->m = m[i];
    }

    return true;
}

void GeometryReader::readMultiPoint(shapeObj *shape)
{
    readLineString(shape);
    shape->type = MS_SHAPE_POINT;
}

void GeometryReader::readLineString(shapeObj *shape)
{
    lineObj *line = (lineObj *) malloc(sizeof(lineObj));
    shape->numlines = readLineObj(line) ? 1 : 0;
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
    uint32_t numlines = 0;
    if (nrings > 1) {
        for (uint32_t i = 0; i < nrings; i++) {
            const auto e = ends->Get(i);
            m_length = e - m_offset;
            if (readLineObj(&line[numlines]))
                numlines++;
            m_offset = e;
        }
    } else if (readLineObj(line)) {
        numlines = 1;
    }
    shape->numlines = numlines;
    shape->line = line;
    shape->type = MS_SHAPE_POLYGON;
}

void GeometryReader::readMultiPolygon(shapeObj *shape)
{
    const auto parts = m_geometry->parts();
    lineObj *line = (lineObj *) nullptr;
    auto numlines = 0;
    for (size_t i = 0; i < parts->size(); i++) {
        GeometryReader(m_ctx, parts->Get(i), GeometryType::Polygon).read(shape);
        lineObj *tmp = line;
        line = (lineObj *) realloc(line, (numlines + shape->numlines) * sizeof(lineObj));
        if (!line) {
            free(tmp);
            free(shape->line);
            break;
        }
        for (int j = 0; j < shape->numlines; j++)
            line[numlines + j] = shape->line[j];
        numlines += shape->numlines;
        free(shape->line);
    }
    shape->line = line;
    shape->numlines = numlines;
}

/*void GeometryReader::readGeometryCollection(shapeObj *shape)
{
    // TODO
    return;
}*/

void GeometryReader::read(shapeObj *shape)
{
    // nested types
    switch (m_geometry_type) {
        //case GeometryType::GeometryCollection: return readGeometryCollection(shape);
        case GeometryType::MultiPolygon: return readMultiPolygon(shape);
        /*case GeometryType::CompoundCurve: return readCompoundCurve();
        case GeometryType::CurvePolygon: return readCurvePolygon();
        case GeometryType::MultiCurve: return readMultiCurve();
        case GeometryType::MultiSurface: return readMultiSurface();
        case GeometryType::PolyhedralSurface: return readPolyhedralSurface();*/
        default: break;
    }

    // if not nested must have geometry data
    const auto pXy = m_geometry->xy();
    if (pXy == nullptr)
        return;
    const auto xySize = pXy->size();
    m_xy = pXy->data();
    m_xy_count = xySize / 2;
    m_length = m_xy_count;

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