/*
** Date/time utility functions.
*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "maptime.h"

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
