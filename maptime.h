#ifndef MAPTIME_H
#define MAPTIME_H

/* (bug 602) gettimeofday() and struct timeval aren't available on Windows,
 * so we provide a replacement with msGettimeofday() and struct mstimeval on
 * Windows.  On Unix/Linux they are just #defines to the real things.
 */
#if defined(_WIN32) && !defined(__CYGWIN__)
struct mstimeval{
    long    tv_sec;         /* seconds */
    long    tv_usec;        /* and microseconds */
};
void msGettimeofday(struct mstimeval *t, void *__not_used_here__);
#else
#  include <sys/time.h>     /* for gettimeofday() */
#  define  mstimeval timeval
#  define  msGettimeofday(t,u) gettimeofday(t,u)
#endif


// function prototypes
void msTimeInit(struct tm *time);
int msDateCompare(struct tm *time1, struct tm *time2);
int msTimeCompare(struct tm *time1, struct tm *time2);
char *msStrptime(const char *s, const char *format, struct tm *tm);


#endif /* MAPTIME_H */
