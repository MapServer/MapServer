/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  cgiRequestObj and CGI parameter parsing. 
 * Author:   Steve Lime and the MapServer team.
 *
 * Notes: Portions derived from NCSA HTTPd Server's example CGI programs (util.c).
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cgiutil.h"
#include "mapserver.h"

MS_CVSID("$Id$")

#define LF 10
#define CR 13

static char *readPostBody( cgiRequestObj *request ) 
{
  char *data; 
  size_t data_max, data_len;
  int chunk_size;

  msIO_needBinaryStdin();

  /* -------------------------------------------------------------------- */
  /*      If the length is provided, read in one gulp.                    */
  /* -------------------------------------------------------------------- */
  if( getenv("CONTENT_LENGTH") != NULL ) {
    data_max = (size_t) atoi(getenv("CONTENT_LENGTH"));
    /* Test for suspicious CONTENT_LENGTH (negative value or SIZE_MAX) */
    if( data_max >= SIZE_MAX ) {
      msIO_printf("Content-type: text/html%c%c",10,10);
      msIO_printf("Suspicious Content-Length.\n");
      exit( 1 );
    }
    data = (char *) malloc(data_max+1);
    if( data == NULL ) {
      msIO_printf("Content-type: text/html%c%c",10,10);
      msIO_printf("malloc() failed, Content-Length: %u unreasonably large?\n", data_max );
      exit( 1 );
    }

    if( (int) msIO_fread(data, 1, data_max, stdin) < data_max ) {
      msIO_printf("Content-type: text/html%c%c",10,10);
      msIO_printf("POST body is short\n");
      exit(1);
    }

    data[data_max] = '\0';
    return data;
  }

  /* -------------------------------------------------------------------- */
  /*      Otherwise read in chunks to the end.                            */
  /* -------------------------------------------------------------------- */
#define DATA_ALLOC_SIZE 10000

  data_max = DATA_ALLOC_SIZE;
  data_len = 0;
  data = (char *) malloc(data_max+1);

  while( (chunk_size = msIO_fread( data + data_len, 1, data_max-data_len, stdin )) > 0 ) {
    data_len += chunk_size;

    if( data_len == data_max ) {
      /* Realloc buffer, making sure we check for possible size_t overflow */
        if ( data_max > SIZE_MAX - (DATA_ALLOC_SIZE+1) ) {
        msIO_printf("Content-type: text/html%c%c",10,10);
        msIO_printf("Possible size_t overflow, cannot reallocate input buffer, POST body too large?\n" );
        exit(1);
      }

      data_max = data_max + DATA_ALLOC_SIZE;
      data = (char *) realloc(data, data_max+1);

      if( data == NULL ) {
        msIO_printf("Content-type: text/html%c%c",10,10);
        msIO_printf("out of memory trying to allocate %u input buffer, POST body too large?\n", data_max+1 );
        exit(1);
      }
    }
  }

  data[data_len] = '\0';  
  return data;
}

