#include "flatgeobuf_c.h"

#include "feature_generated.h"
#include "geometryreader.h"
#include "packedrtree.h"
#include <stdexcept>

using namespace mapserver::flatbuffers;
using namespace mapserver::FlatGeobuf;

typedef flatgeobuf_ctx ctx;

uint8_t flatgeobuf_magicbytes[] = { 0x66, 0x67, 0x62, 0x03, 0x66, 0x67, 0x62, 0x01 };
uint8_t FLATGEOBUF_MAGICBYTES_SIZE = sizeof(flatgeobuf_magicbytes);
uint32_t INIT_BUFFER_SIZE = 1024 * 4;

template <typename T>
void parse_value(uint8_t *data, char **values, uint16_t i, uint32_t &offset, bool found)
{
    using std::to_string;
    if (found)
    {
        msFree(values[i]);
        values[i] = msStrdup(to_string(*((T*) (data + offset))).c_str());
    }
    offset += sizeof(T);
}

ctx *flatgeobuf_init_ctx()
{
    ctx *c = (ctx *) malloc(sizeof(ctx));
    memset(c, 0, sizeof(ctx));
    c->is_null_geom = false;
    c->done = false;
    return c;
}

void flatgeobuf_free_ctx(ctx *ctx)
{
    if (ctx->columns)
        free(ctx->columns);
    if (ctx->search_result)
        free(ctx->search_result);
    if (ctx->buf)
        free(ctx->buf);
    free(ctx);
}

void flatgeobuf_ensure_line(ctx *ctx, uint32_t len)
{
    if (!ctx->line) {
        ctx->line_len = len;
        ctx->line = (lineObj *) malloc(ctx->line_len * sizeof(lineObj));
        return;
    }
    if (ctx->line_len < len) {
        ctx->line_len = len;
        ctx->line = (lineObj *) realloc(ctx->buf, ctx->line_len * sizeof(lineObj));
    }
}

void flatgeobuf_ensure_point(ctx *ctx, uint32_t len)
{
    if (!ctx->point) {
        ctx->point_len = len;
        ctx->point = (pointObj *) malloc(ctx->point_len * sizeof(pointObj));
        return;
    }
    if (ctx->point_len < len) {
        ctx->point_len = len;
        ctx->point = (pointObj *) realloc(ctx->buf, ctx->point_len * sizeof(pointObj));
    }
}

int flatgeobuf_ensure_buf(ctx *ctx, uint32_t size)
{
    if (size > 100 * 1024 * 1024)
        return -1;
    if (!ctx->buf) {
        ctx->buf_size = INIT_BUFFER_SIZE;
        ctx->buf = (uint8_t *) malloc(ctx->buf_size);
        return 0;
    }
    if (ctx->buf_size < size) {
        ctx->buf_size = ctx->buf_size * 2;
        ctx->buf = (uint8_t *) realloc(ctx->buf, ctx->buf_size);
        flatgeobuf_ensure_buf(ctx, size);
    }
    return 0;
}

int flatgeobuf_decode_feature(ctx *ctx, layerObj *layer, shapeObj *shape)
{
    ctx->is_null_geom = false;

    uint32_t featureSize;
    if (VSIFReadL(&featureSize, sizeof(featureSize), 1, ctx->file) != 1) {
        if (VSIFEofL(ctx->file)) {
            ctx->done = true;
            return 0;
        }
        return -1;
    }

    ctx->offset += sizeof(uoffset_t);
    if (flatgeobuf_ensure_buf(ctx, featureSize) != 0) {
        msSetError(MS_FGBERR, "Failed to allocate buffer for feature", "flatgeobuf_decode_feature");
        return -1;
    }

    if (VSIFReadL(ctx->buf, 1, featureSize, ctx->file) != featureSize) {
        msSetError(MS_FGBERR, "Failed to read feature", "flatgeobuf_decode_feature");
        return -1;
    }
    ctx->offset += featureSize;
    auto feature = GetFeature(ctx->buf);
    const auto geometry = feature->geometry();
    if (geometry) {
        GeometryReader(ctx, geometry).read(shape);
    } else {
        ctx->is_null_geom = true;
        return 0;
    }
    auto properties = feature->properties();
    if (properties && properties->size() != 0) {
        ctx->properties = (uint8_t *) properties->data();
        ctx->properties_size = properties->size();
        flatgeobuf_decode_properties(ctx, layer, shape);
    } else {
        ctx->properties_size = 0;
    }

    return 0;
}

