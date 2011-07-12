#ifdef USE_OGL

#include <algorithm>

#include "mapserver.h"
#include "maperror.h"
#include "mapoglrenderer.h"

using namespace std;


double OglRenderer::SIZE_RES = 100.0 / 72.0;
ms_uint32 OglRenderer::OUTLINE_WIDTH = 2;
ms_uint32 OglRenderer::FONT_SIZE = 20;
ms_uint32 OglRenderer::FONT_RES = 200;
double OglRenderer::OGL_PI = 3.14159265;
ms_uint32 OglRenderer::SHAPE_CIRCLE_RES = 10;
double OglRenderer::SHAPE_CIRCLE_RADIUS = 10.0;

ms_uint32 OglTexture::TEXTURE_BORDER = 4;

OglRenderer::fontCache_t OglRenderer::fontCache = OglRenderer::fontCache_t();
OglRenderer::dashCache_t OglRenderer::dashCache = OglRenderer::dashCache_t();
std::vector<GLuint> OglRenderer::shapes = std::vector<GLuint>();

GLvoid CALLBACK beginCallback(GLenum which);
GLvoid CALLBACK errorCallback(GLenum errorCode);
GLvoid CALLBACK endCallback(void);
GLvoid CALLBACK combineDataCallback(GLdouble coords[3], GLdouble* vertex_data[4], GLfloat weight[4], void** dataOut, void* polygon_data);
GLvoid CALLBACK vertexCallback(GLdouble *vertex);

OglTexture::OglTexture(ms_uint32 width, ms_uint32 height, colorObj* color)
	: OglRenderer(getTextureSize(GL_TEXTURE_WIDTH, width)+TEXTURE_BORDER,
				  getTextureSize(GL_TEXTURE_HEIGHT, height)+TEXTURE_BORDER)

{
	this->pow2width = getWidth()-TEXTURE_BORDER;
	this->pow2height = getHeight()-TEXTURE_BORDER;
	this->textureWidth = width;
	this->textureHeight = height;

	glPushMatrix();

	glRasterPos2d(TEXTURE_BORDER/2, TEXTURE_BORDER/2);
	glPixelZoom((float)pow2width/width, (float)pow2height/height);
	glTranslated(TEXTURE_BORDER/2, TEXTURE_BORDER/2, 0);
	glScaled((double)pow2width/width, (double)pow2height/height, 1.0);
}

OglTexture::~OglTexture()
{
	makeCurrent();
	glPopMatrix();
	glPixelZoom(1.0f, 1.0f);
	glRasterPos2d(0, 0);
}

OglCache* OglTexture::renderToTile()
{
	makeCurrent();
	OglCache* tile = new OglCache();
	tile->texture = createTexture(TEXTURE_BORDER/2, TEXTURE_BORDER/2);
	tile->width = this->textureWidth;
	tile->height = this->textureHeight;
	return tile;
}

GLuint OglTexture::createTexture(ms_uint32 offsetX, ms_uint32 offsetY)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	if (OglContext::MAX_ANISOTROPY != 0)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, OglContext::MAX_ANISOTROPY);
	}
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, offsetX, offsetY, pow2width, pow2height, 0);
	glBindTexture(GL_TEXTURE_2D,0);
	return texture;
}

ms_uint32 OglTexture::getTextureSize(GLuint dimension, ms_uint32 value)
{
	glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA,  value, value, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    GLint check;
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, dimension, &check);
    if (check == 0)
    {
    	return MS_MAX(MS_MIN(NextPowerOf2(value), OglContext::MAX_TEXTURE_SIZE), OglContext::MIN_TEXTURE_SIZE);
    }
    else
    {
    	return value;
    }
}

