/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Functions copied from GDAL's CPL. This file contain utility
 *           functions that come from the GDAL/OGR cpl library. The idea
 *           behind it is to have access in mapserver to all these
 *           utilities, without being constrained to link with GDAL/OGR.
 * Author:   Y. Assefa, DM Solutions Group (assefa@dmsolutions.ca)
 *
 *
 * Note:
 * Names of functions used here are the same as those in the cpl
 * library with the exception the the CPL prefix is changed to ms
 * (eg : CPLGetBasename() would become msGetBasename())
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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

/* $Id$ */

#include <assert.h>
#include "mapserver.h"

/* should be size of largest possible filename */
#define MS_PATH_BUF_SIZE 2048
static char szStaticResult[MS_PATH_BUF_SIZE];

/************************************************************************/
/*                        msFindFilenameStart()                         */
/************************************************************************/

static int msFindFilenameStart(const char *pszFilename)

{
  int iFileStart;

  for (iFileStart = strlen(pszFilename);
       iFileStart > 0 && pszFilename[iFileStart - 1] != '/' &&
       pszFilename[iFileStart - 1] != '\\';
       iFileStart--) {
  }

  return iFileStart;
}

/************************************************************************/
/*                           msGetBasename()                            */
/************************************************************************/

/**
 * Extract basename (non-directory, non-extension) portion of filename.
 *
 * Returns a string containing the file basename portion of the passed
 * name.  If there is no basename (passed value ends in trailing directory
 * separator, or filename starts with a dot) an empty string is returned.
 *
 * <pre>
 * msGetBasename( "abc/def.xyz" ) == "def"
 * msGetBasename( "abc/def" ) == "def"
 * msGetBasename( "abc/def/" ) == ""
 * </pre>
 *
 * @param pszFullFilename the full filename potentially including a path.
 *
 * @return just the non-directory, non-extension portion of the path in
 * an internal string which must not be freed.  The string
 * may be destroyed by the next ms filename handling call.
 */

const char *msGetBasename(const char *pszFullFilename)

{
  int iFileStart = msFindFilenameStart(pszFullFilename);
  int iExtStart, nLength;

  for (iExtStart = strlen(pszFullFilename);
       iExtStart > iFileStart && pszFullFilename[iExtStart] != '.';
       iExtStart--) {
  }

  if (iExtStart == iFileStart)
    iExtStart = strlen(pszFullFilename);

  nLength = iExtStart - iFileStart;

  assert(nLength < MS_PATH_BUF_SIZE);

  strlcpy(szStaticResult, pszFullFilename + iFileStart, nLength + 1);

  return szStaticResult;
}

/* Id: GDAL/port/cplgetsymbol.cpp,v 1.14 2004/11/11 20:40:38 fwarmerdam Exp */
/* ==================================================================== */
/*                  Unix Implementation                                 */
/* ==================================================================== */
#if defined(HAVE_DLFCN_H)

#define GOT_GETSYMBOL

#include <dlfcn.h>

/************************************************************************/
/*                            msGetSymbol()                            */
/************************************************************************/

/**
 * Fetch a function pointer from a shared library / DLL.
 *
 * This function is meant to abstract access to shared libraries and
 * DLLs and performs functions similar to dlopen()/dlsym() on Unix and
 * LoadLibrary() / GetProcAddress() on Windows.
 *
 * If no support for loading entry points from a shared library is available
 * this function will always return NULL.   Rules on when this function
 * issues a msError() or not are not currently well defined, and will have
 * to be resolved in the future.
 *
 * Currently msGetSymbol() doesn't try to:
 * <ul>
 *  <li> prevent the reference count on the library from going up
 *    for every request, or given any opportunity to unload
 *    the library.
 *  <li> Attempt to look for the library in non-standard
 *    locations.
 *  <li> Attempt to try variations on the symbol name, like
 *    pre-prending or post-pending an underscore.
 * </ul>
 *
 * Some of these issues may be worked on in the future.
 *
 * @param pszLibrary the name of the shared library or DLL containing
 * the function.  May contain path to file.  If not system supplies search
 * paths will be used.
 * @param pszSymbolName the name of the function to fetch a pointer to.
 * @return A pointer to the function if found, or NULL if the function isn't
 * found, or the shared library can't be loaded.
 */

void *msGetSymbol(const char *pszLibrary, const char *pszSymbolName) {
  void *pLibrary;
  void *pSymbol;

  pLibrary = dlopen(pszLibrary, RTLD_LAZY);
  if (pLibrary == NULL) {
    msSetError(MS_MISCERR, "Dynamic loading failed: %s", "msGetSymbol()",
               dlerror());
    return NULL;
  }

  pSymbol = dlsym(pLibrary, pszSymbolName);

#if (defined(__APPLE__) && defined(__MACH__))
  /* On mach-o systems, C symbols have a leading underscore and depending
   * on how dlcompat is configured it may or may not add the leading
   * underscore.  So if dlsym() fails add an underscore and try again.
   */
  if (pSymbol == NULL) {
    char withUnder[strlen(pszSymbolName) + 2];
    withUnder[0] = '_';
    withUnder[1] = 0;
    strcat(withUnder, pszSymbolName);
    pSymbol = dlsym(pLibrary, withUnder);
  }
#endif

  if (pSymbol == NULL) {
    msSetError(MS_MISCERR, "Dynamic loading failed: %s", "msGetSymbol()",
               dlerror());
    return NULL;
  }

  /* We accept leakage of pLibrary */
  /* coverity[leaked_storage] */
  return (pSymbol);
}

#endif /* def __unix__ && defined(HAVE_DLFCN_H) */

/* ==================================================================== */
/*                 Windows Implementation                               */
/* ==================================================================== */
#ifdef WIN32

#define GOT_GETSYMBOL

#include <windows.h>

/************************************************************************/
/*                            msGetSymbol()                            */
/************************************************************************/

void *msGetSymbol(const char *pszLibrary, const char *pszSymbolName) {
  void *pLibrary;
  void *pSymbol;

  pLibrary = LoadLibrary(pszLibrary);
  if (pLibrary == NULL) {
    msSetError(MS_MISCERR, "Can't load requested dynamic library: %s",
               "msGetSymbol()", pszLibrary);
    return NULL;
  }

  pSymbol = (void *)GetProcAddress((HINSTANCE)pLibrary, pszSymbolName);

  if (pSymbol == NULL) {
    msSetError(MS_MISCERR, "Can't find requested entry point: %s in lib %s",
               "msGetSymbol()", pszSymbolName, pLibrary);
    return NULL;
  }

  return (pSymbol);
}

#endif /* def _WIN32 */

/* ==================================================================== */
/*      Dummy implementation.                                           */
/* ==================================================================== */

#ifndef GOT_GETSYMBOL

/************************************************************************/
/*                            msGetSymbol()                            */
/*                                                                      */
/*      Dummy implementation.                                           */
/************************************************************************/

void *msGetSymbol(const char *pszLibrary, const char *pszEntryPoint) {
  msSetError(
      MS_MISCERR,
      "msGetSymbol(%s,%s) called.  Failed as this is stub implementation.",
      "msGetSymbol()", pszLibrary, pszEntryPoint);
  return NULL;
}
#endif
