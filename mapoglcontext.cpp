/*
 * mapoglcontext.cpp
 *
 *  Created on: 11/01/2009
 *      Author: toby
 */

#ifdef USE_OGL

#include "mapserver.h"
#include "maperror.h"
#include "mapoglcontext.h"

#define _T(x) __TEXT(x)

ms_uint32 OglContext::MAX_MULTISAMPLE_SAMPLES = 16;
ms_uint32 OglContext::MAX_ANISOTROPY = 0;
ms_uint32 OglContext::MAX_TEXTURE_SIZE = 0;
ms_uint32 OglContext::MIN_TEXTURE_SIZE = 0;
OglContext* OglContext::current = NULL;

OglContext::OglContext(ms_uint32 width, ms_uint32 height)
{
	if (!window) initWindow();
	if (!sharingContext) initSharingContext();
	createPBuffer(width, height);
	makeCurrent();
}

#if defined(_WIN32) && !defined(__CYGWIN__)

HDC OglContext::window = NULL;
HGLRC OglContext::sharingContext = NULL;

LRESULT CALLBACK WndProc(HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam)
{
	return(1L);
}

OglContext::~OglContext()
{
	//TODO
}

bool OglContext::makeCurrent()
{
	if (current != this)
	{
		current = this;
		if (!wglMakeContextCurrentARB(hPBufferDC, hPBufferDC, hPBufferRC))
		{
			msSetError(MS_OGLERR, "Can't Activate The GL Rendering pbuffer.", "OglContext::makeCurrent()");
			return FALSE;
		}
	}
	return true;
}

bool OglContext::initWindow()
{
	int windowPixelFormat;
	WNDCLASS wc;
	DWORD dwExStyle;
	DWORD dwStyle;
	RECT WindowRect;
	HMODULE hInstance = NULL;
	HWND hWnd = NULL;
	WindowRect.left=(long)0;
	WindowRect.right=(long)0;
	WindowRect.top=(long)0;
	WindowRect.bottom=(long)0;
	hInstance = GetModuleHandle(NULL);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpfnWndProc = (WNDPROC) WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = _T("OpenGL");

	if (!RegisterClass(&wc))
	{
		msSetError(MS_OGLERR, "Failed To Register The Window Class.", "OglContext::initWindow()");
		return FALSE;
	}
	dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	dwStyle=WS_OVERLAPPEDWINDOW;

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

	if (!(hWnd=CreateWindowEx( dwExStyle,
							_T("OpenGL"),
							_T("temp"),
							dwStyle |
							WS_CLIPSIBLINGS |
							WS_CLIPCHILDREN,
							0, 0,
							WindowRect.right-WindowRect.left,
							WindowRect.bottom-WindowRect.top,
							NULL,
							NULL,
							hInstance,
							NULL)))
	{
		msSetError(MS_OGLERR, "Window Creation Error.", "OglContext::initWindow()");
		return FALSE;
	}

	static PIXELFORMATDESCRIPTOR pfd=
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24, // Select Our Color Depth
		0, 0, 0, 0, 0, 0, // Color Bits Ignored
		8, // Alpha Buffer
		0, // Shift Bit Ignored
		0, // No Accumulation Buffer
		0, 0, 0, 0, // Accumulation Bits Ignored
		16, // 16Bit Z-Buffer (Depth Buffer)
		0, // No Stencil Buffer
		0, // No Auxiliary Buffer
		PFD_MAIN_PLANE, // Main Drawing Layer
		0, // Reserved
		0, 0, 0 // Layer Masks Ignored
	};

	if (!(window=GetDC(hWnd)))
	{
		msSetError(MS_OGLERR, "Can't Create A GL Device Context.", "OglContext::initWindow()");
		return FALSE;
	}

	if (!(windowPixelFormat=ChoosePixelFormat(window,&pfd)))
	{
		msSetError(MS_OGLERR, "Can't Find A Suitable windowPixelFormat.", "OglContext::initWindow()");
		return FALSE;
	}

	if(!SetPixelFormat(window,windowPixelFormat,&pfd))
	{
		msSetError(MS_OGLERR, "Can't Set The windowPixelFormat.", "OglContext::initWindow()");
		return FALSE;
	}
	return TRUE;
}