OglRenderer::OglRenderer(ms_uint32 width, ms_uint32 height, colorObj* color)
{
	int viewPort[4];
	glGetIntegerv(GL_VIEWPORT ,viewPort);
	viewportX = viewPort[0];
	viewportY = viewPort[1];
	viewportWidth = viewPort[2];
	viewportHeight = viewPort[3];

	glPushMatrix();

	this->width = width;
	this->height = height;
	context = new OglContext(width, height);
	context->makeCurrent();
	transparency = 1.0;

	// set read/write context to pbuffer
	glEnable(GL_MULTISAMPLE_ARB);
	glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);

	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// draw & read the front buffer of pbuffer
	glDrawBuffer(GL_FRONT);
	glReadBuffer(GL_FRONT);

	if (!(tess = gluNewTess()))
	{
		msSetError(MS_OGLERR, "P-buffer Error: Unable to create tessalotor",
				"OglRenderer");
	}

	gluTessCallback(tess, GLU_TESS_VERTEX, (void (CALLBACK*)(void)) vertexCallback);
	gluTessCallback(tess, GLU_TESS_BEGIN, (void(CALLBACK*)(void)) beginCallback);
	gluTessCallback(tess, GLU_TESS_END, (void(CALLBACK*)(void)) endCallback);
	gluTessCallback(tess, GLU_TESS_ERROR, (void(CALLBACK*)(void)) errorCallback);
	gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (void(CALLBACK*)(void)) combineDataCallback);

	if (color)
	{
		glClearColor((double) color->red / 255,
					 (double) color->green / 255,
					 (double) color->blue / 255,
					 (double)color->alpha / 100);
	}
	else
	{
		glClearColor(0.0, 0.0, 0.0, 0.0);
	}
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, 0.0, height, -3.0, 3.0);

	createShapes();
}

OglRenderer::~OglRenderer()
{
	makeCurrent();
	glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
	glPopMatrix();
	delete context;
}

void OglRenderer::getStringBBox(char *font, double size, char *string, rectObj *rect, double** advances)
{
	FTFont* face = getFTFont(font, size);
	float llx =0.0f, lly=0.0f, llz=0.0f, urx=0.0f, ury=0.0f, urz=0.0f;
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	face->BBox(string, llx, lly, llz, urx, ury, urz);
	glPopAttrib();

	rect->minx = llx;
	rect->maxx = urx;
	rect->miny = -ury;
	rect->maxy = -lly;

	if (advances)
	{
		int length = strlen(string);
		*advances = new double[length];
		for (int i = 0; i < length; ++i)
		{
			(*advances)[i] = face->Advance(string);
		}
	}
}

void OglRenderer::attach(imageObj* img)
{
	context->makeCurrent();
	int* buffer = new int[4 * width * height];

	for (int i = 0; i < img->img.gd->sy; i++)
	{
		//gdFree( gd->tpixels[i] ); // 100ms
		img->img.gd->tpixels[i] = &buffer[i * img->width];
	}
	glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
}

GLubyte* OglRenderer::gd2ogl(gdImagePtr img)
{
	int sx = gdImageSX(img);
	int sy = gdImageSY(img);
	GLubyte* imgPixels = new GLubyte[sx*sy*4];
	GLubyte* rowptr = imgPixels;
	for (int i = 0; i < sy; ++i)
	{
		for (int j = 0; j < sx; ++j)
		{
			ms_uint32 pixel = gdImageGetTrueColorPixel(img, j, i);
			rowptr[j * 4 + 0] = gdTrueColorGetRed(pixel);
			rowptr[j * 4 + 1] = gdTrueColorGetGreen(pixel);
			rowptr[j * 4 + 2] = gdTrueColorGetBlue(pixel);
			rowptr[j * 4 + 3] = (127 - gdTrueColorGetAlpha(pixel)) * 2;
		}
		rowptr += 4 * sx;
	}
	return imgPixels;
}

void OglRenderer::setTransparency(double transparency)
{
	this->transparency = transparency;
}

void OglRenderer::setColor(colorObj *color)
{
	if (color && MS_VALID_COLOR(*color))
	{
		glColor4d((double) color->red / 255, (double) color->green / 255, (double) color->blue / 255, transparency);
	}
	else
	{
		glColor4d(0.0, 0.0, 0.0, 1.0);
	}
}

void OglRenderer::drawVectorLineStrip(symbolObj *symbol, double width)
{
	glPushMatrix();
	glScaled(0.1, 0.1, 0);
	glLineWidth(width);
	glBegin(GL_LINE_STRIP);
    for (int i = 0; i < symbol->numpoints; i++)
	{
		if (symbol->points[i].x < 0 && symbol->points[i].y < 0)
		{
			glEnd();
			glBegin(GL_LINE_STRIP);
		}
		else
		{
			glVertex2d(symbol->points[i].x*10, symbol->points[i].y*10);
		}
	}
    glEnd();

    glPopMatrix();
}

