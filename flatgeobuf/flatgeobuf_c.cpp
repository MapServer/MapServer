#include "flatgeobuf_c.h"
#include "feature_generated.h"
#include "geometryreader.h"
#include "packedrtree.h"

using namespace flatbuffers;
using namespace FlatGeobuf;

typedef flatgeobuf_ctx ctx;

uint8_t flatgeobuf_magicbytes[] = { 0x66, 0x67, 0x62, 0x03, 0x66, 0x67, 0x62, 0x01 };
uint8_t FLATGEOBUF_MAGICBYTES_SIZE = sizeof(flatgeobuf_magicbytes);

struct FeatureItem : FlatGeobuf::Item {
    uoffset_t size;
    uint64_t offset;
};

int flatgeobuf_decode_feature(ctx *ctx)
{
    auto size = flatbuffers::GetPrefixedSize(ctx->buf + ctx->offset);

    Verifier verifier(ctx->buf + ctx->offset, size);
	if (VerifySizePrefixedFeatureBuffer(verifier)) {
        msSetError(MS_FGBERR, "buffer did not pass verification", "flatgeobuf_decode_feature");
        return -1;
    }

    ctx->offset += sizeof(uoffset_t);

    auto feature = GetFeature(ctx->buf + ctx->offset);
    ctx->offset += size;

    const auto geometry = feature->geometry();
    if (geometry != nullptr) {
        GeometryReader reader(geometry, (GeometryType) ctx->geometry_type, ctx->has_z, ctx->has_m);
        ctx->lwgeom = reader.read();
        if (ctx->srid > 0)
            lwgeom_set_srid(ctx->lwgeom, ctx->srid);
    } else {
        ctx->lwgeom = NULL;
    }
    if (feature->properties() != nullptr && feature->properties()->size() != 0) {
        ctx->properties = (uint8_t *) feature->properties()->data();
        ctx->properties_len = feature->properties()->size();
    } else {
        ctx->properties_len = 0;
    }

    return 0;
}

int flatgeobuf_decode_header(ctx *ctx)
{
    auto size = flatbuffers::GetPrefixedSize(ctx->buf + ctx->offset);

    Verifier verifier(ctx->buf + ctx->offset, size);
	if (VerifySizePrefixedHeaderBuffer(verifier)) {
        msSetError(MS_FGBERR, "buffer did not pass verification", "flatgeobuf_decode_header");
        return -1;
    }

    ctx->offset += sizeof(uoffset_t);

    auto header = GetHeader(ctx->buf + ctx->offset);
	ctx->offset += size;

	ctx->geometry_type = (uint8_t) header->geometry_type();
    ctx->features_count = header->features_count();
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
        ctx->columns = (flatgeobuf_column **) lwalloc(sizeof(flatgeobuf_column *) * size);
        ctx->columns_size = size;
        for (uint32_t i = 0; i < size; i++) {
            auto column = columns->Get(i);
            ctx->columns[i] = (flatgeobuf_column *) lwalloc(sizeof(flatgeobuf_column));
            memset(ctx->columns[i], 0, sizeof(flatgeobuf_column));
            ctx->columns[i]->name = column->name()->c_str();
            ctx->columns[i]->type = (uint8_t) column->type();
        }
    }

    if (ctx->index_node_size > 0 && ctx->features_count > 0) {
        auto treeSize = PackedRTree::size(ctx->features_count, ctx->index_node_size);
        ctx->offset += treeSize;
    }

    return 0;
}