int flatgeobuf_decode_properties(ctx *ctx, layerObj *layer, shapeObj *shape)
{
	uint8_t type;
	uint32_t offset = 0;
	uint8_t *data = ctx->properties;
	uint32_t size = ctx->properties_size;
    uint16_t numvalues = layer->numitems;
    bool found;
    char **values;

    if (numvalues == 0)
        return 0;

    values = (char **) msSmallCalloc(sizeof(char *), numvalues);
    if (shape->values) msFreeCharArray(shape->values, shape->numvalues);
    shape->numvalues = numvalues;
    shape->values = values;

    if (size > 0 && size < (sizeof(uint16_t) + sizeof(uint8_t))) {
        msSetError(MS_FGBERR, "Unexpected properties data size", "flatgeobuf_decode_properties");
        return -1;
    }
	while (offset + 1 < size) {
        uint16_t i;
        memcpy(&i, data + offset, sizeof(uint16_t));
		offset += sizeof(uint16_t);
		if (i >= ctx->columns_len) {
            msSetError(MS_FGBERR, "Column index out of range", "flatgeobuf_decode_properties");
            return -1;
        }
		const auto& column = ctx->columns[i];
        const int32_t ii = column.itemindex;
        found = ii != -1;
		type = column.type;
		switch (type) {
		case flatgeobuf_column_type_bool:
            parse_value<uint8_t>(data, values, ii, offset, found);
			break;
		case flatgeobuf_column_type_byte:
            parse_value<int8_t>(data, values, ii, offset, found);
			break;
		case flatgeobuf_column_type_ubyte:
            parse_value<uint8_t>(data, values, ii, offset, found);
			break;
		case flatgeobuf_column_type_short:
			parse_value<int16_t>(data, values, ii, offset, found);
			break;
		case flatgeobuf_column_type_ushort:
            parse_value<uint16_t>(data, values, ii, offset, found);
			break;
		case flatgeobuf_column_type_int:
            parse_value<int32_t>(data, values, ii, offset, found);
            break;
		case flatgeobuf_column_type_uint:
			parse_value<uint32_t>(data, values, ii, offset, found);
			break;
		case flatgeobuf_column_type_long:
            parse_value<int64_t>(data, values, ii, offset, found);
            break;
		case flatgeobuf_column_type_ulong:
			parse_value<uint64_t>(data, values, ii, offset, found);
			break;
		case flatgeobuf_column_type_float:
            parse_value<float>(data, values, ii, offset, found);
			break;
		case flatgeobuf_column_type_double:
			parse_value<double>(data, values, ii, offset, found);
			break;
        case flatgeobuf_column_type_datetime:
		case flatgeobuf_column_type_string: {
			uint32_t len;
			if (offset + sizeof(len) > size){
                msSetError(MS_FGBERR, "Invalid size for string value", "flatgeobuf_decode_properties");
                return -1;
            }
			memcpy(&len, data + offset, sizeof(uint32_t));
			offset += sizeof(len);
			if (found) {
                char *str = (char *) msSmallMalloc(len + 1);
                memcpy(str, data + offset, len);
                str[len] = '\0';
                msFree(values[ii]);
                values[ii] = str;
            }
			offset += len;
			break;
		}
        }
    }

    for(int i = 0; i < shape->numvalues; i++)
    {
        if( shape->values[i] == NULL)
            shape->values[i] = msStrdup("");
    }

    return 0;
}

int flatgeobuf_check_magicbytes(ctx *ctx)
{
    if (ctx->offset != 0) {
        msSetError(MS_FGBERR, "Unexpected offset", "flatgeobuf_check_magicbytes");
        return -1;
    }
    flatgeobuf_ensure_buf(ctx, FLATGEOBUF_MAGICBYTES_SIZE);
    if (VSIFReadL(ctx->buf, 8, 1, ctx->file) != 1) {
        msSetError(MS_FGBERR, "Failed to read magicbytes", "flatgeobuf_check_magicbytes");
        return -1;
    }
	uint32_t i;
	for (i = 0; i < FLATGEOBUF_MAGICBYTES_SIZE / 2; i++) {
        if (ctx->buf[i] != flatgeobuf_magicbytes[i]) {
            msSetError(MS_FGBERR, "Data is not FlatGeobuf", "flatgeobuf_check_magicbytes");
            return -1;
        }
    }
	ctx->offset += FLATGEOBUF_MAGICBYTES_SIZE;
    return 0;
}

