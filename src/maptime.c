/******************************************************************************
 * $id$
 *
 * Project:  MapServer
 * Purpose:  Date/Time utility functions.
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
 ****************************************************************************/

#define _GNU_SOURCE /* glibc2 needs this for strptime() */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "mapserver.h"
#include "maptime.h"
#include "maperror.h"
#include "mapthread.h"

typedef struct {
  char pattern[64];
  ms_regex_t *regex;
  char format[32];
  char userformat[32];
  MS_TIME_RESOLUTION resolution;
} timeFormatObj;

static timeFormatObj ms_timeFormats[] = {
    {"^[0-9]{8}", NULL, "%Y%m%d", "YYYYMMDD", TIME_RESOLUTION_DAY},
    {"^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z", NULL,
     "%Y-%m-%dT%H:%M:%SZ", "YYYY-MM-DDTHH:MM:SSZ", TIME_RESOLUTION_SECOND},
    {"^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}", NULL,
     "%Y-%m-%dT%H:%M:%S", "YYYY-MM-DDTHH:MM:SS", TIME_RESOLUTION_SECOND},
    {"^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}", NULL,
     "%Y-%m-%d %H:%M:%S", "YYYY-MM-DD HH:MM:SS", TIME_RESOLUTION_SECOND},
    {"^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}", NULL, "%Y-%m-%dT%H:%M",
     "YYYY-MM-DDTHH:MM", TIME_RESOLUTION_MINUTE},
    {"^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}", NULL, "%Y-%m-%d %H:%M",
     "YYYY-MM-DD HH:MM", TIME_RESOLUTION_MINUTE},
    {"^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}", NULL, "%Y-%m-%dT%H",
     "YYYY-MM-DDTHH", TIME_RESOLUTION_HOUR},
    {"^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}", NULL, "%Y-%m-%d %H",
     "YYYY-MM-DD HH", TIME_RESOLUTION_HOUR},
    {"^[0-9]{4}-[0-9]{2}-[0-9]{2}", NULL, "%Y-%m-%d", "YYYY-MM-DD",
     TIME_RESOLUTION_DAY},
    {"^[0-9]{4}-[0-9]{2}", NULL, "%Y-%m", "YYYY-MM", TIME_RESOLUTION_MONTH},
    {"^[0-9]{4}", NULL, "%Y", "YYYY", TIME_RESOLUTION_YEAR},
    {"^T[0-9]{2}:[0-9]{2}:[0-9]{2}Z", NULL, "T%H:%M:%SZ", "THH:MM:SSZ",
     TIME_RESOLUTION_SECOND},
    {"^T[0-9]{2}:[0-9]{2}:[0-9]{2}", NULL, "T%H:%M:%S", "THH:MM:SS",
     TIME_RESOLUTION_SECOND},
    {"^[0-9]{2}:[0-9]{2}:[0-9]{2}Z", NULL, "%H:%M:%SZ", "HH:MM:SSZ",
     TIME_RESOLUTION_SECOND},
    {"^[0-9]{2}:[0-9]{2}:[0-9]{2}", NULL, "%H:%M:%S", "HH:MM:SS",
     TIME_RESOLUTION_SECOND}};

#define MS_NUMTIMEFORMATS                                                      \
  (int)(sizeof(ms_timeFormats) / sizeof(ms_timeFormats[0]))

int *ms_limited_pattern = NULL;
int ms_num_limited_pattern;

int ms_time_inited = 0;
int msTimeSetup() {
  if (!ms_time_inited) {
    msAcquireLock(TLOCK_TIME);
    if (!ms_time_inited) {
      int i;
      for (i = 0; i < MS_NUMTIMEFORMATS; i++) {
        ms_timeFormats[i].regex = msSmallMalloc(sizeof(ms_regex_t));
        if (0 != ms_regcomp(ms_timeFormats[i].regex, ms_timeFormats[i].pattern,
                            MS_REG_EXTENDED | MS_REG_NOSUB)) {
          msSetError(MS_REGEXERR, "Failed to compile expression (%s).",
                     "msTimeSetup()", ms_timeFormats[i].pattern);
          return MS_FAILURE;
          /* TODO: free already init'd regexes */
        }
      }
      ms_limited_pattern =
          (int *)msSmallMalloc(sizeof(int) * MS_NUMTIMEFORMATS);
      ms_num_limited_pattern = 0;
      ms_time_inited = 1;
    }
    msReleaseLock(TLOCK_TIME);
  }
  return MS_SUCCESS;
}

