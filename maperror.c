#include "map.h"
#include "maperror.h"
#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <stdarg.h>

static char *ms_errorCodes[MS_NUMERRORCODES]={"",
					    "Unable to access file.",
					    "Memory allocation error.",
					    "Incorrect data type.",
					    "Symbol definition error.",
					    "Regular expression error.",
					    "TrueType Font error.",
					    "DBASE file error.",
					    "GD library error.",
					    "Unknown identifier.",
					    "Premature End-of-File.",
					    "Projection library error.",
					    "General error message.",
					    "CGI error.",
					    "Web application error.",
					    "Image handling error.",
					    "Hash table error.",
					    "Join error.",
					    "Search returned no results.",
					    "Shapefile error.",
					    "Expression parser error.",
                                            "SDE error.",
					    "OGR error."
};

errorObj ms_error = {-1, "", ""};

char *msGetErrorString(int code) {
  
  if(code<0 || code>MS_NUMERRORCODES-1)
    return("Invalid error code.");

  return(ms_errorCodes[code]);
}

void msSetError(int code, char *message, char *routine)
{
  char *errfile=NULL;
  FILE *errstream;
  time_t errtime;

  ms_error.code = code;

  if(!routine)
    strcpy(ms_error.routine, "");
  else
    strncpy(ms_error.routine, routine, ROUTINELENGTH);

  if(!message)
    strcpy(ms_error.message, "");
  else
    strncpy(ms_error.message, message, MESSAGELENGTH);

  errfile = getenv("MS_ERRORFILE");
  if(errfile) {
    if(strcmp(errfile, "stderr") == 0)
      errstream = stderr;
    else if(strcmp(errfile, "stdout") == 0)
      errstream = stdout;
    else
      errstream = fopen(errfile, "a");
    if(!errstream) return;
    errtime = time(NULL);
    fprintf(errstream, "%s - %s: %s %s\n", chop(ctime(&errtime)), ms_error.routine, ms_errorCodes[ms_error.code], ms_error.message);
    fclose(errstream);
  }
}

void msWriteError(FILE *stream)
{
  fprintf(stream, "%s: %s %s\n", ms_error.routine, ms_errorCodes[ms_error.code], ms_error.message);
}

char *msGetVersion() {
  static char version[256];

  sprintf(version, "MapServer version %s", MS_VERSION);

#ifdef USE_GD_GIF
  strcat(version, " OUTPUT=GIF");
#endif
#ifdef USE_GD_PNG
  strcat(version, " OUTPUT=PNG");
#endif
#ifdef USE_GD_JPEG
  strcat(version, " OUTPUT=JPEG");
#endif
#ifdef USE_GD_WBMP
  strcat(version, " OUTPUT=WBMP");
#endif
#ifdef USE_PROJ
  strcat(version, " SUPPORTS=PROJ");
#endif
#ifdef USE_GD_TTF
  strcat(version, " SUPPORTS=TTF");
#endif
#ifdef USE_TIFF
  strcat(version, " INPUT=TIFF");
#endif
#ifdef USE_EPPL
  strcat(version, " INPUT=EPPL7");
#endif
#ifdef USE_JPEG
  strcat(version, " INPUT=JPEG");
#endif
#ifdef USE_SDE
  strcat(version, " INPUT=SDE");
#endif
#ifdef USE_OGR
  strcat(version, " INPUT=OGR");
#endif
#ifdef USE_GDAL
  strcat(version, " INPUT=GDAL");
#endif

  return(version);
}

void msDebug( const char * pszFormat, ... )
{
#ifndef _WIN32
    va_list args;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    fprintf(stderr, "[%s].%ld ", chop(ctime(&(tv.tv_sec))), tv.tv_usec);

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
#endif
}

