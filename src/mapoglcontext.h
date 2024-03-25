/******************************************************************************
 * $id: mapoglcontext.h 7725 2011-04-09 15:56:58Z toby $
 *
 * Project:  MapServer
 * Purpose:  Various template processing functions.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2008 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef MAPOGLCONTEXT_H_
#define MAPOGLCONTEXT_H_

#ifdef USE_OGL

#if defined(_WIN32) && !defined(__CYGWIN__)

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "opengl/glext.h"
#include "opengl/wglext.h"

typedef HDC OGL_WINDOW;
typedef HGLRC OGL_CONTEXT;
typedef HGLRC OGL_PBUFFER;

#define CALLBACK __stdcall

#else

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

typedef Display *OGL_WINDOW;
typedef GLXContext OGL_CONTEXT;
typedef GLXPbuffer OGL_PBUFFER;

#define CALLBACK

#endif

class OglRenderer;

class OglContext {
private:
  static OglContext *current;
  static bool initWindow();
  static bool initSharingContext();
  static ms_uint32 getTextureSize(GLuint dimension, ms_uint32 value);
  static GLuint NextPowerOf2(GLuint in);

#if defined(_WIN32) && !defined(__CYGWIN__)
  static HDC window;
  static HGLRC sharingContext;
  HDC hPBufferDC;
  HGLRC hPBufferRC;
#else
  static Display *window;
  static GLXContext sharingContext;
  static GLXFBConfig *configs;
  GLXPbuffer pbuffer;
#endif

public:
  static ms_uint32 MAX_MULTISAMPLE_SAMPLES;
  static ms_uint32 MAX_ANISOTROPY;
  static ms_uint32 MAX_TEXTURE_SIZE;
  static ms_uint32 MIN_TEXTURE_SIZE;

  OglContext(ms_uint32 width, ms_uint32 height);
  ~OglContext();
  int getHeight() const { return height; }
  int getWidth() const { return width; }
  bool isValid() const { return valid; }
  bool makeCurrent();
  void bindPBufferToTexture();

private:
  bool createPBuffer(ms_uint32 width, ms_uint32 height);
  ms_uint32 width;
  ms_uint32 height;
  bool valid;
  int windowPixelFormat;

#if defined(_WIN32) && !defined(__CYGWIN__)
  static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
  static PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB;
  static PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB;
  static PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB;
  static PFNWGLMAKECONTEXTCURRENTARBPROC wglMakeContextCurrentARB;
#endif
};

#endif /* USE_OGL */
#endif /* MAPOGLCONTEXT_H_ */
