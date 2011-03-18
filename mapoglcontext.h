/*
 * mapoglcontext.h
 *
 *  Created on: 11/01/2009
 *      Author: toby
 */

#ifndef MAPOGLCONTEXT_H_
#define MAPOGLCONTEXT_H_

#ifdef USE_OGL


#if defined(_WIN32) && !defined(__CYGWIN__)

#include <windows.h>
#include<GL/gl.h>
#include<GL/glu.h>

#include "opengl/glext.h"
#include "opengl/wglext.h"

typedef HDC OGL_WINDOW;
typedef HGLRC OGL_CONTEXT;
typedef HGLRC OGL_PBUFFER;

#define CALLBACK __stdcall

#else

#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>

typedef Display* OGL_WINDOW;
typedef GLXContext OGL_CONTEXT;
typedef GLXPbuffer OGL_PBUFFER;

#define CALLBACK

#endif

class OglRenderer;

class OglContext {
private:
	static OglContext* current;
	static bool initWindow();
	static bool initSharingContext();

#if defined(_WIN32) && !defined(__CYGWIN__)
	static HDC window;
	static HGLRC sharingContext;
	HDC hPBufferDC;
	HGLRC hPBufferRC;
#else
    static Display* window;
    static GLXContext sharingContext;
    static GLXFBConfig* configs;
	GLXPbuffer pbuffer;
#endif

public:
	static ms_uint32 MAX_MULTISAMPLE_SAMPLES;
	static ms_uint32 MAX_ANISOTROPY;
	static ms_uint32 MAX_TEXTURE_SIZE;
	static ms_uint32 MIN_TEXTURE_SIZE;

	OglContext(ms_uint32 width, ms_uint32 height);
	~OglContext();
	int getHeight() { return height; }
	int getWidth() { return width; }
	bool makeCurrent();
	void bindPBufferToTexture();
private:
	bool createPBuffer(ms_uint32 width, ms_uint32 height);
	ms_uint32 width;
	ms_uint32 height;
	int windowPixelFormat;

#if defined(_WIN32) && !defined(__CYGWIN__)
	static PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB ;
	static PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB ;
	static PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB ;
	static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB ;
	static PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB ;
	static PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB ;
	static PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB ;
	static PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB ;
	static PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB ;
	static PFNWGLBINDTEXIMAGEARBPROC wglBindTexImageARB ;
	static PFNWGLRELEASETEXIMAGEARBPROC wglReleaseTexImageARB ;
	static PFNWGLMAKECONTEXTCURRENTARBPROC wglMakeContextCurrentARB;
	static PFNWGLGETPIXELFORMATATTRIBIVEXTPROC wglGetPixelFormatAttribivEXT;

	static PFNGLGENBUFFERSARBPROC wglGenBuffersARB;                     // VBO Name Generation Procedure
	static PFNGLBINDBUFFERARBPROC wglBindBufferARB;                     // VBO Bind Procedure
	static PFNGLBUFFERDATAARBPROC wglBufferDataARB;                     // VBO Data Loading Procedure
	static PFNGLBUFFERSUBDATAARBPROC wglBufferSubDataARB;               // VBO Sub Data Loading Procedure
	static PFNGLDELETEBUFFERSARBPROC wglDeleteBuffersARB;               // VBO Deletion Procedure
	static PFNGLGETBUFFERPARAMETERIVARBPROC wglGetBufferParameterivARB; // return various parameters of VBO
	static PFNGLMAPBUFFERARBPROC wglMapBufferARB;                       // map VBO procedure
	static PFNGLUNMAPBUFFERARBPROC wglUnmapBufferARB;                   // unmap VBO procedure

#endif
};

#endif /* USE_OGL */
#endif /* MAPOGLCONTEXT_H_ */