void msTimeCleanup() {
  if (ms_time_inited) {
    int i;
    for (i = 0; i < MS_NUMTIMEFORMATS; i++) {
      if (ms_timeFormats[i].regex) {
        ms_regfree(ms_timeFormats[i].regex);
        msFree(ms_timeFormats[i].regex);
        ms_timeFormats[i].regex = NULL;
      }
    }
    msFree(ms_limited_pattern);
    ms_time_inited = 0;
  }
}

void msTimeInit(struct tm *time) {
  time->tm_sec = 0; /* set all members to zero */
  time->tm_min = 0;
  time->tm_hour = 0;
  time->tm_mday = 0;
  time->tm_mon = 0;
  time->tm_year = 0;
  time->tm_wday = 0;
  time->tm_yday = 0;
  time->tm_isdst = 0;

  return;
}

static int compareIntVals(int a, int b) {
  if (a < b)
    return -1;
  else if (a > b)
    return 1;
  else
    return 0;
}

int msDateCompare(struct tm *time1, struct tm *time2) {
  int result;

  if ((result = compareIntVals(time1->tm_year, time2->tm_year)) != 0)
    return result; /* not equal based on year */
  else if ((result = compareIntVals(time1->tm_mon, time2->tm_mon)) != 0)
    return result; /* not equal based on month */
  else if ((result = compareIntVals(time1->tm_mday, time2->tm_mday)) != 0)
    return result; /* not equal based on day of month */

  return (0); /* must be equal */
}

int msTimeCompare(struct tm *time1, struct tm *time2) {
  int result;

  // fprintf(stderr, "in msTimeCompare()...\n");
  // fprintf(stderr, "time1: %d %d %d %d %d %d\n", time1->tm_year,
  // time1->tm_mon, time1->tm_mday, time1->tm_hour, time1->tm_min,
  // time1->tm_sec); fprintf(stderr, "time2: %d %d %d %d %d %d\n",
  // time2->tm_year, time2->tm_mon, time2->tm_mday, time2->tm_hour,
  // time2->tm_min, time2->tm_sec);

  if ((result = compareIntVals(time1->tm_year, time2->tm_year)) != 0)
    return result; /* not equal based on year */
  else if ((result = compareIntVals(time1->tm_mon, time2->tm_mon)) != 0)
    return result; /* not equal based on month */
  else if ((result = compareIntVals(time1->tm_mday, time2->tm_mday)) != 0)
    return result; /* not equal based on day of month */
  else if ((result = compareIntVals(time1->tm_hour, time2->tm_hour)) != 0)
    return result; /* not equal based on hour */
  else if ((result = compareIntVals(time1->tm_min, time2->tm_min)) != 0)
    return result; /* not equal based on minute */
  else if ((result = compareIntVals(time1->tm_sec, time2->tm_sec)) != 0)
    return result; /* not equal based on second */

  return (0); /* must be equal */
}

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <sys/timeb.h>
void msGettimeofday(struct mstimeval *tp, void *tzp) {
  struct _timeb theTime;

  _ftime(&theTime);
  tp->tv_sec = theTime.time;
  tp->tv_usec = theTime.millitm * 1000;
}
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
/* we need to provide our own prototype on windows. */
char *strptime(const char *buf, const char *format, struct tm *timeptr);
#endif

char *msStrptime(const char *s, const char *format, struct tm *tm) {
  memset(tm, 0, sizeof(struct tm));
  return strptime(s, format, tm);
}

/**
   return MS_TRUE if the time string matches the timeformat.
   else return MS_FALSE.
 */
int msTimeMatchPattern(const char *timestring, const char *timeformat) {
  int i = -1;
  if (msTimeSetup() != MS_SUCCESS) {
    return MS_FALSE;
  }

  /* match the pattern format first and then check if the time string  */
  /* matches the pattern. If it is the case retrurn the MS_TRUE */
  for (i = 0; i < MS_NUMTIMEFORMATS; i++) {
    if (strcasecmp(ms_timeFormats[i].userformat, timeformat) == 0)
      break;
  }

  if (i >= 0 && i < MS_NUMTIMEFORMATS) {
    int match = ms_regexec(ms_timeFormats[i].regex, timestring, 0, NULL, 0);
    if (match == 0)
      return MS_TRUE;
  }
  return MS_FALSE;
}

