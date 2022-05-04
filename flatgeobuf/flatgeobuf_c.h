#ifndef FLATGEOBUF_C_H
#define FLATGEOBUF_C_H

#include "../../mapserver.h"
#include "../../maperror.h"
#include "../../mapprimitive.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t flatgeobuf_magicbytes[];
extern uint8_t FLATGEOBUF_MAGICBYTES_SIZE;

// need c compatible variant of this enum
#define flatgeobuf_column_type_byte UINT8_C(0)
#define flatgeobuf_column_type_ubyte UINT8_C(1)
#define flatgeobuf_column_type_bool UINT8_C(2)
#define flatgeobuf_column_type_short UINT8_C(3)
#define flatgeobuf_column_type_ushort UINT8_C(4)
#define flatgeobuf_column_type_int UINT8_C(5)
#define flatgeobuf_column_type_uint UINT8_C(6)
#define flatgeobuf_column_type_long UINT8_C(7)
#define flatgeobuf_column_type_ulong UINT8_C(8)
#define flatgeobuf_column_type_float UINT8_C(9)
#define flatgeobuf_column_type_double UINT8_C(10)
#define flatgeobuf_column_type_string UINT8_C(11)
#define flatgeobuf_column_type_json UINT8_C(12)
#define flatgeobuf_column_type_datetime UINT8_C(13)
#define flatgeobuf_column_type_binary UINT8_C(14)

typedef struct flatgeobuf_column
{
	const char *name;
	uint8_t type;
	const char *title;
	const char *description;
	uint32_t width;
	uint32_t precision;
	uint32_t scale;
	bool nullable;
	bool unique;
	bool primary_key;
	const char * metadata;
} flatgeobuf_column;

typedef struct flatgeobuf_item
{
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	uint32_t size;
	uint64_t offset;
} flatgeobuf_item;

typedef struct flatgeobuf_search_item {
    uint64_t offset;
    uint64_t index;
} flatgeobuf_search_item;

typedef struct flatgeobuf_ctx
{
	VSILFILE *file;
	uint64_t feature_offset;
	uint64_t offset;
	uint8_t *buf;
	uint32_t buf_size;
	bool done;

    // header contents
	const char *name;
	uint64_t features_count;
	uint8_t geometry_type;
	bool has_extent;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	bool has_z;
	bool has_m;
	bool has_t;
	bool has_tm;
	uint16_t index_node_size;
	int32_t srid;
	flatgeobuf_column *columns;
	uint16_t columns_len;

	// mapserver structs
	rectObj bounds;

	// index search result
	flatgeobuf_search_item *search_result;
	uint32_t search_result_len;
	uint32_t search_index;

	// shape parts buffers
	// NOTE: not used at this time, need to introduce optional free in mapdraw
	lineObj *line;
	uint32_t line_len;
	pointObj *point;
	uint32_t point_len;

	int ms_type;
    uint8_t *properties;
	uint32_t properties_len;
    uint32_t properties_size;
} flatgeobuf_ctx;

flatgeobuf_ctx *flatgeobuf_init_ctx();
void flatgeobuf_free_ctx(flatgeobuf_ctx *ctx);

void flatgeobuf_ensure_buf(flatgeobuf_ctx *ctx, uint32_t size);
void flatgeobuf_ensure_line(flatgeobuf_ctx *ctx, uint32_t len);
void flatgeobuf_ensure_point(flatgeobuf_ctx *ctx, uint32_t len);

int flatgeobuf_check_magicbytes(flatgeobuf_ctx *ctx);
int flatgeobuf_decode_header(flatgeobuf_ctx *ctx);
int flatgeobuf_decode_feature(flatgeobuf_ctx *ctx, shapeObj *shape);

int flatgeobuf_index_search(flatgeobuf_ctx *ctx, rectObj *rect);
int flatgeobuf_index_skip(flatgeobuf_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* FLATGEOBUF_C_H */
