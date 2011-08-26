#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdarg.h>

/** 
 * Routines that may not be present or uniform across all platforms
 */

/**
 * Duplicates the given c string.  It is the caller's responsibility to manage the memory
 * allocated by this call
 *
 * NOTE: Included because strdup has been deprecated on Windows platforms and may therefore be removed
 *       at some point
 */
char* xp_strdup(const char* src);

/**
 * Formats the given format string and arguments into string s
 *
 * NOTE: Included because vsnprintf behavior on Windows does not allow use as a preallocation count step
 */
int xp_vsnprintf(char* s, size_t length, const char* format, va_list ap);

/**
 * Formats the given format string and arguments and allocates a new string (ret)
 *
 * NOTE: Included because vasprintf is not included on Windows
 */
int xp_vasprintf(char** ret, const char* format, va_list ap);

/**
 * Formats the given format string and arguments and allocates a new string (ret)
 *
 * NOTE: Included because asprintf is not included on Windows
 */
int xp_asprintf(char** ret, const char* format, ...);

#endif