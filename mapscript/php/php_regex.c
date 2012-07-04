/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Regex wrapper
 * Author:   Bill Binko
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.2  2005/05/27 15:00:12  dan
 * New regex wrappers to solve issues with previous version (bug 1354)
 *
 */

/* we can't include mapserver.h, so we need our own basics */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <direct.h>
#include <memory.h>
#include <malloc.h>
#else
#include <unistd.h>
#endif

/*Need to specify this so that mapregex.h doesn't defined constants and
  doesn't #define away our ms_*/
#define BUILDING_REGEX_PROXY 1

#include "../../mapregex.h"

/* regex_extra.h doesn't exist in PHP >=5.3 */
#include "php.h"

#if ZEND_MODULE_API_NO < 20090626
#include "regex/regex_extra.h"
#include "regex/regex.h"
#else
#include "php_regex.h"
#endif



MS_API_EXPORT(int) ms_regcomp(ms_regex_t *regex, const char *expr, int cflags)
{
  /* Must free in regfree() */
  regex_t* sys_regex = (regex_t*) malloc(sizeof(regex_t));
  regex->sys_regex = (void*) sys_regex;
  return regcomp(sys_regex, expr, cflags);
}

MS_API_EXPORT(size_t) ms_regerror(int errcode, const ms_regex_t *regex, char *errbuf, size_t errbuf_size)
{
  return regerror(errcode, (regex_t*)(regex->sys_regex), errbuf, errbuf_size);
}

MS_API_EXPORT(int) ms_regexec(const ms_regex_t *regex, const char *string, size_t nmatch, ms_regmatch_t pmatch[], int eflags)
{
  /*This next line only works because we know that regmatch_t
    and ms_regmatch_t are exactly alike (POSIX STANDARD)*/
  return regexec((const regex_t*)(regex->sys_regex),
                 string, nmatch,
                 (regmatch_t*) pmatch, eflags);
}

MS_API_EXPORT(void) ms_regfree(ms_regex_t *regex)
{
  regfree((regex_t*)(regex->sys_regex));
  free(regex->sys_regex);
  return;
}
