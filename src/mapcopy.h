/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Declarations related to copyng mapObjs.  Provided by Mladen Turk
 *           for resolution of bug 701
 *           http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=701.
 * Author:   Mladen Turk (and Sean Gilles?)
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

/* Following works for GCC and MSVC.  That's ~98% of our users, if Tyler
 * Mitchell's survey is correct
 */
#define MS_COPYSTELEM(_name) (dst)->/**/_name = (src)->/**/_name

#define MS_MACROBEGIN              do {
#define MS_MACROEND                } while (0)

#define MS_COPYRECT(_dst, _src)       \
    MS_MACROBEGIN                     \
        (_dst)->minx = (_src)->minx;  \
        (_dst)->miny = (_src)->miny;  \
        (_dst)->maxx = (_src)->maxx;  \
        (_dst)->maxy = (_src)->maxy;  \
    MS_MACROEND

#define MS_COPYPOINT(_dst, _src)      \
    MS_MACROBEGIN                     \
        (_dst)->x = (_src)->x;        \
        (_dst)->y = (_src)->y;        \
        (_dst)->m = (_src)->m;        \
    MS_MACROEND

#define MS_COPYCOLOR(_dst, _src)      \
    MS_MACROBEGIN                     \
        (_dst)->red   = (_src)->red;  \
        (_dst)->green = (_src)->green;\
        (_dst)->blue  = (_src)->blue; \
        (_dst)->alpha  = (_src)->alpha; \
    MS_MACROEND

#define MS_COPYSTRING(_dst, _src)     \
    MS_MACROBEGIN                     \
        if ((_dst) != NULL)           \
            msFree((_dst));           \
        if ((_src))                   \
            (_dst) = msStrdup((_src));  \
        else                          \
            (_dst) = NULL;            \
    MS_MACROEND


