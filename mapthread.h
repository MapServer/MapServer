#ifndef MAPTHREAD_H
#define MAPTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_THREAD
void msThreadInit();
int msGetThreadId();
void msAcquireLock(int);
void msReleaseLock(int);
int  msAllocateLock();
#else
#define msThreadInit()
#define msGetThreadId() (0)
#define msAcquireLock(x)
#define msReleaseLock(x)
#define msAllocateLock() (0)
#endif

#define TLOCK_PARSER	1
#define TLOCK_GDAL	2
#define TLOCK_ERROROBJ  3
#define TLOCK_PROJ      4

#define TLOCK_STATIC_MAX 20
#define TLOCK_MAX       100

#ifdef __cplusplus
}
#endif

#endif /* MAPTHREAD_H */