int loadParams(cgiRequestObj *request) {
  register int x,m=0;
  char *s;
  int debuglevel;

  if(getenv("REQUEST_METHOD")==NULL) {
    msIO_printf("This script can only be used to decode form results and \n");
    msIO_printf("should be initiated as a CGI process via a httpd server.\n");
    exit(0);
  }

  debuglevel = (int)msGetGlobalDebugLevel();

  if(strcmp(getenv("REQUEST_METHOD"),"POST") == 0) { /* we've got a post from a form */     
    char *post_data;

    request->type = MS_POST_REQUEST;

    s = getenv("CONTENT_TYPE"); 
    if (s != NULL)
      request->contenttype = strdup(s);
     /* we've to set default content-type which is
      * application/octet-stream according to
      * W3 RFC 2626 section 7.2.1 */
    else request->contenttype = strdup("application/octet-stream");

    post_data = readPostBody( request );
    if(strcmp(request->contenttype, "application/x-www-form-urlencoded")) 
      request->postrequest = post_data;
    else {
      int data_len = strlen(post_data);
      while( data_len > 0 && isspace(post_data[data_len-1]) )
        post_data[--data_len] = '\0';

      while( post_data[0] ) {
        if(m >= MS_MAX_CGI_PARAMS) {
          msIO_printf("Too many name/value pairs, aborting.\n");
          exit(0);
        }

        request->ParamValues[m] = makeword(post_data,'&');
        plustospace(request->ParamValues[m]);
        unescape_url(request->ParamValues[m]);
        request->ParamNames[m] = makeword(request->ParamValues[m],'=');
        m++;
      }
      free( post_data );
    }

    /* check the QUERY_STRING even in the post request since it can contain 
       information. Eg a wfs request with  */
    s = getenv("QUERY_STRING");
    if(s) {
      if (debuglevel >= MS_DEBUGLEVEL_DEBUG)
		  msDebug("loadParams() QUERY_STRING: %s\n", s);

      for(x=0;s[0] != '\0';x++) {       
        if(m >= MS_MAX_CGI_PARAMS) {
          msIO_printf("Too many name/value pairs, aborting.\n");
          exit(0);
        }
        request->ParamValues[m] = makeword(s,'&');
        plustospace(request->ParamValues[m]);
        unescape_url(request->ParamValues[m]);
        request->ParamNames[m] = makeword(request->ParamValues[m],'=');
        m++;
      }
    }     
  } else { 
    if(strcmp(getenv("REQUEST_METHOD"),"GET") == 0) { /* we've got a get request */
      request->type = MS_GET_REQUEST;

      s = getenv("QUERY_STRING");
      if(s == NULL) {
        msIO_printf("Content-type: text/html%c%c",10,10);
        msIO_printf("No query information to decode. QUERY_STRING not set.\n");	
        exit(1);
      }

	  if (debuglevel >= MS_DEBUGLEVEL_DEBUG)
		  msDebug("loadParams() QUERY_STRING: %s\n", s);

      if(strlen(s)==0) {
        msIO_printf("Content-type: text/html%c%c",10,10);
        msIO_printf("No query information to decode. QUERY_STRING is set, but empty.\n");
        exit(1);
      }

      for(x=0;s[0] != '\0';x++) {       
        if(m >= MS_MAX_CGI_PARAMS) {
          msIO_printf("Too many name/value pairs, aborting.\n");
          exit(0);
        }
        request->ParamValues[m] = makeword(s,'&');
        plustospace(request->ParamValues[m]);
        unescape_url(request->ParamValues[m]);
        request->ParamNames[m] = makeword(request->ParamValues[m],'=');
        m++;
      }
    } else {
      msIO_printf("Content-type: text/html%c%c",10,10);
      msIO_printf("This script should be referenced with a METHOD of GET or METHOD of POST.\n");
      exit(1);
    }
  }

  /* check for any available cookies */
  s = getenv("HTTP_COOKIE");
  if(s != NULL) {
    request->httpcookiedata = strdup(s);
    for(x=0;s[0] != '\0';x++) {
      if(m >= MS_MAX_CGI_PARAMS) {
        msIO_printf("Too many name/value pairs, aborting.\n");
        exit(0);
      }
      request->ParamValues[m] = makeword(s,';');
      plustospace(request->ParamValues[m]);
      unescape_url(request->ParamValues[m]);
      request->ParamNames[m] = makeword_skip(request->ParamValues[m],'=',' ');
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

char *makeword_skip(char *line, char stop, char skip) {
  int x = 0,y,offset=0;
  char *word = (char *) malloc(sizeof(char) * (strlen(line) + 1));

  for(x=0;((line[x]) && (line[x] == skip));x++);
  offset = x;

  for(x=offset;((line[x]) && (line[x] != stop));x++)
    word[x-offset] = line[x];

  word[x-offset] = '\0';
  if(line[x]) ++x;
  y=0;

  while((line[y++] = line[x++]));
  return word;
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

int _getline(char *s, int n, FILE *f) {
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

void send_fd(FILE *f, FILE *fd) {
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
    if(ind("&;`'\"|*?~<>^()[]{}$\\\n",cmd[x]) != -1) {
      for(y=l+1;y>x;y--)
        cmd[y] = cmd[y-1];
      l++; /* length has been increased */
      cmd[x] = '\\';
      x++; /* skip the character */
    }
  }
}

/*
** Allocate a new request holder structure
*/
cgiRequestObj *msAllocCgiObj() {
  cgiRequestObj *request = (cgiRequestObj *)malloc(sizeof(cgiRequestObj));

  if(!request)
    return NULL;

  request->ParamNames = NULL;
  request->ParamValues = NULL;
  request->NumParams = 0;
  request->type = -1;
  request->contenttype = NULL;
  request->postrequest = NULL;
  request->httpcookiedata = NULL;

  return request;
}
      
void msFreeCgiObj(cgiRequestObj *request) {
  msFreeCharArray(request->ParamNames, request->NumParams);
  msFreeCharArray(request->ParamValues, request->NumParams);
  request->ParamNames = NULL;
  request->ParamValues = NULL;
  request->NumParams = 0;
  request->type = -1;
  msFree(request->contenttype);
  msFree(request->postrequest);
  msFree(request->httpcookiedata);
  request->contenttype = NULL;
  request->postrequest = NULL;
  request->httpcookiedata = NULL;

  msFree(request);
}