bool OglContext::initSharingContext()
{
	if (!(sharingContext=wglCreateContext(window)))
	{
		msSetError(MS_OGLERR, "Can't Create A GL Rendering Context.", "OglContext::createContext()");
		return FALSE;
	}

	if(!wglMakeCurrent(window,sharingContext))
	{
		msSetError(MS_OGLERR, "Can't Activate The GL Rendering Context.", "OglContext::createContext()");
		return FALSE;
	}

	wglMakeContextCurrentARB = (PFNWGLMAKECONTEXTCURRENTARBPROC)wglGetProcAddress("wglMakeContextCurrentARB");
	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
	wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
	wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
	wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
	wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
	wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
	wglQueryPbufferARB = (PFNWGLQUERYPBUFFERARBPROC)wglGetProcAddress("wglQueryPbufferARB");
	wglBindTexImageARB = (PFNWGLBINDTEXIMAGEARBPROC)wglGetProcAddress("wglBindTexImageARB");
	wglReleaseTexImageARB = (PFNWGLRELEASETEXIMAGEARBPROC)wglGetProcAddress("wglReleaseTexImageARB");
	wglGetPixelFormatAttribivEXT = (PFNWGLGETPIXELFORMATATTRIBIVEXTPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");

	wglGenBuffersARB = (PFNGLGENBUFFERSARBPROC)wglGetProcAddress("glGenBuffersARB");
	wglBindBufferARB = (PFNGLBINDBUFFERARBPROC)wglGetProcAddress("glBindBufferARB");
	wglBufferDataARB = (PFNGLBUFFERDATAARBPROC)wglGetProcAddress("glBufferDataARB");
	wglBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)wglGetProcAddress("glBufferSubDataARB");
	wglDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)wglGetProcAddress("glDeleteBuffersARB");
	wglGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)wglGetProcAddress("glGetBufferParameterivARB");
	wglMapBufferARB = (PFNGLMAPBUFFERARBPROC)wglGetProcAddress("glMapBufferARB");
	wglUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)wglGetProcAddress("glUnmapBufferARB");

	glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint*) &MAX_ANISOTROPY);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&MAX_TEXTURE_SIZE);

	MIN_TEXTURE_SIZE = 8;


	return TRUE;
}

bool OglContext::createPBuffer(ms_uint32 width, ms_uint32 height)
{
	HPBUFFERARB hPBuffer = NULL;
	hPBufferDC = NULL;
	hPBufferRC = NULL;

	int pixelFormat;
	int valid = false;
	UINT numFormats;
	float fAttributes[] = {0,0};
	int samples = MAX_MULTISAMPLE_SAMPLES;

	while (!valid && samples >= 0)
	{
		int iAttributes[] =
		{
			WGL_SAMPLES_ARB,samples,
			WGL_DRAW_TO_PBUFFER_ARB,GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
			WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
			WGL_COLOR_BITS_ARB,24,
			WGL_ALPHA_BITS_ARB,8,
			WGL_DEPTH_BITS_ARB,16,
			WGL_STENCIL_BITS_ARB,0,
			WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			0,0
		};

		valid = wglChoosePixelFormatARB(window,iAttributes,fAttributes,1,&pixelFormat,&numFormats);
	}
	if(numFormats == 0)
	{
		msSetError(MS_OGLERR, "P-buffer Error: Unable to find an acceptable pixel format.", "OglContext::createPBuffer()");
		return FALSE;
	}

	if (!(hPBuffer = wglCreatePbufferARB(window, pixelFormat, width, height, 0)))
	{
		msSetError(MS_OGLERR, "P-buffer Error: Unable to create P-buffer.", "OglContext::createPBuffer()");
		return FALSE;
	}
	if (!(hPBufferDC = wglGetPbufferDCARB(hPBuffer)))
	{
		msSetError(MS_OGLERR, "P-buffer Error: Unable to get P-buffer DC.", "OglContext::createPBuffer()");
		return FALSE;
	}
	if (!(hPBufferRC = wglCreateContext(hPBufferDC)))
	{
		msSetError(MS_OGLERR, "P-buffer Error: Unable to get P-buffer DC.", "OglContext::createPBuffer()");
		return FALSE;
	}

	if (wglShareLists(sharingContext,hPBufferRC) == FALSE)
	{
		msSetError(MS_OGLERR, "P-buffer Error: Unable to share display lists.", "OglContext::createPBuffer()");
		return FALSE;
	}

	return TRUE;
}

