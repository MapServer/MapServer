#include "flatgeobuf_c.h"

#include "feature_generated.h"
#include "geometryreader.h"
#include "packedrtree.h"

using namespace flatbuffers;
using namespace FlatGeobuf;

typedef flatgeobuf_ctx ctx;

uint8_t flatgeobuf_magicbytes[] = { 0x66, 0x67, 0x62, 0x03, 0x66, 0x67, 0x62, 0x01 };
uint8_t FLATGEOBUF_MAGICBYTES_SIZE = sizeof(flatgeobuf_magicbytes);
uint32_t INIT_BUFFER_SIZE = 1024 * 4;

ctx *flatgeobuf_init_ctx()
{
    ctx *c = (ctx *) malloc(sizeof(ctx));
    memset(c, 0, sizeof(ctx));
    c->done = false;
    return c;
}

void flatgeobuf_free_ctx(ctx *ctx)
{
    if (ctx->columns) {
        free(ctx->columns);
    }
    if (ctx->search_result)
        free(ctx->search_result);
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

void flatgeobuf_ensure_buf(ctx *ctx, uint32_t size)
{
    if (!ctx->buf) {
        ctx->buf_size = INIT_BUFFER_SIZE;
        ctx->buf = (uint8_t *) malloc(ctx->buf_size);
        return;
    }
    if (ctx->buf_size < size) {
        ctx->buf_size = ctx->buf_size * 2;
        ctx->buf = (uint8_t *) realloc(ctx->buf, ctx->buf_size);
        flatgeobuf_ensure_buf(ctx, size);
    }
}

int flatgeobuf_decode_feature(ctx *ctx, shapeObj *shape)
{
    uint32_t featureSize;
    if (VSIFReadL(&featureSize, sizeof(featureSize), 1, ctx->file) != 1) {
        if (VSIFEofL(ctx->file)) {
            ctx->done = true;
            return 0;
        }
        return -1;
    }

    ctx->offset += sizeof(uoffset_t);
    flatgeobuf_ensure_buf(ctx, featureSize);

    if (VSIFReadL(ctx->buf, 1, featureSize, ctx->file) != featureSize) {
        msSetError(MS_FGBERR, "Failed to read feature", "flatgeobuf_decode_feature");
        return -1;
    }
    ctx->offset += featureSize;
    auto feature = GetFeature(ctx->buf);
    const auto geometry = feature->geometry();

    if (geometry) {
        GeometryReader reader(ctx, geometry);
        reader.read(shape);
    } else {
        // skip to next if null geometry
        flatgeobuf_decode_feature(ctx, shape);
    }
    auto properties = feature->properties();
    if (properties && properties->size() != 0) {
        ctx->properties = (uint8_t *) properties->data();
        ctx->properties_len = properties->size();
    } else {
        ctx->properties_len = 0;
    }

    return 0;
}

int flatgeobuf_check_magicbytes(ctx *ctx)
{
    if (ctx->offset != 0) {
        msSetError(MS_FGBERR, "Unexpected offset", "flatgeobuf_check_magicbytes");
        return -1;
    }
	ctx->buf = (uint8_t *) malloc(FLATGEOBUF_MAGICBYTES_SIZE);
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
    flatgeobuf_ensure_buf(ctx, headerSize);
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
        }
    }

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