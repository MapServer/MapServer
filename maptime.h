#ifndef MAPTIME_H
#define MAPTIME_H


struct mstimeval{
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};
// function prototypes
void msTimeInit(struct tm *time);
int msDateCompare(struct tm *time1, struct tm *time2);
int msTimeCompare(struct tm *time1, struct tm *time2);
char *msStrptime(const char *s, const char *format, struct tm *tm);

#if defined(_WIN32) && !defined(__CYGWIN__)
void gettimeofday(struct mstimeval *t, void *__not_used_here__);
#endif


#endif /* MAPTIME_H */
