#include "map.h"
#include "maperror.h"
#include "mapthread.h"
#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <stdarg.h>

static char *ms_errorCodes[MS_NUMERRORCODES] = {"",
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
						"OGR error.",
						"Query error.",
						"WMS server error.",
						"WMS connection error.",
						"OracleSpatial error.",
						"WFS server error.",
						"WFS connection error."
};

#ifndef USE_THREAD

errorObj *msGetErrorObj()
{
    static errorObj ms_error = {-1, "", ""};

    return &ms_error;
}
#endif

#ifdef USE_THREAD

typedef struct te_info
{
    struct te_info *next;
    int             thread_id;
    errorObj        ms_error;
} te_info_t;

errorObj *msGetErrorObj()
{
    static te_info_t *error_list = NULL;
    te_info_t *link;
    int        thread_id;
    errorObj   *ret_obj;
    
    msAcquireLock( TLOCK_ERROROBJ );
    
    thread_id = msGetThreadId();

    /* find link for this thread */
    
    for( link = error_list; 
         link != NULL && link->thread_id != thread_id
             && link->next != NULL && link->next->thread_id != thread_id;
         link = link->next ) {}

    /* If the target thread link is already at the head of the list were ok */
    if( error_list != NULL && error_list->thread_id == thread_id )
    {
    }

    /* We don't have one ... initialize one. */
    else if( link == NULL || link->next == NULL )
    {
        te_info_t *new_link;
        errorObj   error_obj = { -1, "", "" };

        new_link = (te_info_t *) malloc(sizeof(te_info_t));
        new_link->next = error_list;
        new_link->thread_id = thread_id;
        new_link->ms_error = error_obj;

        error_list = new_link;
    }

    /* If the link is not already at the head of the list, promote it */
    else if( link != NULL && link->next != NULL )
    {
        te_info_t *target = link->next;

        link->next = link->next->next;
        target->next = error_list;
        error_list = target;
    }

    ret_obj = &(error_list->ms_error);

    msReleaseLock( TLOCK_ERROROBJ ); 

    return ret_obj;
}
#endif

char *msGetErrorString(int code) {
  
  if(code<0 || code>MS_NUMERRORCODES-1)
    return("Invalid error code.");

  return(ms_errorCodes[code]);
}

void msSetError(int code, char *message_fmt, char *routine, ...)
{
  char *errfile=NULL;
  FILE *errstream;
  time_t errtime;
  errorObj *ms_error = msGetErrorObj();
  va_list args;

  ms_error->code = code;

  if(!routine)
    strcpy(ms_error->routine, "");
  else
    strncpy(ms_error->routine, routine, ROUTINELENGTH);

  if(!message_fmt)
    strcpy(ms_error->message, "");
  else
  {
    va_start(args, routine);
    vsprintf( ms_error->message, message_fmt, args );
    va_end(args);
  }

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
    fprintf(errstream, "%s - %s: %s %s\n", chop(ctime(&errtime)), ms_error->routine, ms_errorCodes[ms_error->code], ms_error->message);
    fclose(errstream);
  }
}

void msWriteError(FILE *stream)
{
  errorObj *ms_error = msGetErrorObj();

  fprintf(stream, "%s: %s %s\n", ms_error->routine, ms_errorCodes[ms_error->code], ms_error->message);
}

char *msGetVersion() {
  static char version[384];

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
#if defined (USE_GD_TTF) || defined (USE_GD_FT)
  strcat(version, " SUPPORTS=TTF");
#endif
#ifdef USE_WMS_SVR
  strcat(version, " SUPPORTS=WMS_SERVER");
#endif
#ifdef USE_WMS_LYR
  strcat(version, " SUPPORTS=WMS_CLIENT");
#endif
#ifdef USE_WFS_SVR
  strcat(version, " SUPPORTS=WFS_SERVER");
#endif
#ifdef USE_WFS_LYR
  strcat(version, " SUPPORTS=WFS_CLIENT");
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
#ifdef USE_POSTGIS
  strcat(version, " INPUT=POSTGIS");
#endif
#ifdef USE_ORACLESPATIAL
  strcat(version, " INPUT=ORACLESPATIAL"); 
#endif
#ifdef USE_OGR
  strcat(version, " INPUT=OGR");
#endif
#ifdef USE_GDAL
  strcat(version, " INPUT=GDAL");
#endif
  strcat(version, " INPUT=SHAPEFILE");

  return(version);
}

void msWebDebug( const char * pszFormat, ... )
{
#ifndef _WIN32
    va_list args;
    struct timeval tv;

    fprintf(stdout, "Content-type: text/html%c%c",10,10);

    gettimeofday(&tv, NULL);
    fprintf(stdout, "[%s].%ld ", chop(ctime(&(tv.tv_sec))), tv.tv_usec);

    va_start(args, pszFormat);
    vfprintf(stdout, pszFormat, args);
    va_end(args);

    exit(0);
#endif
}

void msDebug( const char * pszFormat, ... )
{
#ifdef ENABLE_STDERR_DEBUG
    va_list args;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    fprintf(stderr, "[%s].%ld ", chop(ctime(&(tv.tv_sec))), tv.tv_usec);

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
#endif
}

