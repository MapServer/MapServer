/* $Id$ */

#include "map.h"
#include "maperror.h"
#include "mapthread.h"
#include "maptime.h"

#include "gdfonts.h"

#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <stdarg.h>

#ifdef NEED_NONBLOCKING_STDERR
#include <fcntl.h>
#endif

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
						"WFS connection error.",
						"WMS Map Context error.",
						"HTTP request error.",
						"Child array error.",
						"WCS server error."
};

#ifndef USE_THREAD

errorObj *msGetErrorObj()
{
    static errorObj ms_error = {MS_NOERR, "", "", NULL};

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
        errorObj   error_obj = { MS_NOERR, "", "", NULL };

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

/* msInsertErrorObj()
**
** We maintain a chained list of errorObj in which the first errorObj is
** the most recent (i.e. a stack).  msErrorReset() should be used to clear
** the list.
**
** Note that since some code in MapServer will fetch the head of the list and
** keep a handle on it for a while, the head of the chained list is static
** and never changes.
** A new errorObj is always inserted after the head, and only if the
** head of the list already contains some information.  i.e. If the static
** errorObj at the head of the list is empty then it is returned directly, 
** otherwise a new object is inserted after the head and the data that was in
** the head is moved to the new errorObj, freeing the head errorObj to receive
** the new error information.
*/
static errorObj *msInsertErrorObj()
{
  errorObj *ms_error;
  ms_error = msGetErrorObj();

  if (ms_error->code != MS_NOERR)
  {
      /* Head of the list already in use, insert a new errorObj after the head
       * and move head contents to this new errorObj, freeing the errorObj
       * for reuse.
       */
      errorObj *new_error;
      new_error = (errorObj *)malloc(sizeof(errorObj));

      /* Note: if malloc() failed then we simply do nothing and the head will
       * be overwritten by the caller... we cannot produce an error here 
       * since we are already inside a msSetError() call.
       */
      if (new_error)
      {
          new_error->next = ms_error->next;
          new_error->code = ms_error->code;
          strcpy(new_error->routine, ms_error->routine);
          strcpy(new_error->message, ms_error->message);

          ms_error->next = new_error;
          ms_error->code = MS_NOERR;
          ms_error->routine[0] = '\0';
          ms_error->message[0] = '\0';
      }
  }

  return ms_error;
}

/* msResetErrorList()
**
** Clear the list of error objects.
*/
void msResetErrorList()
{
  errorObj *ms_error, *this_error;
  ms_error = msGetErrorObj();

  this_error = ms_error->next;
  while( this_error != NULL)
  {
      errorObj *next_error;

      next_error = this_error->next;
      msFree(this_error);
      this_error = next_error;
  }

  ms_error->next = NULL;
  ms_error->code = MS_NOERR;
  ms_error->routine[0] = '\0';
  ms_error->message[0] = '\0';
}

char *msGetErrorCodeString(int code) {
  
  if(code<0 || code>MS_NUMERRORCODES-1)
    return("Invalid error code.");

  return(ms_errorCodes[code]);
}

char *msGetErrorString(char *delimiter) 
{
  char errbuf[256];
  char *errstr=NULL;

  errorObj *error = msGetErrorObj();

  if(!delimiter || !error) return(NULL);

  if((errstr = strdup("")) == NULL) return(NULL); // empty at first
  while(error && error->code != MS_NOERR) {
    if(error->next && error->next->code != MS_NOERR) // (peek ahead) more errors, use delimiter
      snprintf(errbuf, 255, "%s: %s %s%s", error->routine, ms_errorCodes[error->code], error->message, delimiter);
    else
      snprintf(errbuf, 255, "%s: %s %s", error->routine, ms_errorCodes[error->code], error->message);   

    if((errstr = (char *) realloc(errstr, sizeof(char)*(strlen(errstr)+strlen(errbuf)+1))) == NULL) return(NULL);
    strcat(errstr, errbuf);

    error = error->next;   
  }

  return(errstr);
}

void msSetError(int code, const char *message_fmt, const char *routine, ...)
{
  char *errfile=NULL;
  FILE *errstream;
  time_t errtime;
  errorObj *ms_error = msInsertErrorObj();
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

  while (ms_error && ms_error->code != MS_NOERR)
  {
      fprintf(stream, "%s: %s %s <br>\n", ms_error->routine, ms_errorCodes[ms_error->code], ms_error->message);
      ms_error = ms_error->next;
  }
}

void msWriteErrorXML(FILE *stream)
{
  errorObj *ms_error = msGetErrorObj();

  while (ms_error && ms_error->code != MS_NOERR)
  {
      fprintf(stream, "%s: %s %s\n", ms_error->routine, ms_errorCodes[ms_error->code], ms_error->message);
      ms_error = ms_error->next;
  }
}

void msWriteErrorImage(mapObj *map, char *filename, int blank) {
  gdFontPtr font = gdFontSmall;
  gdImagePtr img=NULL;
  int width=400, height=300, color;
  int nMargin =5;
  int nTextLength = 0;
  int nUsableWidth = 0;
  int nMaxCharsPerLine = 0;
  int nLines = 0;
  int i = 0;
  int nStart = 0;
  int nEnd = 0;
  int nLength = 0;
  char **papszLines = NULL;
  int nXPos = 0;
  int nYPos = 0;
  int nWidthTxt = 0;
  int nSpaceBewteenLines = font->h;
  int nBlack = 0;   
  outputFormatObj *format = NULL;

  char errormsg[MESSAGELENGTH+ROUTINELENGTH+4];
  errorObj *ms_error = msGetErrorObj();

  if (map) {
      if( map->width != -1 && map->height != -1 )
      {
          width = map->width;
          height = map->height;
      }
      format = map->outputformat;
  }

  if (format == NULL) format = msCreateDefaultOutputFormat( NULL, "GD/PC256" );

  img = gdImageCreate(width, height);
  color = gdImageColorAllocate(img, map->imagecolor.red, 
                               map->imagecolor.green,
                               map->imagecolor.blue); // BG color
  nBlack = gdImageColorAllocate(img, 0,0,0); // Text color

  if (map->outputformat && map->outputformat->transparent)
    gdImageColorTransparent(img, 0);

  sprintf(errormsg, "%s: %s", ms_error->routine, ms_error->message);
  nTextLength = strlen(errormsg); 
  nWidthTxt  =  nTextLength * font->w;
  nUsableWidth = width - (nMargin*2);

  // Check to see if it all fits on one line. If not, split the text on several lines.
  if(!blank) {
    if (nWidthTxt > nUsableWidth) {
      nMaxCharsPerLine =  nUsableWidth/font->w;
      nLines = (int) ceil ((double)nTextLength / (double)nMaxCharsPerLine);
      if (nLines > 0) {
        papszLines = (char **)malloc(nLines*sizeof(char *));
        for (i=0; i<nLines; i++) {
          papszLines[i] = (char *)malloc((nMaxCharsPerLine+1)*sizeof(char));
          papszLines[i][0] = '\0';
        }
      }
      for (i=0; i<nLines; i++) {
        nStart = i*nMaxCharsPerLine;
        nEnd = nStart + nMaxCharsPerLine;
        if (nStart < nTextLength) {
          if (nEnd > nTextLength)
            nEnd = nTextLength;
          nLength = nEnd-nStart;

          strncpy(papszLines[i], errormsg+nStart, nLength);
          papszLines[i][nLength+1] = '\0';
        }
      }
    } else {
      nLines = 1;
      papszLines = (char **)malloc(nLines*sizeof(char *));
      papszLines[0] = (char *)malloc((strlen(errormsg)+1)*sizeof(char));
      papszLines[0] = strcpy(papszLines[0], errormsg);
      papszLines[0][strlen(papszLines[0])+1]='\0';
    }   
    for (i=0; i<nLines; i++) {
      nYPos = (nSpaceBewteenLines) * ((i*2) +1); 
      nXPos = nSpaceBewteenLines;

      gdImageString(img, font, nXPos, nYPos, (unsigned char *)papszLines[i], nBlack);
    }
    if (papszLines) {
      for (i=0; i<nLines; i++) {
	free(papszLines[i]);
      }
      free(papszLines);
    }
  }

  // actually write the image
  if(!filename) printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(format), 10,10);
  msSaveImageGD(img, filename, format);
  gdImageDestroy(img);

  if (format->refcount == 0)
    msFreeOutputFormat(format);
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
#ifdef USE_PDF
  strcat(version, " OUTPUT=PDF");
#endif
#ifdef USE_MING_FLASH
  strcat(version, " OUTPUT=SWF");
#endif
#ifdef USE_PROJ
  strcat(version, " SUPPORTS=PROJ");
#endif
#ifdef USE_GD_FT
  strcat(version, " SUPPORTS=FREETYPE");
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
#ifdef USE_WCS_SVR
  strcat(version, " SUPPORTS=WCS_SERVER");
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

    msGettimeofday(&tv, NULL);
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

#ifdef NEED_NONBLOCKING_STDERR
    static char nonblocking_set = 0;
    if (!nonblocking_set)
    {
        fcntl(fileno(stderr), F_SETFL, O_NONBLOCK);
        nonblocking_set = 1;
    }
#endif

    msGettimeofday(&tv, NULL);
    fprintf(stderr, "[%s].%ld ", chop(ctime(&(tv.tv_sec))), tv.tv_usec);

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
#endif
}