#else /* UNIX */

Display* OglContext::window = NULL;
GLXContext OglContext::sharingContext = NULL;
GLXFBConfig* OglContext::configs = NULL;

OglContext::~OglContext()
{
	//TODO
}


bool OglContext::makeCurrent()
{
	if (current != this)
	{
		current = this;
		glXMakeCurrent(window, pbuffer, sharingContext);
	}
	return true;
}

bool OglContext::initSharingContext()
{
	int fb_attributes[] =
	{
		GLX_SAMPLES_ARB, MAX_MULTISAMPLE_SAMPLES,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 16,
		GLX_DOUBLEBUFFER, 0,
		GLX_SAMPLE_BUFFERS_ARB, 1,
		0
	};

	int num_configs = 0;
	while (num_configs == 0 && fb_attributes[1] >= 0)
	{
		configs = glXChooseFBConfig(window, DefaultScreen(window), fb_attributes, &num_configs);
		fb_attributes[1] -= 2;
	}

	if (configs == NULL || num_configs == 0)
	{
		msSetError(MS_OGLERR, "glXChooseFBConfig could not find any configs.", "OglContext::init()");
		return false;
	}

	sharingContext = glXCreateNewContext(window, *configs, GLX_RGBA_TYPE, NULL, 1);
	if (sharingContext == NULL)
	{
		msSetError(MS_OGLERR, "glXCreateNewContext failed.", "OglContext::initSharingContext()");
		return false;
	}

	return true;
}

bool OglContext::createPBuffer(ms_uint32 width, ms_uint32 height)
{
	int maxHeight, maxWidth;

	glXGetFBConfigAttrib(window, *configs, GLX_MAX_PBUFFER_WIDTH, &maxWidth);
	glXGetFBConfigAttrib(window, *configs, GLX_MAX_PBUFFER_HEIGHT, &maxHeight);

	ms_uint32 uMaxHeight = maxHeight, uMaxWidth = maxWidth;

	this->width = MS_MIN(width, uMaxWidth);
	this->height = MS_MIN(height, uMaxHeight);

	int iPbufferAttributes[] =
	{
			GLX_PBUFFER_WIDTH, this->width,
			GLX_PBUFFER_HEIGHT, this->height,
			GLX_LARGEST_PBUFFER, false,
			0, 0
	};

	pbuffer = glXCreatePbuffer(window, *configs, iPbufferAttributes);
	if (pbuffer == 0)
	{
		msSetError(MS_OGLERR, "glXCreatePbuffer failed.", "OglContext::init()");
		return false;
	}

	return true;
}

bool OglContext::initWindow()
{
	const char* const display_name = getenv("DISPLAY");
	if (!display_name)
	{
		msSetError(MS_OGLERR, "DISPLAY environment variable not set", "OglContext::init()");
		return false;
	}

	window = XOpenDisplay(display_name);

	const int fb_attributes[] = {
			GLX_RENDER_TYPE, GLX_RGBA_BIT,
			GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
			GLX_RED_SIZE, 8,
			GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8,
			GLX_ALPHA_SIZE, 8,
			GLX_DEPTH_SIZE, 16,
			GLX_DOUBLEBUFFER, 0,
			GLX_SAMPLE_BUFFERS_ARB, 1,
			0
		};

	int num_configs = 0;
	GLXFBConfig* tempConfigs = glXChooseFBConfig(window, DefaultScreen(window), fb_attributes, &num_configs);

	if (tempConfigs == NULL || num_configs == 0)
	{
		msSetError(MS_OGLERR, "glXChooseFBConfig could not find any configs.", "OglContext::initWindow()");
		return false;
	}

	GLXContext tempContext = glXCreateNewContext(window, *tempConfigs, GLX_RGBA_TYPE, NULL, 1);
	if (tempContext == NULL)
	{
		msSetError(MS_OGLERR, "glXCreateNewContext failed.", "OglContext::initWindow()");
		return false;
	}

	int iPbufferAttributes[] =	{0, 0};

	GLXPbuffer tempBuffer = glXCreatePbuffer(window, *tempConfigs, iPbufferAttributes);
	if (tempBuffer == 0)
	{
		msSetError(MS_OGLERR, "glXCreatePbuffer failed.", "OglContext::initWindow()");
		return false;
	}

	glXMakeCurrent(window, tempBuffer, tempContext);

	glGetIntegerv(GL_MAX_SAMPLES_EXT, (GLint*)&MAX_MULTISAMPLE_SAMPLES);
	glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint*) &MAX_ANISOTROPY);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&MAX_TEXTURE_SIZE);

	MIN_TEXTURE_SIZE = 8;

	return true;
}