double OglRenderer::drawTriangles(pointObj* p1, pointObj* p2, double width, double tilelength, double textureStart)
{
	double dx = p1->x - p2->x;
	double dy = p1->y - p2->y;
	double dist = sqrt(dx * dx + dy * dy);
	double theta;
	if (dx != 0)
		theta = atan(dy / dx);
	else
		theta = (dy > 0 ? 1 : -1) * OGL_PI / 2;

	double xw = sin(theta) * width / 2;
	double yw = cos(theta) * width / 2;

	double textureEnd = dist / tilelength;

	glTexCoord2d(textureStart, 1);
	glVertex2d(p1->x + xw, p1->y - yw); // Bottom Left Of The Texture and Quad
	glTexCoord2d(textureStart, 0);
	glVertex2d(p1->x - xw, p1->y + yw); // Bottom Right Of The Texture and Quad
	glTexCoord2d(textureStart + textureEnd, 1);
	glVertex2d(p2->x + xw, p2->y - yw); // Top Right Of The Texture and Quad
	glTexCoord2d(textureStart + textureEnd, 0);
	glVertex2d(p2->x - xw, p2->y + yw); // Top Left Of The Texture and Quad

	return dist;
}

double OglRenderer::drawQuad(pointObj* p1, pointObj* p2, double width, double tilelength, double textureStart)
{
	double dx = p1->x - p2->x;
	double dy = p1->y - p2->y;
	double dist = sqrt(dx * dx + dy * dy);
	double theta;
	if (dx)
		theta = atan(dy / dx);
	else
		theta = (dy > 0 ? 1 : -1) * OGL_PI / 2;

	glPushMatrix();
	glTranslated(p1->x, p1->y, 0);
	glRotated((p1->x < p2->x ? 0 : 180) + theta * (180 / OGL_PI), 0, 0, 1);
	glTranslated(-p1->x, -p1->y, 0);

	glBegin(GL_QUADS);
	glTexCoord2d(textureStart, 1);
	glVertex2d(p1->x, p1->y - width / 2); // Bottom Left Of The Texture and Quad
	glTexCoord2d(textureStart + (dist / tilelength), 1);
	glVertex2d(p1->x + dist, p1->y - width / 2); // Bottom Right Of The Texture and Quad
	glTexCoord2d(textureStart + (dist / tilelength), 0);
	glVertex2d(p1->x + dist, p1->y + width / 2); // Top Right Of The Texture and Quad
	glTexCoord2d(textureStart, 0);
	glVertex2d(p1->x, p1->y + width / 2); // Top Left Of The Texture and Quad
	glEnd();

	glPopMatrix();

	return dist;
}

void OglRenderer::renderTile(OglCache* tile, double x, double y, double angle)
{
	makeCurrent();
	glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
	glBindTexture(GL_TEXTURE_2D, tile->texture);
	glPushMatrix();
	glTranslated(x, y, 0);
	if (angle)
	{
		glRotated(angle, 0, 0, 1);
	}
	glTranslated(-x, -y, 0);
	glBegin(GL_QUADS);
		glTexCoord2d(0, 1); glVertex2d( x-(tile->width/2), y+(tile->height/2));	// Bottom Left Of The Texture and Quad
		glTexCoord2d(1, 1); glVertex2d( x+(tile->width/2), y+(tile->height/2));	// Bottom Right Of The Texture and Quad
		glTexCoord2d(1, 0); glVertex2d( x+(tile->width/2), y-(tile->height/2));	// Top Right Of The Texture and Quad
		glTexCoord2d(0, 0); glVertex2d( x-(tile->width/2), y-(tile->height/2));	// Top Left Of The Texture and Quad
	glEnd();

	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
}

void OglRenderer::renderPolylineTile(shapeObj *shape, OglCache* tile){
	makeCurrent();
	glBindTexture(GL_TEXTURE_2D, tile->texture); // Select Our Texture
	glBegin(GL_TRIANGLE_STRIP);

	double place = 0.0;
	for (int i = 0; i < shape->numlines; i++)
	{
		for (int j = 0; j < shape->line[i].numpoints - 1; j++)
		{
			double dist = drawTriangles(&shape->line[i].point[j], &shape->line[i].point[j + 1], tile->height, tile->width, place / tile->width);
			place = (place + dist);
			while (place >= tile->width) place -= tile->width;
		}
	}
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
}


void OglRenderer::renderVectorSymbol(double x, double y, symbolObj *symbol, double scale, double angle, colorObj *c, colorObj *oc, double ow)
{
	makeCurrent();
	glPushMatrix();
	glTranslated(x, y, 0);
	glRotated(angle, 0, 0, 1);
	glScaled(scale, scale, 0);

	if (oc != NULL && MS_VALID_COLOR(*oc) && ow > 0)
	{
		setColor(oc);
		drawVectorLineStrip(symbol, ow);
	}

	setColor(c);
    drawVectorLineStrip(symbol, ow);

    glPopMatrix();
}

