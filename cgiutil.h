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
MS_DLL_EXPORT void getword(char *, char *, char);
MS_DLL_EXPORT char *makeword_skip(char *, char, char);
MS_DLL_EXPORT char *makeword(char *, char);
MS_DLL_EXPORT char *fmakeword(FILE *, char, int *);
MS_DLL_EXPORT char x2c(char *);
MS_DLL_EXPORT void unescape_url(char *);
MS_DLL_EXPORT void plustospace(char *);
MS_DLL_EXPORT int rind(char *, char);
MS_DLL_EXPORT int _getline(char *, int, FILE *);
MS_DLL_EXPORT void send_fd(FILE *, FILE *);
MS_DLL_EXPORT int ind(char *, char);
MS_DLL_EXPORT void escape_shell_cmd(char *);

MS_DLL_EXPORT cgiRequestObj *msAllocCgiObj(void);
MS_DLL_EXPORT void msFreeCgiObj(cgiRequestObj *request);

#endif /* CGIUTIL_H */
