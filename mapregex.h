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
 ****************************************************************************/

#ifndef MAP_REGEX_H
#define MAP_REGEX_H

#ifdef __cplusplus
extern "C" {
#endif

/* We want these to match the POSIX standard, so we need these*/
/* === regex2.h === */
#ifdef _WIN32
#define MS_API_EXPORT(type) __declspec(dllexport) type __stdcall
#elif defined(__GNUC__) && __GNUC__ >= 4
#define MS_API_EXPORT(type) __attribute__((visibility("default"))) type
#else
#define MS_API_EXPORT(type) type
#endif

typedef struct {
  void *sys_regex;
} ms_regex_t;

typedef int ms_regoff_t;
typedef struct {
  ms_regoff_t rm_so; /* Byte offset from string's start to substring's start. */
  ms_regoff_t rm_eo; /* Byte offset from string's start to substring's end.  */
} ms_regmatch_t;

MS_API_EXPORT(int) ms_regcomp(ms_regex_t *, const char *, int);
MS_API_EXPORT(size_t) ms_regerror(int, const ms_regex_t *, char *, size_t);
MS_API_EXPORT(int)
ms_regexec(const ms_regex_t *, const char *, size_t, ms_regmatch_t[], int);
MS_API_EXPORT(void) ms_regfree(ms_regex_t *);

#ifndef BUILDING_REGEX_PROXY

/* === regcomp.c === */
#define MS_REG_BASIC 0000
#define MS_REG_EXTENDED 0001
#define MS_REG_ICASE 0002
// WARNING: GNU regex has REG_NOSUB = (1 << 3) = 8
#define MS_REG_NOSUB 0004
// WARNING: GNU regex has REG_NEWLINE = (1 << 2) = 4
#define MS_REG_NEWLINE 0010

/* === regerror.c === */
#define MS_REG_OKAY 0
#define MS_REG_NOMATCH 1
#define MS_REG_BADPAT 2
#define MS_REG_ECOLLATE 3
#define MS_REG_ECTYPE 4
#define MS_REG_EESCAPE 5
#define MS_REG_ESUBREG 6
#define MS_REG_EBRACK 7
#define MS_REG_EPAREN 8
#define MS_REG_EBRACE 9
#define MS_REG_BADBR 10
#define MS_REG_ERANGE 11
#define MS_REG_ESPACE 12
#define MS_REG_BADRPT 13
#define MS_REG_EMPTY 14
#define MS_REG_ASSERT 15
#define MS_REG_INVARG 16
#define MS_REG_ATOI 255  /* convert name to number (!) */
#define MS_REG_ITOA 0400 /* convert number to name (!) */

/* === regexec.c === */
#define MS_REG_NOTBOL 00001
#define MS_REG_NOTEOL 00002
#define MS_REG_STARTEND 00004
#define MS_REG_TRACE 00400 /* tracing of execution */
#define MS_REG_LARGE 01000 /* force large representation */
#define MS_REG_BACKR 02000 /* force use of backref code */

#endif

#ifdef __cplusplus
}
#endif

#endif
