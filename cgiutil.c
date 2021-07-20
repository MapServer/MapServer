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
#include "mapserver.h"
#include "cgiutil.h"

#define LF 10
#define CR 13

int readPostBody( cgiRequestObj *request, char **data )
{
  size_t data_max, data_len;
  int chunk_size;

  (void)request;

  msIO_needBinaryStdin();

  /* -------------------------------------------------------------------- */
  /*      If the length is provided, read in one gulp.                    */
  /* -------------------------------------------------------------------- */
  if( getenv("CONTENT_LENGTH") != NULL ) {
    data_max = (size_t) atoi(getenv("CONTENT_LENGTH"));
    /* Test for suspicious CONTENT_LENGTH (negative value or SIZE_MAX) */
    if( data_max >= SIZE_MAX ) {
      // msIO_setHeader("Content-Type","text/html");
      // msIO_sendHeaders();
      // msIO_printf("Suspicious Content-Length.\n");
      msSetError(MS_WEBERR, "Suspicious Content-Length.", "readPostBody()");
      return MS_FAILURE;
    }
    *data = (char *) malloc(data_max+1);
    if( *data == NULL ) {
      // msIO_setHeader("Content-Type","text/html");
      // msIO_sendHeaders();
      // msIO_printf("malloc() failed, Content-Length: %u unreasonably large?\n", (unsigned int)data_max );
      msSetError(MS_WEBERR, "malloc() failed, Content-Length: %u unreasonably large?", "readPostBody()", (unsigned int)data_max);
      return MS_FAILURE;
    }

    if( (int) msIO_fread(*data, 1, data_max, stdin) < (int) data_max ) {
      // msIO_setHeader("Content-Type","text/html");
      // msIO_sendHeaders();
      // msIO_printf("POST body is short\n");
      msSetError(MS_WEBERR, "POST body is short.", "readPostBody()");
      return MS_FAILURE;
    }

    (*data)[data_max] = '\0';
    return MS_SUCCESS;
  }
  /* -------------------------------------------------------------------- */
  /*      Otherwise read in chunks to the end.                            */
  /* -------------------------------------------------------------------- */
#define DATA_ALLOC_SIZE 10000

  data_max = DATA_ALLOC_SIZE;
  data_len = 0;
  *data = (char *) msSmallMalloc(data_max+1);
  (*data)[data_max] = '\0';

  while( (chunk_size = msIO_fread( *data + data_len, 1, data_max-data_len, stdin )) > 0 ) {
    data_len += chunk_size;

    if( data_len == data_max ) {
      /* Realloc buffer, making sure we check for possible size_t overflow */
      if ( data_max > SIZE_MAX - (DATA_ALLOC_SIZE+1) ) {
        // msIO_setHeader("Content-Type","text/html");
        // msIO_sendHeaders();
        // msIO_printf("Possible size_t overflow, cannot reallocate input buffer, POST body too large?\n" );
	msSetError(MS_WEBERR, "Possible size_t overflow, cannot reallocate input buffer, POST body too large?", "readPostBody()");
        return MS_FAILURE;
      }

      data_max = data_max + DATA_ALLOC_SIZE;
      *data = (char *) msSmallRealloc(*data, data_max+1);
    }
  }

  (*data)[data_len] = '\0';
  return MS_SUCCESS;
}

static char* msGetEnv(const char *name, void* thread_context)
{
  (void)thread_context;
  return getenv(name);
}