void msUnsetLimitedPatternToUse() {
  msTimeSetup();
  ms_num_limited_pattern = 0;
}

void msSetLimitedPatternsToUse(const char *patternstring) {
  int *limitedpatternindice = NULL;
  int numpatterns = 0, ntmp = 0;
  char **patterns = NULL;
  msTimeSetup();

  limitedpatternindice = (int *)msSmallMalloc(sizeof(int) * MS_NUMTIMEFORMATS);

  /* free previous setting */
  msUnsetLimitedPatternToUse();

  if (patternstring) {
    patterns = msStringSplit(patternstring, ',', &ntmp);
    if (patterns && ntmp >= 1) {

      for (int i = 0; i < ntmp; i++) {
        for (int j = 0; j < MS_NUMTIMEFORMATS; j++) {
          if (strcasecmp(ms_timeFormats[j].userformat, patterns[i]) == 0) {
            limitedpatternindice[numpatterns] = j;
            numpatterns++;
            break;
          }
        }
      }
    }
    msFreeCharArray(patterns, ntmp);
  }

  if (numpatterns > 0) {
    for (int i = 0; i < numpatterns; i++)
      ms_limited_pattern[i] = limitedpatternindice[i];

    ms_num_limited_pattern = numpatterns;
  }
  free(limitedpatternindice);
}

int msParseTime(const char *string, struct tm *tm) {
  int num_patterns = 0;

  if (MS_STRING_IS_NULL_OR_EMPTY(string))
    return MS_FALSE; /* nothing to parse so bail */

  if (msTimeSetup() != MS_SUCCESS) {
    return MS_FALSE;
  }

  /* if limited patterns are set, use them, else use all the patterns defined */
  if (ms_num_limited_pattern > 0)
    num_patterns = ms_num_limited_pattern;
  else
    num_patterns = MS_NUMTIMEFORMATS;

  for (int i = 0; i < num_patterns; i++) {
    int match;
    int indice;
    if (ms_num_limited_pattern > 0)
      indice = ms_limited_pattern[i];
    else
      indice = i;

    match = ms_regexec(ms_timeFormats[indice].regex, string, 0, NULL, 0);
    /* test the expression against the string */
    if (match == 0) {
      /* match    */
      msStrptime(string, ms_timeFormats[indice].format, tm);
      return (MS_TRUE);
    }
  }

  msSetError(MS_REGEXERR, "Unrecognized date or time format (%s).",
             "msParseTime()", string);
  return (MS_FALSE);
}

/**
 * Parse the time string and return the reslution
 */
int msTimeGetResolution(const char *timestring) {
  int i = 0;

  if (!timestring)
    return -1;

  for (i = 0; i < MS_NUMTIMEFORMATS; i++) {
    ms_regex_t *regex = (ms_regex_t *)msSmallMalloc(sizeof(ms_regex_t));
    if (ms_regcomp(regex, ms_timeFormats[i].pattern,
                   MS_REG_EXTENDED | MS_REG_NOSUB) != 0) {
      msSetError(MS_REGEXERR, "Failed to compile expression (%s).",
                 "msParseTime()", ms_timeFormats[i].pattern);
      msFree(regex);
      return -1;
    }
    /* test the expression against the string */
    if (ms_regexec(regex, timestring, 0, NULL, 0) == 0) {
      /* match    */
      ms_regfree(regex);
      msFree(regex);
      return ms_timeFormats[i].resolution;
    }
    ms_regfree(regex);
    msFree(regex);
  }

  return -1;
}

