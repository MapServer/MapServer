/*
** Date/Time utility functions.
*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "map.h"
#include "maptime.h"
#include "maperror.h"

typedef struct {
  char pattern[64];
  regex_t *regex;
  char  format[32];  
} timeFormatObj;

#define MS_NUMTIMEFORMATS 13

timeFormatObj ms_timeFormats[MS_NUMTIMEFORMATS] = {
  {"[0-9]{8}", NULL, "%Y%m%d"},
  {"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z", NULL, "%Y-%m-%dT%H:%M:%SZ"},
  {"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}", NULL, "%Y-%m-%dT%H:%M:%S"},
  {"[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}", NULL, "%Y-%m-%d %H:%M:%S"},
  {"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}", NULL, "%Y-%m-%dT%H:%M"},
  {"[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}", NULL, "%Y-%m-%d %H:%M"},
  {"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}", NULL, "%Y-%m-%dT%H"},
  {"[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}", NULL, "%Y-%m-%d %H"},
  {"[0-9]{4}-[0-9]{2}-[0-9]{2}", NULL, "%Y-%m-%d"},
  {"[0-9]{4}-[0-9]{2}", NULL, "%Y-%m"},
  {"[0-9]{4}", NULL, "%Y"},
  {"T[0-9]{2}:[0-9]{2}:[0-9]{2}Z", NULL, "T%H:%M:%SZ"},
  {"T[0-9]{2}:[0-9]{2}:[0-9]{2}", NULL, "T%H:%M:%S"},
};

int *ms_limited_pattern = NULL;
int ms_num_limited_pattern;

void msTimeInit(struct tm *time)
{
  // set all members to zero
  time->tm_sec = 0;
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
  if(a<b) return -1;
  else if(a>b) return 1;
  else return 0;
}

int msDateCompare(struct tm *time1, struct tm *time2)
{
  int result;

  if((result = compareIntVals(time1->tm_year, time2->tm_year)) != 0)
    return result; // not equal based on year
  else if((result = compareIntVals(time1->tm_mon, time2->tm_mon)) != 0)
    return result; // not equal based on month
  else if((result = compareIntVals(time1->tm_mday, time2->tm_mday)) != 0)
    return result; // not equal based on day of month
  
  return(0); // must be equal
}

int msTimeCompare(struct tm *time1, struct tm *time2)
{
  int result;

  if((result = compareIntVals(time1->tm_year, time2->tm_year)) != 0)
    return result; // not equal based on year
  else if((result = compareIntVals(time1->tm_mon, time2->tm_mon)) != 0)
    return result; // not equal based on month
  else if((result = compareIntVals(time1->tm_mday, time2->tm_mday)) != 0)
    return result; // not equal based on day of month
  else if((result = compareIntVals(time1->tm_hour, time2->tm_hour)) != 0)
    return result; // not equal based on hour
  else if((result = compareIntVals(time1->tm_min, time2->tm_min)) != 0)
    return result; // not equal based on minute
  else if((result = compareIntVals(time1->tm_sec, time2->tm_sec)) != 0)
    return result; // not equal based on second

  return(0); // must be equal
}

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <sys/timeb.h>
void msGettimeofday(struct mstimeval* tp, void* tzp)
{
    struct _timeb theTime;
 
    _ftime(&theTime);
    tp->tv_sec = theTime.time;
    tp->tv_usec = theTime.millitm * 1000;
}
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
// we need to provide our own prototype on windows.
char *strptime( const char *buf, const char *format, struct tm *timeptr );
#endif

char *msStrptime(const char *s, const char *format, struct tm *tm)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
    //we are now using a local strptime found strptime.c
    return strptime(s, format, tm);
#else
    /* Use system strptime() on non-windows systems */
    return strptime(s, format, tm);
#endif
}

/**
   return MS_TRUE if the time string matchs the timeformat.
   else return MS_FALSE.
 */
