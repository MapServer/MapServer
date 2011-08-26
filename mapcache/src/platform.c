/** 
 * Cross platform versions of functions that are not available or inconsistent on some platforms
 */
 
 #include <stdlib.h>
 #include <string.h>
 #include "platform.h"
 
 /**
  * Duplicates the given c string.  It is the caller's responsibility to manage the memory
  * allocated by this call
  *
  * NOTE: Included because strdup has been deprecated on Windows platforms and may therefore be removed
  *       at some point
  */
 char* xp_strdup(const char* src)   {
     return strdup(src);
 }
 
 /**
  * Formats the given format string and arguments into string s
  *
  * NOTE: Included because vsnprintf behavior on Windows does not allow use as a preallocation count step
  */
 int xp_vsnprintf(char* s, size_t length, const char* format, va_list ap)  {
     /* TODO: Use _vscprintf on Windows */
     return vsnprintf(s, length, format, ap);
 }
 
 /**
  * Formats the given format string and arguments and allocates a new string (ret)
  *
  * NOTE: Included because vasprintf is not included on Windows
  */
 int xp_vasprintf(char** ret, const char* format, va_list ap)   {
     char* str;
     int length;
     
      // First determine the length by doing a "fake printf"
      length = xp_vsnprintf(0, 0, format, ap);
      
      if (length <= 0)    {
          // Could not determine the space needed for the string
          return -1;
      }

      str = (char*)malloc(length + 1);
      if (!str)   {
          // Could not allocate enough memory for the string
          return -1;
      }

      length = xp_vsnprintf(str, length + 1, format, ap);
      
      *ret = str;

      return length;
 }
 
 /**
  * Formats the given format string and arguments and allocates a new string (ret)
  *
  * NOTE: Included because asprintf is not included on Windows
  */
 int xp_asprintf(char** ret, const char* format, ...)   {
     int retval;
     va_list arglist;
     
     va_start(arglist, format);
     retval = xp_vasprintf(ret, format, arglist);
     va_end(arglist);
     
     return retval;
 }