#endif /* WIN32 */

#if defined(_WIN32) && !defined(__CYGWIN__)

PFNWGLGETEXTENSIONSSTRINGARBPROC OglContext::wglGetExtensionsStringARB = NULL;
PFNWGLGETPIXELFORMATATTRIBIVARBPROC OglContext::wglGetPixelFormatAttribivARB = NULL;
PFNWGLGETPIXELFORMATATTRIBFVARBPROC OglContext::wglGetPixelFormatAttribfvARB = NULL;
PFNWGLCHOOSEPIXELFORMATARBPROC OglContext::wglChoosePixelFormatARB = NULL;
PFNWGLCREATEPBUFFERARBPROC OglContext::wglCreatePbufferARB = NULL;
PFNWGLGETPBUFFERDCARBPROC OglContext::wglGetPbufferDCARB = NULL;
PFNWGLRELEASEPBUFFERDCARBPROC OglContext::wglReleasePbufferDCARB = NULL;
PFNWGLDESTROYPBUFFERARBPROC OglContext::wglDestroyPbufferARB = NULL;
PFNWGLQUERYPBUFFERARBPROC OglContext::wglQueryPbufferARB = NULL;
PFNWGLBINDTEXIMAGEARBPROC OglContext::wglBindTexImageARB = NULL;
PFNWGLRELEASETEXIMAGEARBPROC OglContext::wglReleaseTexImageARB = NULL;
PFNWGLMAKECONTEXTCURRENTARBPROC OglContext::wglMakeContextCurrentARB = NULL;
PFNWGLGETPIXELFORMATATTRIBIVEXTPROC OglContext::wglGetPixelFormatAttribivEXT = NULL;

PFNGLGENBUFFERSARBPROC OglContext::wglGenBuffersARB = 0; // VBO Name Generation Procedure
PFNGLBINDBUFFERARBPROC OglContext::wglBindBufferARB = 0; // VBO Bind Procedure
PFNGLBUFFERDATAARBPROC OglContext::wglBufferDataARB = 0; // VBO Data Loading Procedure
PFNGLBUFFERSUBDATAARBPROC OglContext::wglBufferSubDataARB = 0; // VBO Sub Data Loading Procedure
PFNGLDELETEBUFFERSARBPROC OglContext::wglDeleteBuffersARB = 0; // VBO Deletion Procedure
PFNGLGETBUFFERPARAMETERIVARBPROC OglContext::wglGetBufferParameterivARB = 0; // return various parameters of VBO
PFNGLMAPBUFFERARBPROC OglContext::wglMapBufferARB = 0; // map VBO procedure
PFNGLUNMAPBUFFERARBPROC OglContext::wglUnmapBufferARB = 0; // unmap VBO procedure

#endif

