#include "map.h"
#include "maperror.h"
#include <time.h>

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
