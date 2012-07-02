/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Interface Python file-like objects with GD through IOCtx
 * Author:   Sean Gillies, sgillies@frii.com
 *
 ******************************************************************************
 * The PyFileIfaceObj_IOCtx API is
 *
 * Copyright 1995 Richard Jones, Bureau of Meteorology Australia.
 * richard@bofh.asn.au
 *
 * Current maintainer is
 * Chris Gonnerman <chris.gonnerman@newcenturycomputers.net>
 * Please direct all questions and problems to me.
 *
 ******************************************************************************
 * GDMODULE LICENSE:
 * gdmodule - Python GD module Copyright (c) 1995 Richard Jones All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.  Neither the name of the Bureau of Meteorology
 * Australia nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ***************************************************************************/

#ifdef USE_GD

#include "pygdioctx.h"

int PyFileIfaceObj_IOCtx_GetC(gdIOCtx *ctx)
{
  struct PyFileIfaceObj_gdIOCtx *pctx = (struct PyFileIfaceObj_gdIOCtx *)ctx;
  if (pctx->strObj) {
    Py_DECREF(pctx->strObj);
    pctx->strObj = NULL;
  }
  pctx->strObj = PyObject_CallMethod(pctx->fileIfaceObj, "read", "i", 1);
  if (!pctx->strObj || !PyString_Check(pctx->strObj)) {
    return EOF;
  }
  if (PyString_GET_SIZE(pctx->strObj) == 1) {
    return (int)(unsigned char)PyString_AS_STRING(pctx->strObj)[0];
  }
  return EOF;
}

int PyFileIfaceObj_IOCtx_GetBuf(gdIOCtx *ctx, void *data, int size)
{
  int err;
  char *value;
  struct PyFileIfaceObj_gdIOCtx *pctx = (struct PyFileIfaceObj_gdIOCtx *)ctx;
  if (pctx->strObj) {
    Py_DECREF(pctx->strObj);
    pctx->strObj = NULL;
  }
  pctx->strObj = PyObject_CallMethod(pctx->fileIfaceObj, "read", "i", size);
  if (!pctx->strObj) {
    return 0;
  }
  err = PyString_AsStringAndSize(pctx->strObj, &value, &size);
  if (err < 0) {
    /* this throws away the python exception since the gd library
     * won't pass it up properly.  gdmodule should create its own
     * since the "file" couldn't be read properly.  */
    PyErr_Clear();
    return 0;
  }
  memcpy(data, value, size);
  return size;
}

void PyFileIfaceObj_IOCtx_Free(gdIOCtx *ctx)
{
  struct PyFileIfaceObj_gdIOCtx *pctx = (struct PyFileIfaceObj_gdIOCtx *)ctx;
  if (pctx->strObj) {
    Py_DECREF(pctx->strObj);
    pctx->strObj = NULL;
  }
  if (pctx->fileIfaceObj) {
    Py_DECREF(pctx->fileIfaceObj);
    pctx->fileIfaceObj = NULL;
  }
  /* NOTE: we leave deallocation of the ctx structure itself to outside
   * code for memory allocation symmetry.  This function is safe to
   * call multiple times (gd should call it + we call it to be safe). */
}

struct PyFileIfaceObj_gdIOCtx * alloc_PyFileIfaceObj_IOCtx(PyObject *fileIfaceObj) {
  struct PyFileIfaceObj_gdIOCtx *pctx;
  pctx = calloc(1, sizeof(struct PyFileIfaceObj_gdIOCtx));
  if (!pctx)
    return NULL;
  pctx->ctx.getC = PyFileIfaceObj_IOCtx_GetC;
  pctx->ctx.getBuf = PyFileIfaceObj_IOCtx_GetBuf;
  pctx->ctx.gd_free = PyFileIfaceObj_IOCtx_Free;
  Py_INCREF(fileIfaceObj);
  pctx->fileIfaceObj = fileIfaceObj;
  return pctx;
}

void free_PyFileIfaceObj_IOCtx(struct PyFileIfaceObj_gdIOCtx *pctx)
{
  if (!pctx)
    return;
  assert(pctx->ctx.gd_free != NULL);
  pctx->ctx.gd_free((gdIOCtxPtr)pctx);
  free(pctx);
}

/* ===========================================================================
 * Mapserver function createImageObjFromPyFile by Sean Gillies,
 * <sgillies@frii.com>
 * ======================================================================== */

imageObj *createImageObjFromPyFile(PyObject *file, const char *driver)
{
  imageObj *image=NULL;
  struct PyFileIfaceObj_gdIOCtx *pctx;

  if (file == Py_None) {
    msSetError(MS_IMGERR, "NULL file object",
               "createImageObjFromPyFile()");
    return NULL;
  } else if (!driver) {
    msSetError(MS_IMGERR, "NULL or invalid driver string",
               "createImageObjFromPyFile()");
    return NULL;
  } else {
    pctx = alloc_PyFileIfaceObj_IOCtx(file);
    //image = msImageLoadGDCtx((gdIOCtx *) pctx, driver);
    free_PyFileIfaceObj_IOCtx(pctx);
    return image;
  }
}

#endif

