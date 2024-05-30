/******************************************************************************
 * $id: mapoglcontext.cpp 7725 2011-04-09 15:56:58Z toby $
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

#ifdef USE_OGL

#include "mapserver.h"
#include "maperror.h"
#include "mapoglcontext.h"

#define _T(x) __TEXT(x)

ms_uint32 OglContext::MAX_MULTISAMPLE_SAMPLES = 16;
ms_uint32 OglContext::MAX_ANISOTROPY = 0;
ms_uint32 OglContext::MAX_TEXTURE_SIZE = 0;
ms_uint32 OglContext::MIN_TEXTURE_SIZE = 0;
OglContext *OglContext::current = NULL;

OglContext::OglContext(ms_uint32 width, ms_uint32 height) : valid(false) {
  if (!window && !initWindow())
    return;
  if (!sharingContext && !initSharingContext())
    return;

  if (!(this->width = getTextureSize(GL_TEXTURE_WIDTH, width)))
    return;
  if (!(this->height = getTextureSize(GL_TEXTURE_HEIGHT, height)))
    return;

  if (!createPBuffer(this->width, this->height))
    return;
  if (!makeCurrent())
    return;
  valid = true;
}

ms_uint32 OglContext::getTextureSize(GLuint dimension, ms_uint32 value) {
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, value, value, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, NULL);
  GLint check = 0;
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, dimension, &check);

  if (glGetError() != GL_NO_ERROR) {
    msSetError(MS_OGLERR, "glGetTexLevelParameteriv failed. glError: %d",
               "OglContext::getTextureSize()", glGetError());
  }

  if (check == 0) {
    return MS_MAX(MS_MIN(NextPowerOf2(value), OglContext::MAX_TEXTURE_SIZE),
                  OglContext::MIN_TEXTURE_SIZE);
  } else {
    msSetError(MS_OGLERR, "Unable to create opengl texture of map size",
               "OglContext::getTextureSize()");
    return value;
  }
}

GLuint OglContext::NextPowerOf2(GLuint in) {
  in -= 1;

  in |= in >> 16;
  in |= in >> 8;
  in |= in >> 4;
  in |= in >> 2;
  in |= in >> 1;

  return in + 1;
}

#if defined(_WIN32) && !defined(__CYGWIN__)

HDC OglContext::window = NULL;
HGLRC OglContext::sharingContext = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  return (1L);
}

OglContext::~OglContext() {
  // TODO
}

bool OglContext::makeCurrent() {
  if (current != this) {
    current = this;
    if (!wglMakeContextCurrentARB(hPBufferDC, hPBufferDC, hPBufferRC)) {
      msSetError(MS_OGLERR, "Can't Activate The GL Rendering pbuffer.",
                 "OglContext::makeCurrent()");
      return FALSE;
    }
  }
  return true;
}

bool OglContext::initWindow() {
  int windowPixelFormat;
  WNDCLASS wc;
  DWORD dwExStyle;
  DWORD dwStyle;
  RECT WindowRect;
  HMODULE hInstance = NULL;
  HWND hWnd = NULL;
  WindowRect.left = (long)0;
  WindowRect.right = (long)0;
  WindowRect.top = (long)0;
  WindowRect.bottom = (long)0;
  hInstance = GetModuleHandle(NULL);
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.lpfnWndProc = (WNDPROC)WndProc;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = _T("OpenGL");

  if (!RegisterClass(&wc)) {
    msSetError(MS_OGLERR, "Failed To Register The Window Class.",
               "OglContext::initWindow()");
    return FALSE;
  }
  dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
  dwStyle = WS_OVERLAPPEDWINDOW;

  AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

  if (!(hWnd = CreateWindowEx(dwExStyle, _T("OpenGL"), _T("temp"),
                              dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0,
                              WindowRect.right - WindowRect.left,
                              WindowRect.bottom - WindowRect.top, NULL, NULL,
                              hInstance, NULL))) {
    msSetError(MS_OGLERR, "Window Creation Error.", "OglContext::initWindow()");
    return FALSE;
  }

  static PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,
      PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
      PFD_TYPE_RGBA,
      24, // Select Our Color Depth
      0,
      0,
      0,
      0,
      0,
      0, // Color Bits Ignored
      8, // Alpha Buffer
      0, // Shift Bit Ignored
      0, // No Accumulation Buffer
      0,
      0,
      0,
      0,              // Accumulation Bits Ignored
      16,             // 16Bit Z-Buffer (Depth Buffer)
      0,              // No Stencil Buffer
      0,              // No Auxiliary Buffer
      PFD_MAIN_PLANE, // Main Drawing Layer
      0,              // Reserved
      0,
      0,
      0 // Layer Masks Ignored
  };

  if (!(window = GetDC(hWnd))) {
    msSetError(MS_OGLERR, "Can't Create A GL Device Context.. Last Error: %d",
               "OglContext::initWindow()", GetLastError());
    return FALSE;
  }

  if (!(windowPixelFormat = ChoosePixelFormat(window, &pfd))) {
    msSetError(MS_OGLERR,
               "Can't Find A Suitable windowPixelFormat. Last Error: %d",
               "OglContext::initWindow()", GetLastError());
    return FALSE;
  }

  if (!SetPixelFormat(window, windowPixelFormat, &pfd)) {
    msSetError(MS_OGLERR, "Can't Set The windowPixelFormat. Last Error: %d",
               "OglContext::initWindow()", GetLastError());
    return FALSE;
  }
  return TRUE;
}

bool OglContext::initSharingContext() {
  if (!(sharingContext = wglCreateContext(window))) {
    msSetError(MS_OGLERR, "Can't Create A GL Rendering Context.",
               "OglContext::createContext()");
    return FALSE;
  }

  if (!wglMakeCurrent(window, sharingContext)) {
    msSetError(MS_OGLERR, "Can't Activate The GL Rendering Context.",
               "OglContext::createContext()");
    return FALSE;
  }

  if (!(wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)
            wglGetProcAddress("wglChoosePixelFormatARB"))) {
    msSetError(MS_OGLERR, "unable to retrieve wglChoosePixelFormatARB method.",
               "OglContext::createContext()");
    return FALSE;
  }

  if (!(wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress(
            "wglCreatePbufferARB"))) {
    msSetError(MS_OGLERR, "unable to retrieve wglCreatePbufferARB method.",
               "OglContext::createContext()");
    return FALSE;
  }

  if (!(wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress(
            "wglGetPbufferDCARB"))) {
    msSetError(MS_OGLERR, "unable to retrieve wglGetPbufferDCARB method.",
               "OglContext::createContext()");
    return FALSE;
  }

  if (!(wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)
            wglGetProcAddress("wglReleasePbufferDCARB"))) {
    msSetError(MS_OGLERR, "unable to retrieve wglReleasePbufferDCARB method.",
               "OglContext::createContext()");
    return FALSE;
  }

  if (!(wglMakeContextCurrentARB = (PFNWGLMAKECONTEXTCURRENTARBPROC)
            wglGetProcAddress("wglMakeContextCurrentARB"))) {
    msSetError(MS_OGLERR, "unable to retrieve wglMakeContextCurrentARB method.",
               "OglContext::createContext()");
    return FALSE;
  }

  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint *)&MAX_ANISOTROPY);
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *)&MAX_TEXTURE_SIZE);

  MIN_TEXTURE_SIZE = 8;

  return TRUE;
}

bool OglContext::createPBuffer(ms_uint32 width, ms_uint32 height) {
  HPBUFFERARB hPBuffer = NULL;
  hPBufferDC = NULL;
  hPBufferRC = NULL;

  int pixelFormat;
  int valid = false;
  UINT numFormats = 0;
  float fAttributes[] = {0, 0};
  int samples = MAX_MULTISAMPLE_SAMPLES;

  while ((!valid || numFormats == 0) && samples >= 0) {
    int iAttributes[] = {WGL_SAMPLES_ARB,
                         samples,
                         WGL_DRAW_TO_PBUFFER_ARB,
                         GL_TRUE,
                         WGL_SUPPORT_OPENGL_ARB,
                         GL_TRUE,
                         WGL_ACCELERATION_ARB,
                         WGL_FULL_ACCELERATION_ARB,
                         WGL_COLOR_BITS_ARB,
                         24,
                         WGL_ALPHA_BITS_ARB,
                         8,
                         WGL_DEPTH_BITS_ARB,
                         16,
                         WGL_STENCIL_BITS_ARB,
                         0,
                         WGL_SAMPLE_BUFFERS_ARB,
                         GL_TRUE,
                         WGL_PIXEL_TYPE_ARB,
                         WGL_TYPE_RGBA_ARB,
                         0,
                         0};

    valid = wglChoosePixelFormatARB(window, iAttributes, fAttributes, 1,
                                    &pixelFormat, &numFormats);
    if (!valid || numFormats == 0)
      samples -= 2;
  }

  if (numFormats == 0) {
    msSetError(MS_OGLERR,
               "P-buffer Error: Unable to find an acceptable pixel format.",
               "OglContext::createPBuffer()");
    return FALSE;
  }

  if (!(hPBuffer =
            wglCreatePbufferARB(window, pixelFormat, width, height, 0))) {
    msSetError(MS_OGLERR,
               "P-buffer Error: Unable to create P-buffer. glError: %d",
               "OglContext::createPBuffer()", glGetError());
    return FALSE;
  }
  if (!(hPBufferDC = wglGetPbufferDCARB(hPBuffer))) {
    msSetError(MS_OGLERR,
               "P-buffer Error: Unable to get P-buffer DC. glError: %d",
               "OglContext::createPBuffer()", glGetError());
    return FALSE;
  }
  if (!(hPBufferRC = wglCreateContext(hPBufferDC))) {
    msSetError(MS_OGLERR,
               "P-buffer Error: Unable to get P-buffer DC. glError: %d",
               "OglContext::createPBuffer()", glGetError());
    return FALSE;
  }

  if (wglShareLists(sharingContext, hPBufferRC) == FALSE) {
    msSetError(MS_OGLERR,
               "P-buffer Error: Unable to share display lists. glError: %d",
               "OglContext::createPBuffer()", glGetError());
    return FALSE;
  }

  return TRUE;
}

PFNWGLCHOOSEPIXELFORMATARBPROC OglContext::wglChoosePixelFormatARB = NULL;
PFNWGLCREATEPBUFFERARBPROC OglContext::wglCreatePbufferARB = NULL;
PFNWGLGETPBUFFERDCARBPROC OglContext::wglGetPbufferDCARB = NULL;
PFNWGLRELEASEPBUFFERDCARBPROC OglContext::wglReleasePbufferDCARB = NULL;
PFNWGLMAKECONTEXTCURRENTARBPROC OglContext::wglMakeContextCurrentARB = NULL;

#else /* UNIX */

