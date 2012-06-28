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
 *****************************************************************************
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

#include <Python.h>
#include <gd.h>
#include <gdfonts.h>
#include <gdfontl.h>
#include <gdfontmb.h>
#include <gdfontt.h>
#include <gdfontg.h>
#include <string.h>
#include <errno.h>

#include "../../../mapserver.h"

/*
** Code to act as a gdIOCtx wrapper for a python file object
** (read-only; write support would not be difficult to add)
*/

struct PyFileIfaceObj_gdIOCtx {
  gdIOCtx ctx;
  PyObject *fileIfaceObj;
  PyObject *strObj;
};

struct PyFileIfaceObj_gdIOCtx * alloc_PyFileIfaceObj_IOCtx(PyObject *fileIfaceObj);
void free_PyFileIfaceObj_IOCtx(struct PyFileIfaceObj_gdIOCtx *pctx);
imageObj *createImageObjFromPyFile(PyObject *file, const char *driver);

#endif

