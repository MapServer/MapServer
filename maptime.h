/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Time processing related declarations.
 * Author:   Steve Lime and the MapServer team.
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
 * Revision 1.12  2006/03/14 03:43:30  assefa
 * Move msValidateTimeValue to maptime so it can be used by WMS and SOS.
 * (Bug 1710)
 *
 * Revision 1.11  2005/06/14 16:03:35  dan
 * Updated copyright date to 2005
 *
 * Revision 1.10  2005/02/18 03:06:47  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.9  2005/01/28 06:16:54  sdlime
 * Applied patch to make function prototypes ANSI C compliant. Thanks to Petter Reinholdtsen. This fixes but 1181.
 *
 * Revision 1.8  2004/10/21 10:54:17  assefa
 * Add postgis date_trunc support.
 *
 * Revision 1.7  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#ifndef MAPTIME_H
#define MAPTIME_H

/* (bug 602) gettimeofday() and struct timeval aren't available on Windows,
 * so we provide a replacement with msGettimeofday() and struct mstimeval on
 * Windows.  On Unix/Linux they are just #defines to the real things.
 */
#if defined(_WIN32) && !defined(__CYGWIN__)
struct mstimeval {
    long    tv_sec;         /* seconds */
    long    tv_usec;        /* and microseconds */
};
void msGettimeofday(struct mstimeval *t, void *__not_used_here__);
#else
#  include <sys/time.h>     /* for gettimeofday() */
#  define  mstimeval timeval
#  define  msGettimeofday(t,u) gettimeofday(t,u)
#endif

typedef enum 
{
    TIME_RESOLUTION_UNDEFINED = -1,
    TIME_RESOLUTION_MICROSECOND =0,
    TIME_RESOLUTION_MILLISECOND =1,
    TIME_RESOLUTION_SECOND =2,
    TIME_RESOLUTION_MINUTE =3,
    TIME_RESOLUTION_HOUR =4,
    TIME_RESOLUTION_DAY =5,
    TIME_RESOLUTION_MONTH =6,
    TIME_RESOLUTION_YEAR =7
}MS_TIME_RESOLUTION;

/* function prototypes */
void msTimeInit(struct tm *time);
int msDateCompare(struct tm *time1, struct tm *time2);
int msTimeCompare(struct tm *time1, struct tm *time2);
char *msStrptime(const char *s, const char *format, struct tm *tm);
int msParseTime(const char *string, struct tm *tm);
int msTimeMatchPattern(char *timestring, char *pattern);
void msSetLimitedPattersToUse(char *patternstring);
void msUnsetLimitedPatternToUse(void);
int msTimeGetResolution(const char *timestring);

int msValidateTimeValue(char *timestring, const char *timeextent);

#endif /* MAPTIME_H */