int flatgeobuf_decode_header(ctx *ctx)
{
    if (ctx->offset != FLATGEOBUF_MAGICBYTES_SIZE) {
        msSetError(MS_FGBERR, "Unexpected offset", "flatgeobuf_decode_header");
        return -1;
    }
    if (VSIFSeekL(ctx->file, ctx->offset, SEEK_SET) == -1) {
        msSetError(MS_FGBERR, "Unable to get seek in file", "flatgeobuf_decode_header");
        return -1;
    }
    uint32_t headerSize;
    if (VSIFReadL(&headerSize, 4, 1, ctx->file) != 1) {
        msSetError(MS_FGBERR, "Failed to read header size", "flatgeobuf_decode_header");
        return -1;
    }
    ctx->offset += sizeof(uoffset_t);
    if (flatgeobuf_ensure_buf(ctx, headerSize) != -1) {
        msSetError(MS_FGBERR, "Failed to allocate buffer for header", "flatgeobuf_decode_header");
    }
    if (VSIFReadL(ctx->buf, 1, headerSize, ctx->file) != headerSize) {
        msSetError(MS_FGBERR, "Failed to read header", "flatgeobuf_decode_header");
        return -1;
    }
    auto header = GetHeader(ctx->buf);
	ctx->offset += headerSize;

	ctx->geometry_type = (uint8_t) header->geometry_type();
    ctx->features_count = header->features_count();
    auto envelope = header->envelope();
    if (envelope != nullptr) {
        ctx->has_extent = true;
        ctx->xmin = envelope->Get(0);
        ctx->ymin = envelope->Get(1);
        ctx->xmax = envelope->Get(2);
        ctx->ymax = envelope->Get(3);
        ctx->bounds.minx = ctx->xmin;
        ctx->bounds.miny = ctx->ymin;
        ctx->bounds.maxx = ctx->xmax;
        ctx->bounds.maxy = ctx->ymax;
    }
	ctx->has_z = header->has_z();
    ctx->has_m = header->has_m();
    ctx->has_t = header->has_t();
    ctx->has_tm = header->has_tm();
    ctx->index_node_size = header->index_node_size();
    auto crs = header->crs();
    if (crs != nullptr)
        ctx->srid = crs->code();
    auto columns = header->columns();
    if (columns != nullptr) {
        auto size = columns->size();
        ctx->columns = (flatgeobuf_column *) malloc(size * sizeof(flatgeobuf_column));
        memset(ctx->columns, 0, size * sizeof(flatgeobuf_column));
        ctx->columns_len = size;
        for (uint32_t i = 0; i < size; i++) {
            auto column = columns->Get(i);
            ctx->columns[i].name = column->name()->c_str();
            ctx->columns[i].type = (uint8_t) column->type();
            ctx->columns[i].itemindex = -1;
        }
    }

    ctx->index_offset = ctx->offset;
    ctx->feature_offset = ctx->offset;
    if ( ctx->index_node_size > 0)
        ctx->feature_offset += PackedRTree::size(ctx->features_count, ctx->index_node_size);

    return 0;
}

int flatgeobuf_index_search(ctx *ctx, rectObj *rect)
{
    const auto treeOffset = ctx->offset;
    const auto readNode = [treeOffset, ctx] (uint8_t *buf, size_t i, size_t s) {
        if (VSIFSeekL(ctx->file, treeOffset + i, SEEK_SET) == -1)
            throw std::runtime_error("Unable to seek in file");
        if (VSIFReadL(buf, 1, s, ctx->file) != s)
            throw std::runtime_error("Unable to read file");
    };
    NodeItem n { rect->minx, rect->miny, rect->maxx, rect->maxy, 0 };
    try {
        const auto foundItems = PackedRTree::streamSearch(ctx->features_count, ctx->index_node_size, n, readNode);
        ctx->search_result = (flatgeobuf_search_item *) malloc(foundItems.size() * sizeof(flatgeobuf_search_item));
        memcpy(ctx->search_result, foundItems.data(), foundItems.size() * sizeof(flatgeobuf_search_item));
        ctx->search_result_len = (uint32_t) foundItems.size();
    } catch (const std::exception &e) {
        msSetError(MS_FGBERR, "Unable to seek or read file", "flatgeobuf_index_search");
        return -1;
    }
    return 0;
}

int flatgeobuf_index_skip(ctx *ctx)
{
    auto treeSize = PackedRTree::size(ctx->features_count, ctx->index_node_size);
    ctx->offset += treeSize;
    if (VSIFSeekL(ctx->file, ctx->offset, SEEK_SET) == -1) {
        msSetError(MS_FGBERR, "Unable to seek in file", "flatgeobuf_index_skip");
        return -1;
    }
    return 0;
}

int flatgeobuf_read_feature_offset(ctx *ctx, uint64_t index, uint64_t *featureOffset)
{
    try {
        const auto levelBounds = PackedRTree::generateLevelBounds(ctx->features_count, ctx->index_node_size);
        const auto bottomLevelOffset = ctx->index_offset + (levelBounds.front().first * sizeof(NodeItem));
        const auto nodeItemOffset = bottomLevelOffset + (index * sizeof(NodeItem));
        const auto featureOffsetOffset = nodeItemOffset + (sizeof(double) * 4);
        if (VSIFSeekL(ctx->file, featureOffsetOffset, SEEK_SET) == -1) {
            msSetError(MS_FGBERR, "Failed to seek feature offset", "flatgeobuf_read_feature_offset");
            return -1;
        }
        if (VSIFReadL(featureOffset, sizeof(uint64_t), 1, ctx->file) != 1) {
            msSetError(MS_FGBERR, "Failed to read feature offset", "flatgeobuf_read_feature_offset");
            return -1;
        }
        return 0;
    } catch (const std::exception& e) {
        msSetError(MS_FGBERR, "Failed to calculate tree size", "flatgeobuf_read_feature_offset");
        return -1;
    }
}