Display *OglContext::window = NULL;
GLXContext OglContext::sharingContext = NULL;
GLXFBConfig *OglContext::configs = NULL;

OglContext::~OglContext() {
  // TODO
}

bool OglContext::makeCurrent() {
  if (current != this) {
    current = this;
    if (!glXMakeCurrent(window, pbuffer, sharingContext)) {
      msSetError(MS_OGLERR, "glXMakeCurrent failed. glError: %d",
                 "OglContext::makeCurrent()", glGetError());
      return false;
    }
  }
  return true;
}

bool OglContext::initSharingContext() {
  int fb_attributes[] = {GLX_SAMPLES_ARB,
                         MAX_MULTISAMPLE_SAMPLES,
                         GLX_RENDER_TYPE,
                         GLX_RGBA_BIT,
                         GLX_DRAWABLE_TYPE,
                         GLX_PBUFFER_BIT,
                         GLX_RED_SIZE,
                         8,
                         GLX_GREEN_SIZE,
                         8,
                         GLX_BLUE_SIZE,
                         8,
                         GLX_ALPHA_SIZE,
                         8,
                         GLX_DEPTH_SIZE,
                         16,
                         GLX_DOUBLEBUFFER,
                         0,
                         GLX_SAMPLE_BUFFERS_ARB,
                         1,
                         0};

  int num_configs = 0;
  while (num_configs == 0 && fb_attributes[1] >= 0) {
    configs = glXChooseFBConfig(window, DefaultScreen(window), fb_attributes,
                                &num_configs);
    fb_attributes[1] -= 2;
  }

  if (configs == NULL || num_configs == 0) {
    msSetError(MS_OGLERR,
               "glXChooseFBConfig could not find any configs. Likely your "
               "video card or drivers are not supported. glError: %d",
               "OglContext::init()", glGetError());
    return false;
  }

  sharingContext =
      glXCreateNewContext(window, *configs, GLX_RGBA_TYPE, NULL, 1);
  if (sharingContext == NULL) {
    msSetError(MS_OGLERR, "glXCreateNewContext failed. glError: %d",
               "OglContext::initSharingContext()", glGetError());
    return false;
  }

  return true;
}

