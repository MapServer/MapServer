#ifndef CGIUTIL_H
#define CGIUTIL_H

/*
** Misc. defines
*/
#define MAX_PARAMS 10000

enum MS_REQUEST_TYPE {MS_GET_REQUEST, MS_POST_REQUEST};

//structure to hold request information
typedef struct
{
  char **ParamNames;
  char **ParamValues;
  int NumParams;

  enum MS_REQUEST_TYPE type;
  char *contenttype;

  char *postrequest;
}cgiRequestObj;
      

/*
** Function prototypes
*/ 
int loadParams(cgiRequestObj *);
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

cgiRequestObj *msAllocCgiObj();

#endif /* CGIUTIL_H */