int _msValidateTime(const char *timestring, const char *timeextent) {
  int numelements, numextents, i, numranges;
  struct tm tmtimestart, tmtimeend, tmstart, tmend;
  char **atimerange = NULL, **atimeelements = NULL, **atimeextents = NULL;

  if (!timestring || !timeextent)
    return MS_FALSE;

  if (strlen(timestring) == 0 || strlen(timeextent) == 0)
    return MS_FALSE;

  /* we first need to parse the timesting that is passed
     so that we can determine if it is a descrete time
     or a range */

  numelements = 0;
  atimeelements = msStringSplit(timestring, '/', &numelements);
  msTimeInit(&tmtimestart);
  msTimeInit(&tmtimeend);

  if (numelements == 1) { /*descrete time*/
    /*start end end times are the same*/
    if (msParseTime(timestring, &tmtimestart) != MS_TRUE) {
      msFreeCharArray(atimeelements, numelements);
      return MS_FALSE;
    }
    if (msParseTime(timestring, &tmtimeend) != MS_TRUE) {
      msFreeCharArray(atimeelements, numelements);
      return MS_FALSE;
    }
  } else if (numelements >= 2) { /*range */
    if (msParseTime(atimeelements[0], &tmtimestart) != MS_TRUE) {
      msFreeCharArray(atimeelements, numelements);
      return MS_FALSE;
    }
    if (msParseTime(atimeelements[1], &tmtimeend) != MS_TRUE) {
      msFreeCharArray(atimeelements, numelements);
      return MS_FALSE;
    }
  }

  msFreeCharArray(atimeelements, numelements);

  /* Now parse the time extent. Extents can be
    -  one range (2004-09-21/2004-09-25/resolution)
    -  multiple rages 2004-09-21/2004-09-25/res1,2004-09-21/2004-09-25/res2
    - one value 2004-09-21
    - multiple values 2004-09-21,2004-09-22,2004-09-23
  */

  numextents = 0;
  atimeextents = msStringSplit(timeextent, ',', &numextents);
  if (numextents <= 0) {
    msFreeCharArray(atimeextents, numextents);
    return MS_FALSE;
  }

  /*the time timestring should at be valid in one of the extents
    defined */

  for (i = 0; i < numextents; i++) {
    /* build time structure for the extents */
    msTimeInit(&tmstart);
    msTimeInit(&tmend);

    numranges = 0;
    atimerange = msStringSplit(atimeextents[i], '/', &numranges);
    /* - one value 2004-09-21 */
    if (numranges == 1) {
      /*time tested can either be descrete or a range */

      if (msParseTime(atimerange[0], &tmstart) == MS_TRUE &&
          msParseTime(atimerange[0], &tmend) == MS_TRUE &&
          msTimeCompare(&tmstart, &tmtimestart) <= 0 &&
          msTimeCompare(&tmend, &tmtimeend) >= 0) {
        msFreeCharArray(atimerange, numranges);
        msFreeCharArray(atimeextents, numextents);
        return MS_TRUE;
      }
    }
    /*2004-09-21/2004-09-25/res1*/
    else if (numranges >= 2) {
      if (msParseTime(atimerange[0], &tmstart) == MS_TRUE &&
          msParseTime(atimerange[1], &tmend) == MS_TRUE &&
          msTimeCompare(&tmstart, &tmtimestart) <= 0 &&
          msTimeCompare(&tmend, &tmtimeend) >= 0) {
        msFreeCharArray(atimerange, numranges);
        msFreeCharArray(atimeextents, numextents);
        return MS_TRUE;
      }
    }
    msFreeCharArray(atimerange, numranges);
  }
  msFreeCharArray(atimeextents, numextents);
  return MS_FALSE;
}

int msValidateTimeValue(const char *timestring, const char *timeextent) {
  char **atimes = NULL;
  int i, numtimes = 0;

  /* we need to validate the time passsed in the request */
  /* against the time extent defined */

  if (!timestring || !timeextent)
    return MS_FALSE;

  /* To avoid SQL injections */
  if (strchr(timestring, '\''))
    return MS_FALSE;

  /* parse the time string. We support descrete times (eg 2004-09-21), */
  /* multiple times (2004-09-21, 2004-09-22, ...) */
  /* and range(s) (2004-09-21/2004-09-25, 2004-09-27/2004-09-29) */
  if (strstr(timestring, ",") == NULL &&
      strstr(timestring, "/") == NULL) { /* discrete time */
    return _msValidateTime(timestring, timeextent);
  } else {
    atimes = msStringSplit(timestring, ',', &numtimes);
    if (numtimes >= 1) { /* multiple times */
      for (i = 0; i < numtimes; i++) {
        if (_msValidateTime(atimes[i], timeextent) == MS_FALSE) {
          msFreeCharArray(atimes, numtimes);
          return MS_FALSE;
        }
      }
      msFreeCharArray(atimes, numtimes);
      return MS_TRUE;
    } else {
      msFreeCharArray(atimes, numtimes);
    }
  }
  return MS_FALSE;
}
