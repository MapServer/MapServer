#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cgiutil.h"

#define LF 10
#define CR 13

int loadParams(char **names, char **values) {
  register int x,m=0;
  int cl;
  char *s;

  if(getenv("REQUEST_METHOD")==NULL) {
    printf("This script can only be used to decode form results and \n");
    printf("should be initiated as a CGI process via a httpd server.\n");
    exit(0);
  }

  if(strcmp(getenv("REQUEST_METHOD"),"POST") == 0) { /* we've got a post from a form */     
    if(strcmp(getenv("CONTENT_TYPE"),"application/x-www-form-urlencoded")) {
      printf("Content-type: text/html%c%c",10,10);
      printf("This script can only be used to decode form results. \n");
      exit(1);
    }
    cl = atoi(getenv("CONTENT_LENGTH"));
 
    for(x=0;cl && (!feof(stdin));x++) {
      values[m] = fmakeword(stdin,'&',&cl);
      plustospace(values[m]);
      unescape_url(values[m]);
      names[m] = makeword(values[m],'=');
      m++;
    }
  } else { 
    if(strcmp(getenv("REQUEST_METHOD"),"GET") == 0) { /* we've got a get request */

      s = getenv("QUERY_STRING");
      if(s == NULL) {
	printf("Content-type: text/html%c%c",10,10);
	printf("No query information to decode. QUERY_STRING not set.\n");	
	exit(1);
      }
      if(strlen(s)==0) {
	printf("Content-type: text/html%c%c",10,10);
	printf("No query information to decode. QUERY_STRING is set, but empty.\n");
	exit(1);
      }

      for(x=0;s[0] != '\0';x++) {       
        values[m] = makeword(s,'&');
        plustospace(values[m]);
        unescape_url(values[m]);
        names[m] = makeword(values[m],'=');
        m++;
      }
    } else {
      printf("Content-type: text/html%c%c",10,10);
      printf("This script should be referenced with a METHOD of GET or METHOD of POST.\n");
      exit(1);
    }
  }

  // check for any available cookies
  s = getenv("HTTP_COOKIE");
  if(s == NULL) {
    for(x=0;s[0] != '\0';x++) {
      values[m] = makeword(s,';');
      plustospace(values[m]);
      unescape_url(values[m]);
      names[m] = makeword(values[m],'=');
      m++;
    }
  }

  return(m);
}

void getword(char *word, char *line, char stop) {
    int x = 0,y;

    for(x=0;((line[x]) && (line[x] != stop));x++)
        word[x] = line[x];

    word[x] = '\0';
    if(line[x]) ++x;
    y=0;

    while((line[y++] = line[x++]));
}

char *makeword(char *line, char stop) {
    int x = 0,y;
    char *word = (char *) malloc(sizeof(char) * (strlen(line) + 1));

    for(x=0;((line[x]) && (line[x] != stop));x++)
        word[x] = line[x];

    word[x] = '\0';
    if(line[x]) ++x;
    y=0;

    while((line[y++] = line[x++]));
    return word;
}

char *fmakeword(FILE *f, char stop, int *cl) {
    int wsize;
    char *word;
    int ll;

    wsize = 102400;
    ll=0;
    word = (char *) malloc(sizeof(char) * (wsize + 1));

    while(1) {
        word[ll] = (char)fgetc(f);
        if(ll==wsize) {
            word[ll+1] = '\0';
            wsize+=102400;
            word = (char *)realloc(word,sizeof(char)*(wsize+1));
        }
        --(*cl);
        if((word[ll] == stop) || (feof(f)) || (!(*cl))) {
            if(word[ll] != stop) ll++;
            word[ll] = '\0';
	    word = (char *) realloc(word, ll+1);
            return word;
        }
        ++ll;
    }
}

char x2c(char *what) {
    register char digit;

    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
    return(digit);
}

void unescape_url(char *url) {
    register int x,y;

    for(x=0,y=0;url[y];++x,++y) {
        if((url[x] = url[y]) == '%') {
            url[x] = x2c(&url[y+1]);
            y+=2;
        }
    }
    url[x] = '\0';
}

void plustospace(char *str) {
    register int x;

    for(x=0;str[x];x++) if(str[x] == '+') str[x] = ' ';
}

int rind(char *s, char c) {
    register int x;
    for(x=strlen(s) - 1;x != -1; x--)
        if(s[x] == c) return x;
    return -1;
}

int getline(char *s, int n, FILE *f) {
    register int i=0;

    while(1) {
        s[i] = (char)fgetc(f);

        if(s[i] == CR)
            s[i] = fgetc(f);

        if((s[i] == 0x4) || (s[i] == LF) || (i == (n-1))) {
            s[i] = '\0';
            return (feof(f) ? 1 : 0);
        }
        ++i;
    }
}

void send_fd(FILE *f, FILE *fd)
{
    char c;

    while (1) {
        c = fgetc(f);
        if(feof(f))
            return;
        fputc(c,fd);
    }
}

int ind(char *s, char c) {
    register int x;

    for(x=0;s[x];x++)
        if(s[x] == c) return x;

    return -1;
}

/*
** patched version according to CERT advisory...
*/
void escape_shell_cmd(char *cmd) {
    register int x,y,l;

    l=strlen(cmd);
    for(x=0;cmd[x];x++) {
        if(ind("&;`'\"|*?~<>^()[]{}$\\\n",cmd[x]) != -1){
            for(y=l+1;y>x;y--)
                cmd[y] = cmd[y-1];
            l++; /* length has been increased */
            cmd[x] = '\\';
            x++; /* skip the character */
        }
    }
}

