#ifndef CGIUTIL_H
#define CGIUTIL_H

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define MS_DLL_EXPORT     __declspec(dllexport)
#else
#define  MS_DLL_EXPORT
#endif

/*
** Misc. defines
*/
#define MAX_PARAMS 10000

enum MS_REQUEST_TYPE {MS_GET_REQUEST, MS_POST_REQUEST};

//structure to hold request information
typedef struct
{
#ifndef SWIG
  char **ParamNames;
  char **ParamValues;
#endif

#ifdef SWIG
%immutable;
#endif
  int NumParams;
#ifdef SWIG
%mutable;
#endif

  enum MS_REQUEST_TYPE type;
  char *contenttype;

  char *postrequest;
} cgiRequestObj;
      

/*
** Function prototypes
*/ 
MS_DLL_EXPORT  int loadParams(cgiRequestObj *);
void getword(char *, char *, char);
char *makeword_skip(char *, char, char);
char *makeword(char *, char);
char *fmakeword(FILE *, char, int *);
char x2c(char *);
void unescape_url(char *);
void plustospace(char *);
int rind(char *, char);
int _getline(char *, int, FILE *);
void send_fd(FILE *, FILE *);
int ind(char *, char);
void escape_shell_cmd(char *);

cgiRequestObj *msAllocCgiObj(void);
void msFreeCgiObj(cgiRequestObj *request);

#endif /* CGIUTIL_H */