void OglRenderer::renderPolyline(shapeObj *p, colorObj *c, double width,
		int patternlength, double* pattern, int lineCap, int joinStyle,
		colorObj *outlinecolor, double outlinewidth)
{
	makeCurrent();
	if (outlinecolor)
	{
		glEnable(GL_DEPTH_TEST);
		glPushMatrix();
		glTranslated(0, 0, 1);
	}

	setColor(c);

	if (patternlength > 0)
	{
		if (p->renderer_cache == NULL)
		{
			loadLine(p, width, patternlength, pattern);
		}
		OglCache* cache = (OglCache*) p->renderer_cache;
		GLuint texture = cache->texture;
		if (cache->patternDistance > 0)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			double place = 0;
			glBegin(GL_TRIANGLE_STRIP);
			for (int j = 0; j < p->numlines; j++)
			{
				for (int i = 0; i < p->line[j].numpoints - 1; i++)
				{
					double dist = drawTriangles(&p->line[j].point[i],
							&p->line[j].point[i + 1], width,
							cache->patternDistance, place
									/ cache->patternDistance);
					place = (place + dist);
					while (place >= cache->patternDistance)
						place -= cache->patternDistance; // better than mod, no rounding
				}
			}
			glEnd();
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
	else
	{
		float widthRange[2];
		glGetFloatv(GL_LINE_WIDTH_RANGE, widthRange);
		bool quads = !(width >= widthRange[0] && width <= widthRange[1]);
		double circleScale = width / SHAPE_CIRCLE_RADIUS / 2;
		glLineWidth(width);
		if (quads || (width > 1.0 && (lineCap == MS_CJC_ROUND || joinStyle == MS_CJC_ROUND)))
		{
			for (int j = 0; j < p->numlines; j++)
			{
				for (int i = 0; i < p->line[j].numpoints; i++)
				{
					if (i != p->line[j].numpoints - 1)
					{
						if (quads)
						{
							drawQuad(&p->line[j].point[i], &p->line[j].point[i
									+ 1], width);
						}
						else
						{
							glBegin(GL_LINES);
							glVertex2f(p->line[j].point[i].x, p->line[j].point[i].y);
							glVertex2f(p->line[j].point[i + 1].x,p->line[j].point[i + 1].y);
							glEnd();
						}
					}
					if ((lineCap == MS_CJC_ROUND && (i == 0 || i == p->line[j].numpoints - 1)) ||
						(joinStyle == MS_CJC_ROUND && (i > 0 && i < p->line[j].numpoints - 1)))
					{
						glPushMatrix();
						glTranslated(p->line[j].point[i].x, p->line[j].point[i].y, 0);
						glScaled(circleScale, circleScale, 0);
						glCallList(shapes[circle]);
						glPopMatrix();
					}
				}
			}
		}
		else
		{
			for (int j = 0; j < p->numlines; j++)
			{
				glBegin(GL_LINE_STRIP);
				for (int i = 0; i < p->line[j].numpoints; i++)
				{
					glVertex2f(p->line[j].point[i].x, p->line[j].point[i].y);
				}
				glEnd();

			}
		}
	}

	if (outlinecolor != NULL && MS_VALID_COLOR(*outlinecolor))
	{
		glPopMatrix();
		double owidth = MS_MAX(width + OUTLINE_WIDTH, outlinewidth);
		glPushMatrix();
		glTranslated(0, 0, -1);
		renderPolyline(p, outlinecolor, owidth, patternlength, pattern, lineCap, joinStyle);
		glPopMatrix();
		glDisable(GL_DEPTH_TEST);
	}

}

void OglRenderer::renderPolygon(shapeObj *p, colorObj *color, colorObj *outlinecolor, double outlinewidth, OglCache* tile, int lineCap, int joinStyle)
{
	/*
	 OpenGL cannot draw complex polygons so we need to use a Tessallator to draw the polygon using a GL_TRIANGLE_FAN

	 ISSUE: There's a problem here. It would seem that changing the dimensions or bounding box area breaks the code.
	 Reports need for a combine callback.
	 */
	makeCurrent();
	std::vector<GLdouble*> pointers;
	double texWidth = 0;
	double texHeight = 0;

	if (tile)
	{
		glBindTexture(GL_TEXTURE_2D, tile->texture); // Select Our Texture
		texWidth = tile->width;
		texHeight = tile->height;
	}
	setColor(color);
	gluTessBeginPolygon(tess, NULL );
	for (int j = 0; j < p->numlines; j++)
	{
		gluTessBeginContour(tess);
		for (int i = 0; i < p->line[j].numpoints; i++)
		{
			// create temp array and place
			GLdouble* dbls = new GLdouble[5];
			pointers.push_back(dbls);
			dbls[0] = p->line[j].point[i].x;
			dbls[1] = p->line[j].point[i].y;
			dbls[2] = 0.0;
			dbls[3] = texWidth;
			dbls[4] = texHeight;
			gluTessVertex(tess, dbls, dbls);
		}
		gluTessEndContour(tess);

	}
	gluTessEndPolygon(tess);

	// destroy temp arrays
	for (std::vector<GLdouble*>::iterator iter = pointers.begin(); iter
			!= pointers.end(); iter++)
	{
		delete[] *iter;
	}

	glBindTexture(GL_TEXTURE_2D, 0); // Select Our Texture

	if (outlinecolor != NULL && MS_VALID_COLOR(*outlinecolor) && outlinewidth > 0)
	{
		renderPolyline(p, outlinecolor, outlinewidth, 0, NULL, lineCap,
				joinStyle);
	}
}

void OglRenderer::renderGlyphs(double x, double y, colorObj *color,
		colorObj *outlinecolor, double size, char* font, char *thechars, double angle,
		colorObj *shadowcolor, double shdx, double shdy)
{
	makeCurrent();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	FTFont* face = getFTFont(font, size);

	glPushMatrix();
	glTranslated(floor(x), floor(y), 0);

	glScaled(1.0, -1.0, 1);
	glRotated(angle * (180 / OGL_PI), 0, 0, 1);
	if (outlinecolor && MS_VALID_COLOR(*outlinecolor))
	{
		setColor(outlinecolor);
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				if (i || j)
				{
					glPushMatrix();
					glTranslated(i, j, 0);
					face->Render(thechars);
					glPopMatrix();
				}
			}
		}
	}
	if (color != NULL && MS_VALID_COLOR(*color))
	{
		setColor(color);
		face->Render(thechars);
	}
	glPopMatrix();
	glPopAttrib();
}

