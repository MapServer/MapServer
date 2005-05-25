/* we can't include map.h, so we need our own basics */

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

#include "mapregex.h"

#include <regex.h>

API_EXPORT(int) ms_regcomp(ms_regex_t *regex, const char *expr, int cflags)
{
  /* Must free in regfree() */
  regex_t* sys_regex = (regex_t*) malloc(sizeof(regex_t));
  regex->sys_regex = (void*) sys_regex;
  return regcomp(sys_regex, expr, cflags);
}

API_EXPORT(size_t) ms_regerror(int errcode, const ms_regex_t *regex, char *errbuf, size_t errbuf_size)
{
  return regerror(errcode, (regex_t*)(regex->sys_regex), errbuf, errbuf_size);
}

API_EXPORT(int) ms_regexec(const ms_regex_t *regex, const char *string, size_t nmatch, ms_regmatch_t pmatch[], int eflags)
{
  /*This next line only works because we know that regmatch_t
    and ms_regmatch_t are exactly alike (POSIX STANDARD)*/
  return regexec((const regex_t*)(regex->sys_regex), 
	       string, nmatch, 
	       (regmatch_t*) pmatch, eflags);
}

API_EXPORT(void) ms_regfree(ms_regex_t *regex)
{
  regfree((regex_t*)(regex->sys_regex));
  free(regex->sys_regex);
  return;
}