int loadParams(cgiRequestObj *request,
               char* (*getenv2)(const char*, void* thread_context),
               char *raw_post_data,
               ms_uint32 raw_post_data_length,
               void* thread_context)
{
  register int x,m=0;
  char *s, *queryString = NULL, *httpCookie = NULL;
  int debuglevel;
  int maxParams = MS_DEFAULT_CGI_PARAMS;

  if (getenv2==NULL)
    getenv2 = &msGetEnv;

  if(getenv2("REQUEST_METHOD", thread_context)==NULL) {
    msIO_printf("This script can only be used to decode form results and \n");
    msIO_printf("should be initiated as a CGI process via a httpd server.\n");
    msIO_printf("For other options please try using the --help switch.\n");
    return -1;
  }

  debuglevel = (int)msGetGlobalDebugLevel();

  if(strcmp(getenv2("REQUEST_METHOD", thread_context),"POST") == 0) { /* we've got a post from a form */
    char *post_data;
    int data_len;
    request->type = MS_POST_REQUEST;

    if (request->contenttype == NULL){
        s = getenv2("CONTENT_TYPE", thread_context);
        if (s != NULL){
          request->contenttype = msStrdup(s);
        }
        else {
            /* we've to set default Content-Type which is
             * application/octet-stream according to
             * W3 RFC 2626 section 7.2.1 */
            request->contenttype = msStrdup("application/octet-stream");
        }
    }

    if (raw_post_data) {
      post_data = msStrdup(raw_post_data);
      data_len = raw_post_data_length;
    } else {
      if(MS_SUCCESS != readPostBody( request, &post_data ))
        return -1;
      data_len = strlen(post_data);
    }

    /* if the content_type is application/x-www-form-urlencoded,
       we have to parse it like the QUERY_STRING variable */
    if(strncmp(request->contenttype, "application/x-www-form-urlencoded", strlen("application/x-www-form-urlencoded")) == 0) {
      while( data_len > 0 && isspace(post_data[data_len-1]) )
        post_data[--data_len] = '\0';

      while( post_data[0] ) {
        if(m >= maxParams) {
          maxParams *= 2;
          request->ParamNames = (char **) msSmallRealloc(request->ParamNames,sizeof(char *) * maxParams);
          request->ParamValues = (char **) msSmallRealloc(request->ParamValues,sizeof(char *) * maxParams);
        }
        request->ParamValues[m] = makeword(post_data,'&');
        plustospace(request->ParamValues[m]);
        unescape_url(request->ParamValues[m]);
        request->ParamNames[m] = makeword(request->ParamValues[m],'=');
        m++;
      }
      free( post_data );
    } else
      request->postrequest = post_data;

    /* check the QUERY_STRING even in the post request since it can contain
       information. Eg a wfs request with  */
    s = getenv2("QUERY_STRING", thread_context);
    if(s) {
      if (debuglevel >= MS_DEBUGLEVEL_DEBUG)
        msDebug("loadParams() QUERY_STRING: %s\n", s);

      queryString = msStrdup(s);
      for(x=0; queryString[0] != '\0'; x++) {
        if(m >= maxParams) {
          maxParams *= 2;
          request->ParamNames = (char **) msSmallRealloc(request->ParamNames,sizeof(char *) * maxParams);
          request->ParamValues = (char **) msSmallRealloc(request->ParamValues,sizeof(char *) * maxParams);
        }
        request->ParamValues[m] = makeword(queryString,'&');
        plustospace(request->ParamValues[m]);
        unescape_url(request->ParamValues[m]);
        request->ParamNames[m] = makeword(request->ParamValues[m],'=');
        m++;
      }
    }
  } else {
    if(strcmp(getenv2("REQUEST_METHOD", thread_context),"GET") == 0) { /* we've got a get request */
      request->type = MS_GET_REQUEST;

      s = getenv2("QUERY_STRING", thread_context);
      if(s == NULL) {
        // msIO_setHeader("Content-Type","text/html");
        // msIO_sendHeaders();
        // msIO_printf("No query information to decode. QUERY_STRING not set.\n");
	msSetError(MS_WEBERR, "No query information to decode. QUERY_STRING not set.", "loadParams()");
        return -1;
      }

      if (debuglevel >= MS_DEBUGLEVEL_DEBUG)
        msDebug("loadParams() QUERY_STRING: %s\n", s);

      if(strlen(s)==0) {
        // msIO_setHeader("Content-Type","text/html");
        // msIO_sendHeaders();
        // msIO_printf("No query information to decode. QUERY_STRING is set, but empty.\n");
	msSetError(MS_WEBERR, "No query information to decode. QUERY_STRING is set, but empty.", "loadParams()");
        return -1;
      }

      /* don't modify the string returned by getenv2 */
      queryString = msStrdup(s);
      for(x=0; queryString[0] != '\0'; x++) {
        if(m >= maxParams) {
          maxParams *= 2;
          request->ParamNames = (char **) msSmallRealloc(request->ParamNames,sizeof(char *) * maxParams);
          request->ParamValues = (char **) msSmallRealloc(request->ParamValues,sizeof(char *) * maxParams);
        }
        request->ParamValues[m] = makeword(queryString,'&');
        plustospace(request->ParamValues[m]);
        unescape_url(request->ParamValues[m]);
        request->ParamNames[m] = makeword(request->ParamValues[m],'=');
        m++;
      }
    } else {
      // msIO_setHeader("Content-Type","text/html");
      // msIO_sendHeaders();
      // msIO_printf("This script should be referenced with a METHOD of GET or METHOD of POST.\n");
      msSetError(MS_WEBERR, "This script should be referenced with a METHOD of GET or METHOD of POST.", "loadParams()");
      return -1;
    }
  }

  /* check for any available cookies */
  s = getenv2("HTTP_COOKIE", thread_context);
  if(s != NULL) {
    httpCookie = msStrdup(s);
    request->httpcookiedata = msStrdup(s);
    for(x=0; httpCookie[0] != '\0'; x++) {
      if(m >= maxParams) {
        maxParams *= 2;
        request->ParamNames = (char **) msSmallRealloc(request->ParamNames,sizeof(char *) * maxParams);
        request->ParamValues = (char **) msSmallRealloc(request->ParamValues,sizeof(char *) * maxParams);
      }
      request->ParamValues[m] = makeword(httpCookie,';');
      plustospace(request->ParamValues[m]);
      unescape_url(request->ParamValues[m]);
      request->ParamNames[m] = makeword_skip(request->ParamValues[m],'=',' ');
      m++;
    }
  }

  if (queryString)
    free(queryString);
  if (httpCookie)
    free(httpCookie);

  return(m);
}