/*
HDC OglContext::hWINDOWDC = NULL;
HGLRC OglContext::hWINDOWRC = NULL;
int OglContext::windowPixelFormat = 0;

HPBUFFERARB OglContext::hPBuffer = NULL;
HDC OglContext::hPBufferDC = NULL;
HGLRC OglContext::hPBufferRC = NULL;

// P-Buffer for texture rendering.
HPBUFFERARB OglContext::hTexPBuffer = NULL;
HDC OglContext::hTexPBufferDC = NULL;
HGLRC OglContext::hTexPBufferRC = NULL;

bool OglContext::init()
{
	WNDCLASS wc;
	DWORD dwExStyle;
	DWORD dwStyle;
	RECT WindowRect;
	HMODULE hInstance = NULL;
	HWND hWnd = NULL;
	WindowRect.left=(long)0;
	WindowRect.right=(long)0;
	WindowRect.top=(long)0;
	WindowRect.bottom=(long)0;
	hInstance = GetModuleHandle(NULL);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpfnWndProc = (WNDPROC) WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"OpenGL";

	if (!RegisterClass(&wc))
	{
		printError("Failed To Register The Window Class.");
		return FALSE;
	}
	dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	dwStyle=WS_OVERLAPPEDWINDOW;

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

	if (!(hWnd=CreateWindowEx( dwExStyle,
							L"OpenGL",
							L"temp",
							dwStyle |
							WS_CLIPSIBLINGS |
							WS_CLIPCHILDREN,
							0, 0,
							WindowRect.right-WindowRect.left,
							WindowRect.bottom-WindowRect.top,
							NULL,
							NULL,
							hInstance,
							NULL)))
	{
		printError("Window Creation Error.");
		return FALSE;
	}

	static PIXELFORMATDESCRIPTOR pfd=
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24, // Select Our Color Depth
		0, 0, 0, 0, 0, 0, // Color Bits Ignored
		8, // Alpha Buffer
		0, // Shift Bit Ignored
		0, // No Accumulation Buffer
		0, 0, 0, 0, // Accumulation Bits Ignored
		16, // 16Bit Z-Buffer (Depth Buffer)
		0, // No Stencil Buffer
		0, // No Auxiliary Buffer
		PFD_MAIN_PLANE, // Main Drawing Layer
		0, // Reserved
		0, 0, 0 // Layer Masks Ignored
	};

	if (!(hWINDOWDC=GetDC(hWnd)))
	{
		printError("Can't Create A GL Device Context.");
		return FALSE;
	}

	if (!(windowPixelFormat=ChoosePixelFormat(hWINDOWDC,&pfd)))
	{
		printError("Can't Find A Suitable windowPixelFormat.");
		return FALSE;
	}

	if(!SetPixelFormat(hWINDOWDC,windowPixelFormat,&pfd))
	{
		printError("Can't Set The windowPixelFormat.");
		return FALSE;
	}

	if (!(hWINDOWRC=wglCreateContext(hWINDOWDC)))
	{
		printError("Can't Create A GL Rendering Context.");
		return FALSE;
	}

	if(!wglMakeCurrent(hWINDOWDC,hWINDOWRC))
	{
		printError("Can't Activate The GL Rendering Context.");
		return FALSE;
	}
	wglMakeContextCurrentARB = (PFNWGLMAKECONTEXTCURRENTARBPROC)wglGetProcAddress("wglMakeContextCurrentARB");
	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
	wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
	wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
	wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
	wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
	wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
	wglQueryPbufferARB = (PFNWGLQUERYPBUFFERARBPROC)wglGetProcAddress("wglQueryPbufferARB");
	wglBindTexImageARB = (PFNWGLBINDTEXIMAGEARBPROC)wglGetProcAddress("wglBindTexImageARB");
	wglReleaseTexImageARB = (PFNWGLRELEASETEXIMAGEARBPROC)wglGetProcAddress("wglReleaseTexImageARB");
	wglGetPixelFormatAttribivEXT = (PFNWGLGETPIXELFORMATATTRIBIVEXTPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");

	wglGenBuffersARB = (PFNGLGENBUFFERSARBPROC)wglGetProcAddress("glGenBuffersARB");
	wglBindBufferARB = (PFNGLBINDBUFFERARBPROC)wglGetProcAddress("glBindBufferARB");
	wglBufferDataARB = (PFNGLBUFFERDATAARBPROC)wglGetProcAddress("glBufferDataARB");
	wglBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)wglGetProcAddress("glBufferSubDataARB");
	wglDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)wglGetProcAddress("glDeleteBuffersARB");
	wglGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)wglGetProcAddress("glGetBufferParameterivARB");
	wglMapBufferARB = (PFNGLMAPBUFFERARBPROC)wglGetProcAddress("glMapBufferARB");
	wglUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)wglGetProcAddress("glUnmapBufferARB");

	loadShapes();

	hPBufferRC = NULL;
	hPBufferDC = NULL;
	hPBuffer = NULL;
	int pixelFormat;
	int valid;
	UINT numFormats;
	float fAttributes[] =
	{	0,0};
	int iAttributes[] =
	{
		WGL_DRAW_TO_PBUFFER_ARB,GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
		WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB,24,
		WGL_ALPHA_BITS_ARB,8,
		WGL_DEPTH_BITS_ARB,16,
		WGL_STENCIL_BITS_ARB,0,
		WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
		WGL_SAMPLES_ARB,MUTLISAMPLE_SAMPLES,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		0,0
	};

	valid = wglChoosePixelFormatARB(hWINDOWDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats);
	if(numFormats == 0)
	{
		printError("P-buffer Error: Unable to find an acceptable pixel format");
		return FALSE;
	}

	if (!(hPBuffer = wglCreatePbufferARB(hWINDOWDC, pixelFormat, MAX_WIDTH, MAX_HEIGHT, 0)))
	{
		printError("P-buffer Error: Unable to create P-buffer");
		return FALSE;
	}
	if (!(hPBufferDC = wglGetPbufferDCARB(hPBuffer)))
	{
		printError("P-buffer Error: Unable to get P-buffer DC");
		return FALSE;
	}
	if (!(hPBufferRC = wglCreateContext(hPBufferDC)))
	{
		printError("P-buffer Error: Unable to get P-buffer DC");
		return FALSE;
	}

	if (wglShareLists(hWINDOWRC,hPBufferRC) == FALSE)
	{
		printError("P-buffer Error: Unable to share display lists");
		return FALSE;
	}

	initialised = true;
}

PFNWGLGETEXTENSIONSSTRINGARBPROC OglContext::wglGetExtensionsStringARB = NULL;
PFNWGLGETPIXELFORMATATTRIBIVARBPROC OglContext::wglGetPixelFormatAttribivARB = NULL;
PFNWGLGETPIXELFORMATATTRIBFVARBPROC OglContext::wglGetPixelFormatAttribfvARB = NULL;
PFNWGLCHOOSEPIXELFORMATARBPROC OglContext::wglChoosePixelFormatARB = NULL;
PFNWGLCREATEPBUFFERARBPROC OglContext::wglCreatePbufferARB = NULL;
PFNWGLGETPBUFFERDCARBPROC OglContext::wglGetPbufferDCARB = NULL;
PFNWGLRELEASEPBUFFERDCARBPROC OglContext::wglReleasePbufferDCARB = NULL;
PFNWGLDESTROYPBUFFERARBPROC OglContext::wglDestroyPbufferARB = NULL;
PFNWGLQUERYPBUFFERARBPROC OglContext::wglQueryPbufferARB = NULL;
PFNWGLBINDTEXIMAGEARBPROC OglContext::wglBindTexImageARB = NULL;
PFNWGLRELEASETEXIMAGEARBPROC OglContext::wglReleaseTexImageARB = NULL;
PFNWGLMAKECONTEXTCURRENTARBPROC OglContext::wglMakeContextCurrentARB = NULL;
PFNWGLGETPIXELFORMATATTRIBIVEXTPROC OglContext::wglGetPixelFormatAttribivEXT = NULL;

PFNGLGENBUFFERSARBPROC OglContext::wglGenBuffersARB = 0; // VBO Name Generation Procedure
PFNGLBINDBUFFERARBPROC OglContext::wglBindBufferARB = 0; // VBO Bind Procedure
PFNGLBUFFERDATAARBPROC OglRenderer::wglBufferDataARB = 0; // VBO Data Loading Procedure
PFNGLBUFFERSUBDATAARBPROC OglContext::wglBufferSubDataARB = 0; // VBO Sub Data Loading Procedure
PFNGLDELETEBUFFERSARBPROC OglContext::wglDeleteBuffersARB = 0; // VBO Deletion Procedure
PFNGLGETBUFFERPARAMETERIVARBPROC OglContext::wglGetBufferParameterivARB = 0; // return various parameters of VBO
PFNGLMAPBUFFERARBPROC OglContext::wglMapBufferARB = 0; // map VBO procedure
PFNGLUNMAPBUFFERARBPROC OglContext::wglUnmapBufferARB = 0; // unmap VBO procedure

LRESULT CALLBACK WndProc(HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam)
{
	return(1L);
}

#endif
*/

#endif /* USE_OGL */