void OglRenderer::renderPixmap(symbolObj *symbol, double x, double y,
		double angle, double scale)
{
	makeCurrent();

	float zoomX, zoomY;
	glGetFloatv(GL_ZOOM_X, &zoomX);
	glGetFloatv(GL_ZOOM_Y, &zoomY);

	GLubyte* imgdata = gd2ogl(symbol->img);
	glPixelZoom(zoomX*scale, zoomY*scale);
	glRasterPos2d(x, y);
	glDrawPixels(symbol->img->sx, symbol->img->sy, GL_RGBA, GL_UNSIGNED_BYTE, imgdata);
	glPixelZoom(zoomX, zoomY);
	delete imgdata;
}

void OglRenderer::renderEllipse(double x, double y, double angle, double width, double height,
		colorObj *color, colorObj *outlinecolor, double outlinewidth = 1.0)
{
	makeCurrent();
	if (outlinecolor && MS_VALID_COLOR(*outlinecolor))
	{
		glEnable(GL_DEPTH_TEST);
		glPushMatrix();
		glTranslated(0, 0, 1);
	}

	setColor(color);

	glPushMatrix();
	if (angle)
	{
		glRotated(angle, 0, 0, 1);
	}
	glTranslated(x, y, 0);
	glScaled(width / SHAPE_CIRCLE_RADIUS / 2, height / SHAPE_CIRCLE_RADIUS / 2, 1);
	glCallList(shapes[circle]);
	glPopMatrix();

	if (outlinecolor != NULL && MS_VALID_COLOR(*outlinecolor))
	{
		glPopMatrix();
		glPushMatrix();
		glTranslated(0, 0, -1);
		renderEllipse(x, y, angle, width + outlinewidth, height, outlinecolor, NULL, 0);
		glPopMatrix();
		glDisable(GL_DEPTH_TEST);
	}

}

