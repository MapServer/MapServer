/******************************************************************************
 * Project:  MapServer
 * Purpose:  MapServer private include file.
 * Author:   Even Rouault
 *
 ******************************************************************************
 * Copyright (c) 2025 Even Rouault
 *
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
 *****************************************************************************/

#ifndef MAPRENDERING_H
#define MAPRENDERING_H

#include "mapserver.h"

#ifdef __cplusplus
extern "C" {
#endif

void computeSymbolStyle(symbolStyleObj *s, styleObj *src, symbolObj *symbol,
                        double scalefactor, double resolutionfactor);
int msAdjustMarkerPos(mapObj *map, styleObj *style, symbolObj *symbol,
                      double *p_x, double *p_y, double scalefactor,
                      double rotation);

#ifdef __cplusplus
}
#endif

#endif