int msTimeMatchPattern(char *timestring, char *timeformat)
{
    int i =-1;

    //match the pattern format first and then check if the time string 
    //matchs the pattern. If it is the case retrurn the MS_TRUE
    for (i=0; i<MS_NUMTIMEFORMATS; i++)
    {
        if (strcasecmp(ms_timeFormats[i].format, timeformat) == 0)
          break;
    }
     
    if (i >= 0 && i < MS_NUMTIMEFORMATS)
    {
        if(!ms_timeFormats[i].regex)
        {
            ms_timeFormats[i].regex = (regex_t *) malloc(sizeof(regex_t));
            regcomp(ms_timeFormats[i].regex, 
                    ms_timeFormats[i].pattern, REG_EXTENDED|REG_NOSUB);
        }
        if (regexec(ms_timeFormats[i].regex, timestring, 0,NULL, 0) == 0)
          return MS_TRUE;
        
    }
    return MS_FALSE;
}


void msUnsetLimitedPatternToUse()
{
    if (ms_limited_pattern &&  ms_num_limited_pattern > 0)
      free(ms_limited_pattern);

    ms_num_limited_pattern = 0;
}

void msSetLimitedPattersToUse(char *patternstring)
{
    int *limitedpatternindice = NULL;
    int numpatterns, i, j, ntmp = 0;
    char **patterns = NULL;

    limitedpatternindice = (int *)malloc(sizeof(int)*MS_NUMTIMEFORMATS);
    
    //free previous setting
    msUnsetLimitedPatternToUse();

    if (patternstring)
    {
        patterns = split(patternstring, ',', &ntmp);
        if (patterns && ntmp >= 1)
        {
            
            for (i=0; i<ntmp; i++)
            {
                for (j=0; j<MS_NUMTIMEFORMATS; j++)
                {
                    if (strcasecmp( ms_timeFormats[j].format, patterns[i]) ==0)
                    {
                        limitedpatternindice[numpatterns] = j;
                        numpatterns++;
                        break;
                    }
                }
            }
            
            msFreeCharArray(patterns, ntmp);
        }
    }

    if (numpatterns > 0)
    {
        ms_limited_pattern = (int *)malloc(sizeof(int)*numpatterns);
        for (i=0; i<numpatterns; i++)
          ms_limited_pattern[i] = limitedpatternindice[i];

        ms_num_limited_pattern = numpatterns;
        free (limitedpatternindice);
    }
                                            
}



int msParseTime(const char *string, struct tm *tm) {
  int i, indice = 0;
  int num_patterns = 0;
  
  //if limited patterns are set, use then. Else use all the
  //patterns defined
  if (ms_limited_pattern &&  ms_num_limited_pattern > 0)
    num_patterns = ms_num_limited_pattern;
  else
    num_patterns = MS_NUMTIMEFORMATS;

  for(i=0; i<num_patterns; i++) {
      if (ms_num_limited_pattern > 0)
        indice = ms_limited_pattern[i];
      else
        indice = i;

      if(!ms_timeFormats[indice].regex) { // compile the expression
      ms_timeFormats[indice].regex = (regex_t *) malloc(sizeof(regex_t)); 
      if(regcomp(ms_timeFormats[indice].regex, ms_timeFormats[indice].pattern, REG_EXTENDED|REG_NOSUB) != 0) {
	msSetError(MS_REGEXERR, "Failed to compile expression (%s).", "msParseTime()", ms_timeFormats[indice].pattern);
	return(MS_FALSE);
      }
    }  

    // test the expression against the string
    if(regexec(ms_timeFormats[indice].regex, string, 0, NULL, 0) == 0) 
    { // match   
        msStrptime(string, ms_timeFormats[indice].format, tm);
        return(MS_TRUE);
    }
  }

  msSetError(MS_REGEXERR, "Unrecognized date or time format (%s).", "msParseTime()", string);
  return(MS_FALSE);
}