void getword(char *word, char *line, char stop)
{
  int x = 0,y;

  for(x=0; ((line[x]) && (line[x] != stop)); x++)
    word[x] = line[x];

  word[x] = '\0';
  if(line[x]) ++x;
  y=0;

  while((line[y++] = line[x++]));
}

char *makeword_skip(char *line, char stop, char skip)
{
  int x = 0,y,offset=0;
  char *word = (char *) msSmallMalloc(sizeof(char) * (strlen(line) + 1));

  for(x=0; ((line[x]) && (line[x] == skip)); x++);
  offset = x;

  for(x=offset; ((line[x]) && (line[x] != stop)); x++)
    word[x-offset] = line[x];

  word[x-offset] = '\0';
  if(line[x]) ++x;
  y=0;

  while((line[y++] = line[x++]));
  return word;
}

char *makeword(char *line, char stop)
{
  int x = 0,y;
  char *word = (char *) msSmallMalloc(sizeof(char) * (strlen(line) + 1));

  for(x=0; ((line[x]) && (line[x] != stop)); x++)
    word[x] = line[x];

  word[x] = '\0';
  if(line[x]) ++x;
  y=0;

  while((line[y++] = line[x++]));
  return word;
}

char *fmakeword(FILE *f, char stop, int *cl)
{
  int wsize;
  char *word;
  int ll;

  wsize = 102400;
  ll=0;
  word = (char *) msSmallMalloc(sizeof(char) * (wsize + 1));

  while(1) {
    word[ll] = (char)fgetc(f);
    if(ll==wsize) {
      word[ll+1] = '\0';
      wsize+=102400;
      word = (char *)msSmallRealloc(word,sizeof(char)*(wsize+1));
    }
    --(*cl);
    if((word[ll] == stop) || (feof(f)) || (!(*cl))) {
      if(word[ll] != stop) ll++;
      word[ll] = '\0';
      word = (char *) msSmallRealloc(word, ll+1);
      return word;
    }
    ++ll;
  }
}

char x2c(char *what)
{
  register char digit;

  digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
  digit *= 16;
  digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
  return(digit);
}

void unescape_url(char *url)
{
  register int x,y;

  for(x=0,y=0; url[y]; ++x,++y) {
    if((url[x] = url[y]) == '%') {
      url[x] = x2c(&url[y+1]);
      y+=2;
    }
  }
  url[x] = '\0';
}

void plustospace(char *str)
{
  register int x;

  for(x=0; str[x]; x++) if(str[x] == '+') str[x] = ' ';
}

int rind(char *s, char c)
{
  register int x;
  for(x=strlen(s) - 1; x != -1; x--)
    if(s[x] == c) return x;
  return -1;
}

void send_fd(FILE *f, FILE *fd)
{
  int c;

  while (1) {
    c = fgetc(f);
    if(c == EOF)
      return;
    fputc((char)c,fd);
  }
}

int ind(char *s, char c)
{
  register int x;

  for(x=0; s[x]; x++)
    if(s[x] == c) return x;

  return -1;
}

/*
** patched version according to CERT advisory...
*/
void escape_shell_cmd(char *cmd)
{
  register int x,y,l;

  l=strlen(cmd);
  for(x=0; cmd[x]; x++) {
    if(ind("&;`'\"|*?~<>^()[]{}$\\\n",cmd[x]) != -1) {
      for(y=l+1; y>x; y--)
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
cgiRequestObj *msAllocCgiObj()
{
  cgiRequestObj *request = (cgiRequestObj *)malloc(sizeof(cgiRequestObj));

  if(!request)
    return NULL;

  request->ParamNames = (char **) msSmallMalloc(MS_DEFAULT_CGI_PARAMS*sizeof(char*));
  request->ParamValues = (char **) msSmallMalloc(MS_DEFAULT_CGI_PARAMS*sizeof(char*));
  request->NumParams = 0;
  request->type = MS_GET_REQUEST;
  request->contenttype = NULL;
  request->postrequest = NULL;
  request->httpcookiedata = NULL;

  request->path_info = NULL;
  request->api_path = NULL;
  request->api_path_length = 0;

  return request;
}

void msFreeCgiObj(cgiRequestObj *request)
{
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

  if(request->api_path) {
    msFreeCharArray(request->api_path, request->api_path_length);
    request->api_path = NULL;
    request->api_path_length = 0;
  }

  msFree(request);
}
