typedef struct Sld {
	sld_layer_list * layers;
	ows_version * version;
} sld;

typedef struct Sld_named_layer {
	buffer * name;
	buffer * filter;
	sld_style_list * styles;
} sld_named_layer;

typedef struct Sld_style {
	buffer * name;
	buffer * title;
	buffer * abstract;
	bool is_default;
	feature_type_style_list * feature_type_style;
} sld_style;


typedef struct Sld_feature_type_style {
	buffer * name;
	buffer * title;
	buffer * abstract;
	buffer * feature_type_name;
	array * semantic_type_identifier;
	rule_list * rules;
} sld_feature_type_style;


typedef struct Sld_rule {
	buffer * name;
	buffer * title;
	buffer * abstract;
	sld_graphic * legend_graphic;
	fe_filter * filter;
	bool is_else_filter;
	double minscale;
	double maxscale;
	symbolizer_type_list * symbolizers;
} sld_rules;


typedef struct Sld_line_symbolizer {
	GEOM * geom; /* FIXME be sure about Postgis geom type */
	sld_stroke * stroke;
} sld_line_symbolizer;

enum sld_stroke_linejoin {
	SLD_STROKE_LINEJOIN_MITRE,
	SLD_STROKE_LINEJOIN_ROUND,
	SLD_STROKE_LINEJOIN_BEVEL,
};

enum sld_stroke_linecap {
	SLD_STROKE_LINECAP_BUTT,
	SLD_STROKE_LINECAP_ROUND,
	SLD_STROKE_LINECAP_SQUARE,
};

typedef struct Sld_stroke {
	ows_color * stroke;
	double * opacity;
	double width;
	enum sld_stroke_linejoin  linejoin;
	enum sld_stroke_linecap  linecap;
	buffer * dasharray; /* FIXME need a double list */
	double dashoffset;
	sld_graphic * graphic;
	bool is_graphic_fill;
} sld_stroke;

typedef struct Sld_polygon_symbolizer {
	GEOM * geom; /* FIXME be sure about Postgis geom type */
	sld_fill * fill;
	sld_stroke * stroke;
} sld_polygon_symbolizer;

typedef struct Sld_fill {
	sld_color * color;
	double opacity;
} sld_fill;


typedef struct Sld_point_symbolyzer {
	GEOM * geom; /* FIXME be sure about Postgis geom type */
	sld_graphic * graphic;
} sld_point_symbolizer;


typedef struct Sld_graphic {
	buffer * url;
	bool is_mark;
	double opacity;
	double rotation;
	double size;
} sld_graphic;
