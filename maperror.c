#include "map.h"
#include "maperror.h"

static char *ms_errorCodes[21]={"",
                               "Unable to access file.",
			       "Memory allocation error.",
			       "Incorrect data type.",
			       "Incorrect symbol.",
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
			       "Expression parser error."
};

errorObj ms_error = {-1, "", ""};

void msSetError(int code, char *message, char *routine)
{
  ms_error.code = code;

  if(!routine)
    strcpy(ms_error.routine, "");
  else
    strncpy(ms_error.routine, routine, ROUTINELENGTH);

  if(!message)
    strcpy(ms_error.message, "");
  else
    strncpy(ms_error.message, message, MESSAGELENGTH);
}

void msWriteError(FILE *stream)
{
  fprintf(stream, "%s: %s %s\n", ms_error.routine, ms_errorCodes[ms_error.code], ms_error.message);
}
