#ifndef MAPTIME_H
#define MAPTIME_H

// function prototypes
void msTimeInit(struct tm *time);
int msDateCompare(struct tm *time1, struct tm *time2);
int msTimeCompare(struct tm *time1, struct tm *time2);

#endif /* MAPTIME_H */
