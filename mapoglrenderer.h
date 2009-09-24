#ifndef OGL_H
#define OGL_H

#ifdef USE_OGL


#include <vector>
#include <map>
#include <FTGL/ftgl.h>
#include <FTGL/FTGLTextureFont.h>

#include "mapoglcontext.h"
#include "mapserver.h"

class OglCache
{
public:
	~OglCache()
	{
		glDeleteTextures(1, &texture);
	}
	GLuint texture;
	ms_uint32 width;
	ms_uint32 height;
	ms_uint32 patternDistance;
};

class OglRenderer {
public:
	OglRenderer(ms_uint32 width, ms_uint32 height, colorObj* color = NULL);
	virtual ~OglRenderer();
    GLubyte* gd2ogl(gdImagePtr img);

	void renderPolyline(shapeObj *p,colorObj *c, double width, int patternlength, double* pattern, int lineCap = MS_CJC_ROUND, int joinStyle = MS_CJC_ROUND, colorObj *outlinecolor = NULL, double outlinewidth = 0);
	void renderPolygon(shapeObj*, colorObj *color, colorObj *outlinecolor, double outlinewidth, OglCache* tile = NULL, int lineCap=MS_CJC_ROUND, int joinStyle=MS_CJC_ROUND);
	void renderGlyphs(double x, double y, colorObj *color, colorObj *outlinecolor, double size, char* font, char *thechars, double angle, colorObj *shadowcolor, double shdx, double shdy);
    void renderPixmap(symbolObj *symbol, double x, double y, double angle, double scale);
    void renderEllipse(double x, double y, double angle, double w, double h, colorObj *color, colorObj *outlinecolor, double outlinewidth);
    void renderVectorSymbol(double x, double y, symbolObj *symbol, double scale, double angle, colorObj *c, colorObj *oc, double ow);
    void renderTile(OglCache* tile, double x, double y, double angle);
    void renderPolylineTile(shapeObj *p, OglCache* tile);

    void getStringBBox(char *font, double size, char *string, rectObj *rect, double** advances);
    void setTransparency(double transparency);
    void attach(imageObj* img);

    int getWidth() { return width; }
    int getHeight() { return height; }
protected:
    ms_uint32 width;
    ms_uint32 height;
	double transparency;

	GLint viewportX;
	GLint viewportY;
	GLsizei viewportWidth;
	GLsizei viewportHeight;

	typedef std::map<char*,std::map<double,FTFont*> > fontCache_t;
    typedef std::map<symbolObj*,std::map<double,GLuint> > dashCache_t;

    FTFont* getFTFont(char* font, double size);
    bool loadLine(shapeObj* shape, double width, int patternlength, double *pattern);
    double drawQuad(pointObj *p1, pointObj *p2, double width, double tilelength = 0.0, double textureStart = 0.0);
    double drawTriangles(pointObj *p1, pointObj *p2, double width, double tilelength = 0.0, double textureStart = 0.0);
    void drawVectorLineStrip(symbolObj *symbol, double width);
    void drawFan(pointObj* center, pointObj* from, pointObj* to, int resolution);
    void createShapes();

	void makeCurrent();
    void setColor(colorObj *color);

    GLUtesselator *tess;
    enum shapes_t{ circle = 0};

    OglContext* context;

    static dashCache_t dashCache;
    static fontCache_t fontCache;
    static std::vector<GLuint> shapes;
    static std::vector<symbolObj*> testSymbols;
    static ms_uint32 OUTLINE_WIDTH;
	static ms_uint32 FONT_SIZE;
	static ms_uint32 FONT_RES;
	static double OGL_PI;
	static ms_uint32 SHAPE_CIRCLE_RES;
	static double SHAPE_CIRCLE_RADIUS;
	static double SIZE_RES;
};

class OglTexture : public OglRenderer
{
public:
	OglTexture(ms_uint32 width, ms_uint32 height, colorObj* color = NULL);
	~OglTexture();
	OglCache* renderToTile();
private:
	ms_uint32 textureWidth;
	ms_uint32 textureHeight;
	ms_uint32 pow2width;
	ms_uint32 pow2height;
	double scaleWidth;
	double scaleHeight;
	ms_uint32 getTextureSize(GLuint dimension, ms_uint32 value);
	GLuint NextPowerOf2(GLuint in);
	GLuint createTexture(ms_uint32 x, ms_uint32 y);
	static ms_uint32 TEXTURE_BORDER;
};

#endif /* USE_OGL */
#endif