bool OglRenderer::loadLine(shapeObj* shape, double width, int patternlength,
		double* pattern)
{
/*	OglCache* cache = (OglCache*) shape->renderer_cache;

	if (cache->patternDistance == 0)
	{
		for (int i = 0; i < patternlength; i++)
		{
			cache->patternDistance += pattern[i];
		}
	}
	GLubyte imgdata[TEX_SIZE * TEX_SIZE * 4];
	int pow2length = min(max(NextPowerOf2(cache->patternDistance), MIN_TEX_SIZE), TEX_SIZE);
	int pow2width = min(max(NextPowerOf2(width), MIN_TEX_SIZE), TEX_SIZE);

	double lengthScale = (double) pow2length / cache->patternDistance;
	glColor3d(0, 0, 0);
	double place = 0;
	double xRadii = width / 2 * lengthScale;
	double yRadii = pow2width / 2;
	for (int i = 0; i < patternlength; i++)
	{
		double next = pattern[i] * lengthScale;
		if (i % 2 == 0)
		{

			glPushMatrix();
			glTranslated(place + xRadii, yRadii, 0);
			glScaled(xRadii / SHAPE_CIRCLE_RADIUS,
					yRadii / SHAPE_CIRCLE_RADIUS, 0);
			glCallList(shapes[circle]);
			glPopMatrix();

			glBegin(GL_QUADS);
			glVertex2d(place + pow2width / 2, 0);
			glVertex2d(place + next - pow2width / 2, 0);
			glVertex2d(place + next - pow2width / 2, pow2width);
			glVertex2d(place + pow2width / 2, pow2width);
			glEnd();

			glPushMatrix();
			glTranslated(place + next - xRadii, yRadii, 0);
			glScaled(xRadii / SHAPE_CIRCLE_RADIUS,
					yRadii / SHAPE_CIRCLE_RADIUS, 0);
			glCallList(shapes[circle]);
			glPopMatrix();

		}
		place += next;
	}

	glReadPixels(0, 0, pow2length, pow2width, GL_RGBA, GL_UNSIGNED_BYTE,
			imgdata);
	//createTexture(pow2length, pow2width, GL_ALPHA, imgdata);*/
	return true;
}

FTFont* OglRenderer::getFTFont(char* font, double size)
{
	FTFont** face = &fontCache[font][size];
	if (*face == NULL)
	{
		*face = new FTGLTextureFont( font );
		(*face)->FaceSize(size*SIZE_RES);
	}
	return *face;
}

GLuint OglTexture::NextPowerOf2(GLuint in)
{
	in -= 1;

	in |= in >> 16;
	in |= in >> 8;
	in |= in >> 4;
	in |= in >> 2;
	in |= in >> 1;

	return in + 1;
}

GLvoid CALLBACK beginCallback(GLenum which)
{
	glBegin(which);
}

GLvoid CALLBACK errorCallback(GLenum errorCode)
{
	const GLubyte *estring;
	estring = gluErrorString(errorCode);
	fprintf(stderr, "Tessellation Error: %d %s\n", errorCode, estring);
	exit(0);
}

GLvoid CALLBACK endCallback(void)
{
	glEnd();
}

GLvoid CALLBACK combineDataCallback(GLdouble coords[3], GLdouble* vertex_data[4],
		GLfloat weight[4], void** dataOut, void* polygon_data)
{
	GLdouble *vertex;
	vertex = (GLdouble *) malloc(5 * sizeof(GLdouble));
	vertex[0] = coords[0];
	vertex[1] = coords[1];
	vertex[2] = coords[2];
	vertex[3] = vertex_data[0][3];
	vertex[4] = vertex_data[0][4];
	*dataOut = vertex;
}

GLvoid CALLBACK vertexCallback(GLdouble *vertex)
{
	if (vertex[3] > 0) glTexCoord2d(vertex[0]/vertex[3], vertex[1]/vertex[4]);
	glVertex3dv(vertex);
}

void OglRenderer::createShapes()
{
	double da = MS_MIN(OGL_PI / 2, OGL_PI / SHAPE_CIRCLE_RES);
	GLuint circle = glGenLists(1);
	glNewList(circle, GL_COMPILE);
	glBegin(GL_TRIANGLE_FAN);
	glVertex2d(0, 0);
	for (double a = 0.0; a <= 2 * OGL_PI; a += da)
	{
		glVertex2d(0 + cos(a) * SHAPE_CIRCLE_RADIUS, 0 + sin(a) * SHAPE_CIRCLE_RADIUS);
	}
	glVertex2d(0 + SHAPE_CIRCLE_RADIUS, 0);
	glEnd();

	glEndList();
	shapes.push_back(circle);
}

void OglRenderer::makeCurrent()
{
	context->makeCurrent();
}

#endif
