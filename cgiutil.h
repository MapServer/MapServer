#ifndef CGIUTIL_H
#define CGIUTIL_H

/*
** Misc. defines
*/
#define MAX_ENTRIES 10000

/*
** Structure definitions
*/ 
typedef struct {
    char *name;
    char *val;
} entry;

/*
** Function prototypes
*/ 
int loadEntries(entry *);
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
char *encode_url(char *);

#endif /* CGIUTIL_H */
