#ifndef CGIUTIL_H
#define CGIUTIL_H

/*
** Misc. defines
*/
#define MAX_PARAMS 10000

/*
** Function prototypes
*/ 
int loadParams(char **, char **);
void getword(char *, char *, char);
char *makeword(char *, char);
char *fmakeword(FILE *, char, int *);
char x2c(char *);
void unescape_url(char *);
void plustospace(char *);
int rind(char *, char);
int getline(char *, int, FILE *);
void send_fd(FILE *, FILE *);
int ind(char *, char);
void escape_shell_cmd(char *);

#endif /* CGIUTIL_H */