bool OglContext::createPBuffer(ms_uint32 width, ms_uint32 height) {
  int maxHeight, maxWidth;

  glXGetFBConfigAttrib(window, *configs, GLX_MAX_PBUFFER_WIDTH, &maxWidth);
  glXGetFBConfigAttrib(window, *configs, GLX_MAX_PBUFFER_HEIGHT, &maxHeight);

  ms_uint32 uMaxHeight = maxHeight, uMaxWidth = maxWidth;

  this->width = MS_MIN(width, uMaxWidth);
  this->height = MS_MIN(height, uMaxHeight);

  int iPbufferAttributes[] = {GLX_PBUFFER_WIDTH,
                              this->width,
                              GLX_PBUFFER_HEIGHT,
                              this->height,
                              GLX_LARGEST_PBUFFER,
                              false,
                              0,
                              0};

  pbuffer = glXCreatePbuffer(window, *configs, iPbufferAttributes);
  if (pbuffer == 0) {
    msSetError(MS_OGLERR, "glXCreatePbuffer failed. glError: %d",
               "OglContext::init()", glGetError());
    return false;
  }

  return true;
}

bool OglContext::initWindow() {
  const char *const display_name = getenv("DISPLAY");
  if (!display_name) {
    msSetError(MS_OGLERR, "DISPLAY environment variable not set",
               "OglContext::init()");
    return false;
  }

  window = XOpenDisplay(display_name);
  if (!window) {
    msSetError(MS_OGLERR, "XOpenDisplay() failed.", "OglContext::init()");
    return false;
  }

  const int fb_attributes[] = {GLX_RENDER_TYPE,
                               GLX_RGBA_BIT,
                               GLX_DRAWABLE_TYPE,
                               GLX_PBUFFER_BIT,
                               GLX_RED_SIZE,
                               8,
                               GLX_GREEN_SIZE,
                               8,
                               GLX_BLUE_SIZE,
                               8,
                               GLX_ALPHA_SIZE,
                               8,
                               GLX_DEPTH_SIZE,
                               16,
                               GLX_DOUBLEBUFFER,
                               0,
                               GLX_SAMPLE_BUFFERS_ARB,
                               1,
                               0};

  int num_configs = 0;
  GLXFBConfig *tempConfigs = glXChooseFBConfig(window, DefaultScreen(window),
                                               fb_attributes, &num_configs);

  if (tempConfigs == NULL || num_configs == 0) {
    msSetError(MS_OGLERR,
               "glXChooseFBConfig could not find any configs. Likely your "
               "video card or drivers are not supported.",
               "OglContext::initWindow()");
    return false;
  }

  GLXContext tempContext =
      glXCreateNewContext(window, *tempConfigs, GLX_RGBA_TYPE, NULL, 1);
  if (tempContext == NULL) {
    msSetError(MS_OGLERR, "glXCreateNewContext failed.",
               "OglContext::initWindow()");
    return false;
  }

  int iPbufferAttributes[] = {0, 0};

  GLXPbuffer tempBuffer =
      glXCreatePbuffer(window, *tempConfigs, iPbufferAttributes);
  if (tempBuffer == 0) {
    msSetError(MS_OGLERR, "glXCreatePbuffer failed.",
               "OglContext::initWindow()");
    return false;
  }

  if (!glXMakeCurrent(window, tempBuffer, tempContext)) {
    msSetError(MS_OGLERR, "glXMakeCurrent failed.", "OglContext::initWindow()");
    return false;
  }

  glGetIntegerv(GL_MAX_SAMPLES_EXT, (GLint *)&MAX_MULTISAMPLE_SAMPLES);
  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint *)&MAX_ANISOTROPY);
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *)&MAX_TEXTURE_SIZE);

  MIN_TEXTURE_SIZE = 8;

  return true;
}

#endif /* UNIX */

#endif /* USE_OGL */
