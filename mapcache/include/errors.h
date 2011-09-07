/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
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
 *****************************************************************************/
#ifndef MAPCACHE_ERRORS_H_
#define MAPCACHE_ERRORS_H_

typedef enum {
MAPCACHE_DEBUG,
MAPCACHE_INFO,
MAPCACHE_WARNING,
MAPCACHE_ERROR
} mapcache_log_level;

typedef enum {
   MAPCACHE_REPORT_LOG,
   MAPCACHE_REPORT_MSG,
   MAPCACHE_REPORT_ERROR_IMG,
   MAPCACHE_REPORT_EMPTY_IMG,
   MAPCACHE_REPORT_CUSTOM_IMG
} mapcache_error_reporting;

#endif /* ERRORS_H_ */
/* vim: ai ts=3 sts=3 et sw=3
*/
