/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Various string handling functions.
 * Author:   Steve Lime and the MapServer team.
 *
 * Notes: A couple of string handling functions (strrstr, strlcat) were taken from
 * other sources. Copyright notices accompany those functions below.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
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

#include "mapserver.h"
#include "mapthread.h"

#include "cpl_vsi.h"

#include <ctype.h>
#include <string.h>
#include <errno.h>

/*
 * Find the first occurrence of find in s, ignore case.
 */

#ifdef USE_FRIBIDI
#if (defined(_WIN32) && !defined(__CYGWIN__)) || defined(HAVE_FRIBIDI2)
#include "fribidi.h"
#else
#include <fribidi/fribidi.h>
#endif
#define MAX_STR_LEN 65000
#endif

#ifdef USE_ICONV
#include <iconv.h>
#include <wchar.h>
#endif

#include "mapentities.h"

#ifndef HAVE_STRRSTR
/*
** Copyright (c) 2000-2004  University of Illinois Board of Trustees
** Copyright (c) 2000-2005  Mark D. Roth
** All rights reserved.
**
** Developed by: Campus Information Technologies and Educational Services,
** University of Illinois at Urbana-Champaign
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** ``Software''), to deal with the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** * Redistributions of source code must retain the above copyright
**   notice, this list of conditions and the following disclaimers.
**
** * Redistributions in binary form must reproduce the above copyright
**   notice, this list of conditions and the following disclaimers in the
**   documentation and/or other materials provided with the distribution.
**
** * Neither the names of Campus Information Technologies and Educational
**   Services, University of Illinois at Urbana-Champaign, nor the names
**   of its contributors may be used to endorse or promote products derived
**   from this Software without specific prior written permission.
**
** THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
** ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
** OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
*/
char *strrstr(const char *string, const char *find)
{
  size_t stringlen, findlen;
  const char *cp;

  findlen = strlen(find);
  stringlen = strlen(string);
  if (findlen > stringlen)
    return NULL;

  for (cp = string + stringlen - findlen; cp >= string; cp--)
    if (strncmp(cp, find, findlen) == 0)
      return (char*) cp;

  return NULL;
}
#endif

#ifndef HAVE_STRLCAT
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MS_MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t strlcat(char *dst, const char *src, size_t siz)
{
  register char *d = dst;
  register const char *s = src;
  register size_t n = siz;
  size_t dlen;

  /* Find the end of dst and adjust bytes left but don't go past end */
  while (n-- != 0 && *d != '\0')
    d++;
  dlen = d - dst;
  n = siz - dlen;

  if (n == 0)
    return(dlen + strlen(s));
  while (*s != '\0') {
    if (n != 1) {
      *d++ = *s;
      n--;
    }
    s++;
  }
  *d = '\0';

  return(dlen + (s - src));/* count does not include NUL */
}
#endif

#ifndef HAVE_STRLCPY
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
  register char *d = dst;
  register const char *s = src;
  register size_t n = siz;

  /* Copy as many bytes as will fit */
  if (n != 0 && --n != 0) {
    do {
      if ((*d++ = *s++) == 0)
        break;
    } while (--n != 0);
  }

  /* Not enough room in dst, add NUL and traverse rest of src */
  if (n == 0) {
    if (siz != 0)
      *d = '\0';              /* NUL-terminate dst */
    while (*s++)
      ;
  }

  return(s - src - 1);    /* count does not include NUL */
}
#endif

#ifndef HAVE_STRCASESTR
/*-
 * Copyright (c) 1990, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
char *strcasestr(const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0) {
    c = tolower((unsigned char)c);
    len = strlen(find);
    do {
      do {
        if ((sc = *s++) == 0)
          return (NULL);
      } while ((char)tolower((unsigned char)sc) != c);
    } while (strncasecmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}
#endif

#ifndef HAVE_STRNCASECMP
int strncasecmp(const char *s1, const char *s2, int len)
{
  register const char *cp1, *cp2;
  int cmp = 0;

  cp1 = s1;
  cp2 = s2;

  if(len == 0)
    return(0);

  if (!*cp1)
    return -1;
  else if (!*cp2)
    return 1;

  while(*cp1 && *cp2 && len) {
    if((cmp = (toupper(*cp1) - toupper(*cp2))) != 0)
      return(cmp);
    cp1++;
    cp2++;
    len--;
  }

  if(len == 0) {
    return(0);
  }
  if(*cp1 || *cp2) {
    if (*cp1)
      return(1);
    else
      return (-1);
  }
  return(0);
}
#endif

#ifndef HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2)
{
  register const char *cp1, *cp2;
  int cmp = 0;

  cp1 = s1;
  cp2 = s2;
  if ((!cp1) || (!cp2 )) {
    return (0);
  }
  while(*cp1 && *cp2) {
    if((cmp = (toupper(*cp1) - toupper(*cp2))) != 0)
      return(cmp);
    cp1++;
    cp2++;
  }
  if(*cp1 || *cp2) {
    if (*cp1)
      return(1);
    else
      return (-1);
  }

  return(0);
}
#endif

char *msLongToString(long value)
{
  size_t bufferSize = 256;
  char *buffer = (char*)msSmallMalloc(bufferSize);

  snprintf(buffer, bufferSize, "%ld", value);
  return(buffer);
}

char *msDoubleToString(double value, int force_f)
{
  size_t bufferSize = 256;
  char *buffer = (char*)msSmallMalloc(bufferSize);

  if (force_f == MS_TRUE)
    snprintf(buffer, bufferSize, "%f", value);
  else
    snprintf(buffer, bufferSize, "%g", value);
  return(buffer);
}

char *msIntToString(int value)
{
  size_t bufferSize = 256;
  char *buffer = (char*)msSmallMalloc(bufferSize);

  snprintf(buffer, bufferSize, "%i", value);
  return(buffer);
}

void msStringToUpper(char *string)
{
  if (string != NULL) {
    for (; *string; ++string)
      *string = toupper(*string);
  }
}

void msStringToLower(char *string)
{
  if (string != NULL) {
    for (; *string; ++string)
      *string = tolower(*string);
  }
}

/**
 * Force the first character to uppercase and the rest of the characters to
 * lower case for EACH word in the string.
 */
void msStringInitCap(char *string)
{
  int i;
  int start = 1; 
  if (string != NULL) {
    for (i = 0; i < (int)strlen(string); i++) {
      if (string[i] == ' ')
        start = 1;
      else if (start) {
        string[i] = toupper(string[i]);
        start = 0;
      }
      else {
        string[i] = tolower(string[i]);
      }
    }
  }
}

/**
 * Force the first character to uppercase for the FIRST word in the string
 * and the rest of the characters to lower case.
 */
void msStringFirstCap(char *string)
{
  int i;
  int start = 1; 
  if (string != NULL) {
    for (i = 0; i < (int)strlen(string); i++) {
      if (string[i] != ' ') {
        if (start) {
          string[i] = toupper(string[i]);
          start = 0;
        }
        else
          string[i] = tolower(string[i]);
      }
    }
  }
}

char *msStringChop(char *string)
{
  int n;

  n = strlen(string);
  if(n>0)
    string[n-1] = '\0';

  return(string);
}

/*
** Trim leading and trailing white space.
*/
void msStringTrim(char *str)
{
  int i;

  /* Send nulls home without supper. */
  if( ! str ) return;

  /* Move non-white string to the front. */
  i = strspn(str, " ");
  if(i) {
    memmove(str, str + i, strlen(str) - i + 1);
  }
  /* Nothing left? Exit. */
  if(strlen(str) == 0) {
    return;
  }
  /* Null-terminate end of non-white string. */
  for(i=strlen(str)-1; i>=0; i--) { /* step backwards from end */
    if(str[i] != ' ') {
      str[i+1] = '\0';
      return;
    }
  }
  return;
}

/*
** Remove leading white spaces and shift everything to the left.
*/
char *msStringTrimLeft(char *string)
{
  char *read, *write;
  int i, length;

  if (string && strlen(string) > 0) {
    length = strlen(string);
    read = string;
    write = string;

    for (i=0; i<length; i++) {
      if (isspace(string[i]))
        read++;
      else
        break;
    }

    if (read > write) {
      while (*read) {
        *write = *read;
        read++;
        write++;
      }
      *write = '\0';
    }
  }
  return string;
}

/* ------------------------------------------------------------------------------- */
/*       Trims trailing blanks from a string                                        */
/* ------------------------------------------------------------------------------- */
void msStringTrimBlanks(char *string)
{
  int i,n;

  n = strlen(string);
  for(i=n-1; i>=0; i--) { /* step backwards through the string */
    if(string[i] != ' ') {
      string[i+1] = '\0';
      return;
    }
  }
}

/* ------------------------------------------------------------------------------- */
/*       Trims end-of-line marker from a string                                    */
/*       Usefull in conjunction with fgets() calls                                 */
/* ------------------------------------------------------------------------------- */
void msStringTrimEOL(char *string)
{
  int i;

  for(i=0 ; string[i] != '\0'; i++) {
    if(string[i] == '\n') {
      string[i] = '\0'; /* Terminate the string at the newline */
      return;
    }
  }
}

/* ------------------------------------------------------------------------------- */
/*       Replace all occurances of old with new in str.                            */
/*       It is assumed that str was dynamically created using malloc.              */
/* ------------------------------------------------------------------------------- */
char *msReplaceSubstring(char *str, const char *old, const char *new)
{
  size_t str_len, old_len, new_len, tmp_offset;
  char *tmp_ptr;

  if(new == NULL)
    new = "";

  /*
  ** If old is not found then leave str alone
  */
  if( (tmp_ptr = strstr(str, old)) == NULL)
    return(str);

  /*
  ** Grab some info about incoming strings
  */
  str_len = strlen(str);
  old_len = strlen(old);
  new_len = strlen(new);

  /*
  ** Now loop until old is NOT found in new
  */
  while( tmp_ptr != NULL ) {

    /*
    ** re-allocate memory for buf assuming 1 replacement of old with new
          ** don't bother reallocating if old is larger than new)
    */
    if (old_len < new_len) {
      tmp_offset = tmp_ptr - str;
      str_len = str_len - old_len + new_len;
      str = (char *)msSmallRealloc(str, (str_len + 1)); /* make new space for a copy */
      tmp_ptr = str + tmp_offset;
    }

    /*
    ** Move the trailing part of str to make some room unless old_len == new_len
    */
    if (old_len != new_len) {
      memmove(tmp_ptr+new_len, tmp_ptr+old_len, strlen(tmp_ptr)-old_len+1);
    }

    /*
    ** Now copy new over old
    */
    memcpy(tmp_ptr, new, new_len);

    /*
    ** And look for more matches in the rest of the string
    */
    tmp_ptr = strstr(tmp_ptr + new_len, old);
  }

  return(str);
}

/*
 * same goal as msReplaceSubstring, but for the known case
 * when we won't have to do reallocs etc
 * used to replace the wrap characetr by a newline for labels
 */
void msReplaceChar(char *str, char old, char new)
{
  while(*(str++))
    if(*str==old)
      *str=new;
}

/*
** how many times does ch occur in str
*/
int msCountChars(char *str, char ch)
{
  int i, l, n=0;

  l = strlen(str);
  for(i=0; i<l; i++)
    if(str[i] == ch) n++;

  return(n);
}

/* ------------------------------------------------------------------------------- */
/*       Strip filename from a full path                                           */
/* ------------------------------------------------------------------------------- */
char *msStripPath(char *fn)
{
  char *pSlash;
  char *pBackslash;

  /* try to locate both, the last slash or backslash */
  pSlash = strrchr(fn,'/');
  pBackslash = strrchr(fn,'\\');

  if( pSlash != NULL && pBackslash != NULL ) {
    if( pSlash < pBackslash )
      return ++pBackslash;
    else
      return ++pSlash;
  } else if ( pSlash != NULL )
    return ++pSlash; /* skip past the "slash" */
  else if ( pBackslash != NULL )
    return ++pBackslash; /* skip past the "backslash" */
  else
    return(fn);
}

/*
** Returns the *path* portion of the filename fn. Memory is allocated using malloc.
*/
char *msGetPath(const char *fn)
{
  char *str;
  int i, length;

  length = strlen(fn);
  if((str = msStrdup(fn)) == NULL)
    return(NULL);

  for(i=length-1; i>=0; i--) { /* step backwards through the string */
    if((str[i] == '/') || (str[i] == '\\')) {
      str[i+1] = '\0';
      break;
    }
  }

  if(strcmp(str, fn) == 0) {
    msFree(str);
#if defined(_WIN32) && !defined(__CYGWIN__)
    str = msStrdup(".\\");
#else
    str= msStrdup("./");
#endif
  }

  return(str);
}

/*
** Returns a *path* built from abs_path and path.
** The pszReturnPath must be declared by the caller function as an array
** of MS_MAXPATHLEN char
*/
char *msBuildPath(char *pszReturnPath, const char *abs_path, const char *path)
{
  int   abslen = 0;
  int   pathlen = 0;


  if(path == NULL) {
    msSetError(MS_IOERR, NULL, "msBuildPath");
    return NULL;
  }

  pathlen = strlen(path);
  if (abs_path)
    abslen = strlen(abs_path);

  if((pathlen + abslen + 2) > MS_MAXPATHLEN) {
    msSetError(MS_IOERR, "Path is too long.  Check server logs.",
               "msBuildPath()");
    msDebug("msBuildPath(): (%s%s): path is too long.\n", abs_path, path);
    return NULL;
  }

  /* Check if path is absolute */
  if((abs_path == NULL) || (abslen == 0) ||
      (path[0] == '\\') || (path[0] == '/') ||
      (pathlen > 1 && (path[1] == ':'))) {
    strlcpy(pszReturnPath, path, MS_MAXPATHLEN);
    return(pszReturnPath);
  }

  /* else return abs_path/path */
  if((abs_path[abslen-1] == '/') || (abs_path[abslen-1] == '\\')) {
    snprintf(pszReturnPath, MS_MAXPATHLEN, "%s%s", abs_path, path);
  } else {
    snprintf(pszReturnPath, MS_MAXPATHLEN, "%s/%s", abs_path, path);
  }

  return(pszReturnPath);
}

/*
** Returns a *path* built from abs_path, path1 and path2.
** abs_path/path1/path2
** The pszReturnPath must be declared by the caller function as an array
** of MS_MAXPATHLEN char
*/
char *msBuildPath3(char *pszReturnPath, const char *abs_path, const char *path1,const char *path2)
{
  char szPath[MS_MAXPATHLEN];

  return msBuildPath(pszReturnPath, abs_path,
                     msBuildPath(szPath, path1, path2));
}

/*
** Similar to msBuildPath(), but the input path is only qualified by the
** absolute path if this will result in it pointing to a readable file.
**
** Returns NULL if the resulting path doesn't point to a readable file.
*/

char *msTryBuildPath(char *szReturnPath, const char *abs_path, const char *path)

{
  VSILFILE  *fp;

  if( msBuildPath( szReturnPath, abs_path, path ) == NULL )
    return NULL;

  fp = VSIFOpenL( szReturnPath, "r" );
  if( fp == NULL ) {
    strlcpy( szReturnPath, path, MS_MAXPATHLEN);
    return NULL;
  } else
    VSIFCloseL( fp );

  return szReturnPath;
}

/*
** Similar to msBuildPath3(), but the input path is only qualified by the
** absolute path if this will result in it pointing to a readable file.
**
** Returns NULL if the resulting path doesn't point to a readable file.
*/

char *msTryBuildPath3(char *szReturnPath, const char *abs_path, const char *path1, const char *path2)

{
  VSILFILE  *fp;

  if( msBuildPath3( szReturnPath, abs_path, path1, path2 ) == NULL )
    return NULL;

  fp = VSIFOpenL( szReturnPath, "r" );
  if( fp == NULL ) {
    strlcpy( szReturnPath, path2, MS_MAXPATHLEN);
    return NULL;
  } else
    VSIFCloseL( fp );

  return szReturnPath;
}

/*
** Splits a string into multiple strings based on ch. Consecutive ch's are ignored.
*/
char **msStringSplit(const char *string, char ch, int *num_tokens)
{
  int i,j,k;
  int length,n;
  char **token;
  char last_ch='\0';

  n = 1; /* always at least 1 token, the string itself */
  length = strlen(string);
  for(i=0; i<length; i++) {
    if(string[i] == ch && last_ch != ch)
      n++;
    last_ch = string[i];
  }

  token = (char **) msSmallMalloc(sizeof(char *)*n);

  k = 0;
  token[k] = (char *)msSmallMalloc(sizeof(char)*(length+1));

  j = 0;
  last_ch='\0';
  for(i=0; i<length; i++) {
    if(string[i] == ch) {

      if(last_ch == ch)
        continue;

      token[k][j] = '\0'; /* terminate current token */

      k++;
      token[k] = (char *)msSmallMalloc(sizeof(char)*(length+1));

      j = 0;
    } else {
      token[k][j] = string[i];
      j++;
    }

    last_ch = string[i];
  }

  token[k][j] = '\0'; /* terminate last token */

  *num_tokens = n;

  return(token);
}

/*
 This function is a copy of CSLTokenizeString2() function of the CPL component.
 See the port/cpl_string.cpp file in gdal source for the complete documentation.
 Available Flags:
 * - MS_ALLOWEMPTYTOKENS: allow the return of empty tokens when two
 * delimiters in a row occur with no other text between them.  If not set,
 * empty tokens will be discarded;
 * - MS_STRIPLEADSPACES: strip leading space characters from the token (as
 * reported by isspace());
 * - MS_STRIPENDSPACES: strip ending space characters from the token (as
 * reported by isspace());
 * - MS_HONOURSTRINGS: double quotes can be used to hold values that should
 * not be broken into multiple tokens;
 * - MS_PRESERVEQUOTES: string quotes are carried into the tokens when this
 * is set, otherwise they are removed;
 * - MS_PRESERVEESCAPES: if set backslash escapes (for backslash itself,
 * and for literal double quotes) will be preserved in the tokens, otherwise
 * the backslashes will be removed in processing.
 */
char ** msStringSplitComplex( const char * pszString,
                              const char * pszDelimiters,
                              int *num_tokens,
                              int nFlags )

{
  char        **papszRetList = NULL;
  int         nRetMax = 0, nRetLen = 0;
  char        *pszToken;
  int         nTokenMax, nTokenLen;
  int         bHonourStrings = (nFlags & MS_HONOURSTRINGS);
  int         bAllowEmptyTokens = (nFlags & MS_ALLOWEMPTYTOKENS);
  int         bStripLeadSpaces = (nFlags & MS_STRIPLEADSPACES);
  int         bStripEndSpaces = (nFlags & MS_STRIPENDSPACES);

  pszToken = (char *) msSmallMalloc(sizeof(char)*10);;
  nTokenMax = 10;

  while( pszString != NULL && *pszString != '\0' ) {
    int     bInString = MS_FALSE;
    int     bStartString = MS_TRUE;

    nTokenLen = 0;

    /* Try to find the next delimeter, marking end of token */
    for( ; *pszString != '\0'; pszString++ ) {

      /* End if this is a delimeter skip it and break. */
      if( !bInString && strchr(pszDelimiters, *pszString) != NULL ) {
        pszString++;
        break;
      }

      /* If this is a quote, and we are honouring constant
         strings, then process the constant strings, with out delim
         but don't copy over the quotes */
      if( bHonourStrings && *pszString == '"' ) {
        if( nFlags & MS_PRESERVEQUOTES ) {
          pszToken[nTokenLen] = *pszString;
          nTokenLen++;
        }

        if( bInString ) {
          bInString = MS_FALSE;
          continue;
        } else {
          bInString = MS_TRUE;
          continue;
        }
      }

      /*
       * Within string constants we allow for escaped quotes, but in
       * processing them we will unescape the quotes and \\ sequence
       * reduces to \
       */
      if( bInString && pszString[0] == '\\' ) {
        if ( pszString[1] == '"' || pszString[1] == '\\' ) {
          if( nFlags & MS_PRESERVEESCAPES ) {
            pszToken[nTokenLen] = *pszString;
            nTokenLen++;
          }

          pszString++;
        }
      }

      /*
       * Strip spaces at the token start if requested.
       */
      if ( !bInString && bStripLeadSpaces
           && bStartString && isspace((unsigned char)*pszString) )
        continue;

      bStartString = MS_FALSE;

      /*
       * Extend token buffer if we are running close to its end.
       */
      if( nTokenLen >= nTokenMax-3 ) {
        nTokenMax = nTokenMax * 2 + 10;
        pszToken = (char *) msSmallRealloc(pszToken, sizeof(char)*nTokenMax);
      }

      pszToken[nTokenLen] = *pszString;
      nTokenLen++;
    }

    /*
     * Strip spaces at the token end if requested.
     */
    if ( !bInString && bStripEndSpaces ) {
      while ( nTokenLen && isspace((unsigned char)pszToken[nTokenLen - 1]) )
        nTokenLen--;
    }

    pszToken[nTokenLen] = '\0';

    /*
     * Add the token.
     */
    if( pszToken[0] != '\0' || bAllowEmptyTokens ) {
      if( nRetLen >= nRetMax - 1 ) {
        nRetMax = nRetMax * 2 + 10;
        papszRetList = (char **) msSmallRealloc(papszRetList, sizeof(char*)*nRetMax);
      }

      papszRetList[nRetLen++] = msStrdup( pszToken );
      papszRetList[nRetLen] = NULL;
    }
  }

  /*
   * If the last token was empty, then we need to capture
   * it now, as the loop would skip it.
   */
  if( *pszString == '\0' && bAllowEmptyTokens && nRetLen > 0
      && strchr(pszDelimiters,*(pszString-1)) != NULL ) {
    if( nRetLen >= nRetMax - 1 ) {
      nRetMax = nRetMax * 2 + 10;
      papszRetList = (char **) msSmallRealloc(papszRetList, sizeof(char*)*nRetMax);
    }

    papszRetList[nRetLen++] = msStrdup("");
    papszRetList[nRetLen] = NULL;
  }

  if( papszRetList == NULL )
    papszRetList = (char **) msSmallMalloc(sizeof(char *)*1);

  *num_tokens = nRetLen;
  free(pszToken);

  return papszRetList;
}

/* This method is similar to msStringSplit but support quoted strings.
   It also support multi-characters delimiter and allows to preserve quotes */
char **msStringTokenize( const char *pszLine, const char *pszDelim,
                         int *num_tokens, int preserve_quote )
{
  char **papszResult = NULL;
  int n = 1, iChar, nLength = strlen(pszLine), iTokenChar = 0, bInQuotes = MS_FALSE;
  char *pszToken = (char *) msSmallMalloc(sizeof(char)*(nLength+1));
  int nDelimLen = strlen(pszDelim);

  /* Compute the number of tokens */
  for( iChar = 0; pszLine[iChar] != '\0'; iChar++ ) {
    if( bInQuotes && pszLine[iChar] == '"' && pszLine[iChar+1] == '"' ) {
      iChar++;
    } else if( pszLine[iChar] == '"' ) {
      bInQuotes = !bInQuotes;
    } else if ( !bInQuotes && strncmp(pszLine+iChar,pszDelim,nDelimLen) == 0 ) {
      iChar += nDelimLen - 1;
      n++;
    }
  }

  papszResult = (char **) msSmallMalloc(sizeof(char *)*n);
  n = iTokenChar = bInQuotes = 0;
  for( iChar = 0; pszLine[iChar] != '\0'; iChar++ ) {
    if( bInQuotes && pszLine[iChar] == '"' && pszLine[iChar+1] == '"' ) {
      if (preserve_quote == MS_TRUE)
        pszToken[iTokenChar++] = '"';
      pszToken[iTokenChar++] = '"';
      iChar++;
    } else if( pszLine[iChar] == '"' ) {
      if (preserve_quote == MS_TRUE)
        pszToken[iTokenChar++] = '"';
      bInQuotes = !bInQuotes;
    } else if( !bInQuotes && strncmp(pszLine+iChar,pszDelim,nDelimLen) == 0 ) {
      pszToken[iTokenChar++] = '\0';
      papszResult[n] = pszToken;
      pszToken = (char *) msSmallMalloc(sizeof(char)*(nLength+1));
      iChar += nDelimLen - 1;
      iTokenChar = 0;
      n++;
    } else {
      pszToken[iTokenChar++] = pszLine[iChar];
    }
  }

  pszToken[iTokenChar++] = '\0';
  papszResult[n] = pszToken;

  *num_tokens = n+1;

  return papszResult;
}

/**********************************************************************
 *                       msEncodeChar()
 *
 * Return 1 if the character argument should be encoded for safety
 * in URL use and 0 otherwise. Specific character map taken from
 * http://www.ietf.org/rfc/rfc2396.txt
 *
 **********************************************************************/

int msEncodeChar(const char c)
{
  if (
    (c >= 0x61 && c <= 0x7A ) ||   /* Letters a-z */
    (c >= 0x41 && c <= 0x5A ) ||   /* Letters A-Z */
    (c >= 0x30 && c <= 0x39 ) ||   /* Numbers 0-9 */
    (c >= 0x27 && c <= 0x2A ) ||   /* * ' ( )     */
    (c >= 0x2D && c <= 0x2E ) ||   /* - .         */
    (c == 0x5F ) ||                /* _           */
    (c == 0x21 ) ||                /* !           */
    (c == 0x7E ) ) {               /* ~           */
    return(0);
  } else {
    return(1);
  }
}

char *msEncodeUrl(const char *data)
{
  /*
   * Delegate to msEncodeUrlExcept, with a null second argument
   * to render the except handling moot.
   */
  return(msEncodeUrlExcept(data, '\0'));
}

/**********************************************************************
 *                       msEncodeCharExcept()
 *
 * URL encoding, applies RFP2396 encoding to all characters
 * except the one exception character. An exception character
 * of '\0' implies no exception handling.
 *
 **********************************************************************/

char *msEncodeUrlExcept(const char *data, const char except)
{
  static const char *hex = "0123456789ABCDEF";
  const char *i;
  char  *j, *code;
  int   inc;
  unsigned char ch;

  for (inc=0, i=data; *i!='\0'; i++)
    if (msEncodeChar(*i))
      inc += 2;

  code = (char*)msSmallMalloc(strlen(data)+inc+1);

  for (j=code, i=data; *i!='\0'; i++, j++) {
    if ( except != '\0' && *i == except ) {
      *j = except;
    } else if (msEncodeChar(*i)) {
      ch = *i;
      *j++ = '%';
      *j++ = hex[ch/16];
      *j   = hex[ch%16];
    } else
      *j = *i;
  }
  *j = '\0';

  return code;
}

/************************************************************************/
/*                            msEscapeJSonString()                      */
/************************************************************************/

/* The input (and output) string are not supposed to start/end with double */
/* quote characters. It is the responsibility of the caller to do that. */
char* msEscapeJSonString(const char* pszJSonString)
{
    /* Worst case is one character to become \uABCD so 6 characters */
    char* pszRet;
    int i = 0, j = 0;
    static const char* pszHex = "0123456789ABCDEF";
    
    pszRet = (char*) msSmallMalloc(strlen(pszJSonString) * 6 + 1);
    /* From http://www.json.org/ */
    for(i = 0; pszJSonString[i] != '\0'; i++)
    {
        unsigned char ch = pszJSonString[i];
        if( ch == '\b' )
        {
            pszRet[j++] = '\\';
            pszRet[j++] = 'b';
        }
        else if( ch == '\f' )
        {
            pszRet[j++] = '\\';
            pszRet[j++] = 'f';
        }
        else if( ch == '\n' )
        {
            pszRet[j++] = '\\';
            pszRet[j++] = 'n';
        }
        else if( ch == '\r' )
        {
            pszRet[j++] = '\\';
            pszRet[j++] = 'r';
        }
        else if( ch == '\t' )
        {
            pszRet[j++] = '\\';
            pszRet[j++] = 't';
        }
        else if( ch < 32 )
        {
            pszRet[j++] = '\\';
            pszRet[j++] = 'u';
            pszRet[j++] = '0';
            pszRet[j++] = '0';
            pszRet[j++] = pszHex[ch / 16];
            pszRet[j++] = pszHex[ch % 16];
        }
        else if( ch == '"' )
        {
            pszRet[j++] = '\\';
            pszRet[j++] = '"';
        }
        else if( ch == '\\' )
        {
            pszRet[j++] = '\\';
            pszRet[j++] = '\\';
        }
        else
        {
            pszRet[j++] = ch;
        }
    }
    pszRet[j] = '\0';
    return pszRet;
}

/* msEncodeHTMLEntities()
**
** Return a copy of string after replacing some problematic chars with their
** HTML entity equivalents.
**
** The replacements performed are:
**  '&' -> "&amp;", '"' -> "&quot;", '<' -> "&lt;" and '>' -> "&gt;"
**/
char *msEncodeHTMLEntities(const char *string)
{
  int buflen, i;
  char *newstring;
  const char *c;

  if(string == NULL)
    return NULL;

  /* Start with 100 extra chars for replacements...  */
  /* should be good enough for most cases */
  buflen = strlen(string) + 100;
  newstring = (char*)malloc(buflen+1);
  MS_CHECK_ALLOC(newstring, buflen+1, NULL);

  for(i=0, c=string; *c != '\0'; c++) {
    /* Need to realloc buffer? */
    if (i+6 > buflen) {
      /* If we had to realloc then this string must contain several */
      /* entities... so let's go with twice the previous buffer size */
      buflen *= 2;
      newstring = (char*)realloc(newstring, buflen+1);
      MS_CHECK_ALLOC(newstring, buflen+1, NULL);
    }

    switch(*c) {
      case '&':
        strcpy(newstring+i, "&amp;");
        i += 5;
        break;
      case '<':
        strcpy(newstring+i, "&lt;");
        i += 4;
        break;
      case '>':
        strcpy(newstring+i, "&gt;");
        i += 4;
        break;
      case '"':
        strcpy(newstring+i, "&quot;");
        i += 6;
        break;
      case '\'':
        strcpy(newstring+i, "&#39;"); /* changed from &apos; and i += 6 (bug 1040) */
        i += 5;
        break;
      default:
        newstring[i++] = *c;
    }
  }

  newstring[i++] = '\0';

  return newstring;
}


/* msDecodeHTMLEntities()
**
** Modify the string to replace encoded characters by their true value
**
** The replacements performed are:
**  "&amp;" -> '&', "&quot;" -> '"', "&lt;" -> '<' and "&gt;" -> '>'
**/
void msDecodeHTMLEntities(const char *string)
{
  char *pszAmp=NULL, *pszSemiColon=NULL, *pszReplace=NULL, *pszEnd=NULL;
  char *pszBuffer=NULL;
  size_t bufferSize = 0;

  if(string == NULL)
    return;
  else
    pszBuffer = (char*)string;

  bufferSize = strlen(pszBuffer);
  pszReplace = (char*) msSmallMalloc(bufferSize+1);
  pszEnd = (char*) msSmallMalloc(bufferSize+1);

  while((pszAmp = strchr(pszBuffer, '&')) != NULL) {
    /* Get the &...; */
    strlcpy(pszReplace, pszAmp, bufferSize);
    pszSemiColon = strchr(pszReplace, ';');
    if(pszSemiColon == NULL)
      break;
    else
      pszSemiColon++;

    /* Get everything after the &...; */
    strlcpy(pszEnd, pszSemiColon, bufferSize);

    pszReplace[pszSemiColon-pszReplace] = '\0';

    /* Replace the &...; */
    if(strcasecmp(pszReplace, "&amp;") == 0) {
      pszBuffer[pszAmp - pszBuffer] = '&';
      pszBuffer[pszAmp - pszBuffer + 1] = '\0';
      strcat(pszBuffer, pszEnd);
    } else if(strcasecmp(pszReplace, "&lt;") == 0) {
      pszBuffer[pszAmp - pszBuffer] = '<';
      pszBuffer[pszAmp - pszBuffer + 1] = '\0';
      strcat(pszBuffer, pszEnd);
    } else if(strcasecmp(pszReplace, "&gt;") == 0) {
      pszBuffer[pszAmp - pszBuffer] = '>';
      pszBuffer[pszAmp - pszBuffer + 1] = '\0';
      strcat(pszBuffer, pszEnd);
    } else if(strcasecmp(pszReplace, "&quot;") == 0) {
      pszBuffer[pszAmp - pszBuffer] = '"';
      pszBuffer[pszAmp - pszBuffer + 1] = '\0';
      strcat(pszBuffer, pszEnd);
    } else if(strcasecmp(pszReplace, "&apos;") == 0) {
      pszBuffer[pszAmp - pszBuffer] = '\'';
      pszBuffer[pszAmp - pszBuffer + 1] = '\0';
      strcat(pszBuffer, pszEnd);
    }

    pszBuffer = pszAmp + 1;
  }

  free(pszReplace);
  free(pszEnd);

  return;
}

/*
** msIsXMLValid
**
** Check if the string is an XML valid string. It should contains only
** A-Z, a-z, 0-9, '_', '-', '.', and ':'
** Return MS_TRUE or MS_FALSE
*/
int msIsXMLTagValid(const char *string)
{
  int i, nLen;

  nLen = strlen(string);

  for(i=0; i<nLen; i++) {
    if( !( string[i] >= 'A' && string[i] <= 'Z' ) &&
        !( string[i] >= 'a' && string[i] <= 'z' ) &&
        !( string[i] >= '0' && string[i] <= '9' ) &&
        string[i] != '-' && string[i] != '.' &&
        string[i] != ':' && string[i] != '_' )
      return MS_FALSE;
  }

  return MS_TRUE;
}


/*
 * Concatenate pszSrc to pszDest and reallocate memory if necessary.
*/
char *msStringConcatenate(char *pszDest, const char *pszSrc)
{
  int nLen;

  if (pszSrc == NULL)
    return pszDest;

  /* if destination is null, allocate memory */
  if (pszDest == NULL) {
    pszDest = msStrdup(pszSrc);
  } else { /* if dest is not null, reallocate memory */
    char *pszTemp;

    nLen = strlen(pszDest) + strlen(pszSrc);

    pszTemp = (char*)realloc(pszDest, nLen + 1);
    if (pszTemp) {
      pszDest = pszTemp;
      strcat(pszDest, pszSrc);
      pszDest[nLen] = '\0';
    } else {
      msSetError(MS_MEMERR, "Error while reallocating memory.", "msStringConcatenate()");
      return NULL;
    }
  }

  return pszDest;
}

char *msJoinStrings(char **array, int arrayLength, const char *delimeter)
{
  char *string;
  int stringLength=0;
  int delimeterLength;
  int i;

  if(!array || arrayLength <= 0 || !delimeter) return NULL;

  delimeterLength = strlen(delimeter);

  for(i=0; i<arrayLength; i++)
    stringLength += strlen(array[i]) + delimeterLength;

  string = (char *)calloc(stringLength+1, sizeof(char));
  MS_CHECK_ALLOC(string, (stringLength+1)* sizeof(char), NULL);
  string[0] = '\0';

  for(i=0; i<arrayLength-1; i++) {
    strlcat(string, array[i], stringLength);
    strlcat(string, delimeter, stringLength);
  }
  strlcat(string, array[i], stringLength); /* add last element, no delimiter */

  return string;
}

#define HASH_SIZE  16
/*
 * Return a hashed string for a given input string.
 * The caller should free the return value.
*/
char *msHashString(const char *pszStr)
{
  unsigned char sums[HASH_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  char *pszOutBuf = NULL;
  size_t bufferSize = 0;
  int i=0;

  bufferSize = HASH_SIZE*2+1;
  pszOutBuf = (char*)msSmallMalloc(bufferSize);

  for(i=0; pszStr && pszStr[i]; i++) {
    sums[i%HASH_SIZE] += (unsigned char)(pszStr[i]);
  }

  for(i=0; i<HASH_SIZE; i++) {
    snprintf(pszOutBuf + i*2, bufferSize-(i*2), "%02x", sums[i]);
  }

  return pszOutBuf;
}

char *msCommifyString(char *str)
{
  int i, j, old_length, new_length;
  int num_commas=0, num_decimal_points=0;
  int add_commas;

  char comma=',', decimal_point='.';

  if(!str) return NULL;

  num_decimal_points = msCountChars(str, decimal_point);
  if(num_decimal_points > 1) return str;

  old_length = strlen(str);
  if(num_decimal_points == 0) {
    num_commas =  floor((old_length - 1)/3);
    add_commas=1; /* add commas right away */
  } else {
    num_commas = floor(((old_length - strlen(strchr(str, decimal_point))) - 1)/3);
    add_commas=0; /* wait until after the decimal point */
  }

  if(num_commas < 1) return str; /* nothing to add */

  new_length = old_length + num_commas;
  str = (char *) msSmallRealloc(str, new_length+1);
  str[new_length] = '\0';

  j = 0;
  for(i=new_length-1; i>=0; i--) { /* step backwards through the string */

    if(num_decimal_points == 1 &&  add_commas == 0) { /* to the right of the decimal point, no commas */
      str[i] = str[i-num_commas];
      if(str[i] == decimal_point) add_commas = 1;
    } else if(add_commas == 1 && j>2) { /* need a comma */
      str[i] = comma;
      num_commas--; /* need one fewer now */
      j = 0; /* reset */
    } else {
      str[i] = str[i-num_commas]; /* shift to the right */
      j++;
    }

    if(num_commas == 0) break; /* done, rest of string is ok "as is" */
  }

  return str;
}


/* ------------------------------------------------------------------------------- */
/*       Replace all occurrences of old with new in str.                           */
/*       It is assumed that str was dynamically created using malloc.              */
/*       Same function as msReplaceSubstring but this is case insensitive          */
/* ------------------------------------------------------------------------------- */
char *msCaseReplaceSubstring(char *str, const char *old, const char *new)
{
  size_t str_len, old_len, new_len, tmp_offset;
  char *tmp_ptr;

  /*
  ** If old is not found then leave str alone
  */
  if( (tmp_ptr = (char *) strcasestr(str, old)) == NULL)
    return(str);
  
  if(new == NULL)
    new = "";


  /*
  ** Grab some info about incoming strings
  */
  str_len = strlen(str);
  old_len = strlen(old);
  new_len = strlen(new);

  /*
  ** Now loop until old is NOT found in new
  */
  while( tmp_ptr != NULL ) {

    /*
    ** re-allocate memory for buf assuming 1 replacement of old with new
          ** don't bother reallocating if old is larger than new)
    */
    if (old_len < new_len) {
      tmp_offset = tmp_ptr - str;
      str_len = str_len - old_len + new_len;
      str = (char *)msSmallRealloc(str, (str_len + 1)); /* make new space for a copy */
      tmp_ptr = str + tmp_offset;
    }

    /*
    ** Move the trailing part of str to make some room unless old_len == new_len
    */
    if (old_len != new_len) {
      memmove(tmp_ptr+new_len, tmp_ptr+old_len, strlen(tmp_ptr)-old_len+1);
    }

    /*
    ** Now copy new over old
    */
    memcpy(tmp_ptr, new, new_len);

    /*
    ** And look for more matches in the rest of the string
    */
    tmp_ptr = (char *) strcasestr(tmp_ptr + new_len, old);
  }

  return(str);
}

/*
** Converts a 2 character hexidecimal string to an integer.
*/
int msHexToInt(char *hex)
{
  int number;

  number = (hex[0] >= 'A' ? ((hex[0] & 0xdf) - 'A')+10 : (hex[0] - '0'));
  number *= 16;
  number += (hex[1] >= 'A' ? ((hex[1] & 0xdf) - 'A')+10 : (hex[1] - '0'));

  return(number);
}


/*
** Use FRIBIDI to encode the string.
** The return value must be freed by the caller.
*/
#ifdef USE_FRIBIDI
char *msGetFriBidiEncodedString(const char *string, const char *encoding)
{
  FriBidiChar logical[MAX_STR_LEN];
  FriBidiParType base;
  size_t len;

#ifdef FRIBIDI_NO_CHARSETS
  iconv_t to_ucs4, from_ucs4;
#else
  int to_char_set_num;
  int from_char_set_num;
#endif

  len = strlen(string);

#ifdef FRIBIDI_NO_CHARSETS
  to_ucs4 = iconv_open ("WCHAR_T", encoding);
  from_ucs4 = iconv_open ("UTF-8", "WCHAR_T");
#else
  to_char_set_num = fribidi_parse_charset ((char*)encoding);
  from_char_set_num = fribidi_parse_charset ("UTF-8");
#endif

#ifdef FRIBIDI_NO_CHARSETS
  if (to_ucs4 == (iconv_t) (-1) || from_ucs4 == (iconv_t) (-1))
#else
  if (!to_char_set_num || !from_char_set_num)
#endif
  {
    msSetError(MS_IDENTERR, "Encoding not supported (%s).",
               "msGetFriBidiEncodedString()", encoding);
    return NULL;
  }

#ifdef FRIBIDI_NO_CHARSETS
  {
    char *st = string, *ust = (char *) logical;
    int in_len = (int) len;
    len = sizeof logical;
    iconv (to_ucs4, &st, &in_len, &ust, (int *) &len);
    len = (FriBidiChar *) ust - logical;
  }
#else
  len = fribidi_charset_to_unicode (to_char_set_num, (char*)string, len, logical);
#endif

  {
    FriBidiChar *visual;
    char outstring[MAX_STR_LEN];
    FriBidiStrIndex *ltov, *vtol;
    FriBidiLevel *levels;
    FriBidiStrIndex new_len;
    fribidi_boolean log2vis;
    int i, j;

    visual = (FriBidiChar *) msSmallMalloc (sizeof (FriBidiChar) * (len + 1));
    ltov = NULL;
    vtol = NULL;
    levels = NULL;

    /* Create a bidi string. */
    log2vis = fribidi_log2vis (logical, len, &base,
                               /* output */
                               visual, ltov, vtol, levels);

    if (!log2vis) {
      msSetError(MS_IDENTERR, "Failed to create bidi string.",
                 "msGetFriBidiEncodedString()");
      return NULL;
    }

    new_len = len;

    /* Convert it to utf-8 for display. */
#ifdef FRIBIDI_NO_CHARSETS
    {
      char *str = outstring, *ust = (char *) visual;
      int in_len = len * sizeof visual[0];
      new_len = sizeof outstring;
      iconv (from_ucs4, &ust, &in_len, &str, (int *) &new_len);
      *str = '\0';
      new_len = str - outstring;
    }
#else
    new_len =
      fribidi_unicode_to_charset (from_char_set_num,
                                  visual, len, outstring);

    /* scan str and compress out FRIBIDI_CHAR_FILL UTF8 characters */

    for (i=0, j=0; i<new_len; i++, j++) {
      if (outstring[i] == '\xef' && outstring[i+1] == '\xbb' && outstring[i+2] == '\xbf') {
        i += 3;
      }
      if (i != j) {
        outstring[j] = outstring[i];
      }
    }
    outstring[j] = '\0';

#endif

    free(visual);
    return msStrdup(outstring);
  }
}
#endif

/*
** Simple charset converter. Converts string from specified encoding to UTF-8.
** The return value must be freed by the caller.
*/
char *msGetEncodedString(const char *string, const char *encoding)
{
#ifdef USE_ICONV
  iconv_t cd = NULL;
  const char *inp;
  char *outp, *out = NULL;
  size_t len, bufsize, bufleft, iconv_status;
  assert(encoding);

#ifdef USE_FRIBIDI
  msAcquireLock(TLOCK_FRIBIDI);
  if(fribidi_parse_charset ((char*)encoding)) {
    char *ret = msGetFriBidiEncodedString(string, encoding);
    msReleaseLock(TLOCK_FRIBIDI);
    return ret;
  }
  msReleaseLock(TLOCK_FRIBIDI);
#endif
  len = strlen(string);

  if (len == 0 || strcasecmp(encoding, "UTF-8")==0)
    return msStrdup(string);    /* Nothing to do: string already in UTF-8 */

  cd = iconv_open("UTF-8", encoding);
  if(cd == (iconv_t)-1) {
    msSetError(MS_IDENTERR, "Encoding not supported by libiconv (%s).",
               "msGetEncodedString()", encoding);
    return NULL;
  }

  bufsize = len * 6 + 1; /* Each UTF-8 char can be up to 6 bytes */
  inp = string;
  out = (char*) malloc(bufsize);
  if(out == NULL) {
    msSetError(MS_MEMERR, NULL, "msGetEncodedString()");
    iconv_close(cd);
    return NULL;
  }
  strlcpy(out, string, bufsize);
  outp = out;

  bufleft = bufsize;
  iconv_status = -1;

  while (len > 0) {
    iconv_status = iconv(cd, (char**)&inp, &len, &outp, &bufleft);
    if(iconv_status == -1) {
      msFree(out);
      iconv_close(cd);
      return msStrdup(string);
    }
  }
  out[bufsize - bufleft] = '\0';

  iconv_close(cd);

  return out;
#else
  if (*string == '\0' || (encoding && strcasecmp(encoding, "UTF-8")==0))
    return msStrdup(string);    /* Nothing to do: string already in UTF-8 */

  msSetError(MS_MISCERR, "Not implemeted since Iconv is not enabled.", "msGetEncodedString()");
  return NULL;
#endif
}


char* msConvertWideStringToUTF8 (const wchar_t* string, const char* encoding)
{
#ifdef USE_ICONV

  char* output = NULL;
  char* errormessage = NULL;
  iconv_t cd = NULL;
  size_t nStr;
  size_t nInSize;
  size_t nOutSize;
  size_t iconv_status = -1;
  size_t nBufferSize;

  char* pszUTF8 = NULL;
  const wchar_t* pwszWide = NULL;

  if (string != NULL) {
    nStr = wcslen (string);
    nBufferSize = ((nStr * 6) + 1);
    output = (char*) msSmallMalloc (nBufferSize);

    if (nStr == 0) {
      /* return an empty 8 byte string */
      output[0] = '\0';
      return output;
    }

    cd = iconv_open("UTF-8", encoding);

    nOutSize = nBufferSize;
    if ((iconv_t)-1 != cd) {
      nInSize = sizeof (wchar_t)*nStr;
      pszUTF8 = output;
      pwszWide = string;
      iconv_status = iconv(cd, (char **)&pwszWide, &nInSize, &pszUTF8, &nOutSize);
      if ((size_t)-1 == iconv_status) {
        switch (errno) {
          case E2BIG:
            errormessage = "There is not sufficient room in buffer";
            break;
          case EILSEQ:
            errormessage = "An invalid multibyte sequence has been encountered in the input";
            break;
          case EINVAL:
            errormessage = "An incomplete multibyte sequence has been encountered in the input";
            break;
          default:
            errormessage = "Unknown";
            break;
        }
        msSetError(MS_MISCERR, "Unable to convert string in encoding '%s' to UTF8 %s",
                   "msConvertWideStringToUTF8()",
                   encoding,errormessage);
        iconv_close(cd);
        msFree(output);
        return NULL;
      }
      iconv_close(cd);
    } else {
      msSetError(MS_MISCERR, "Encoding not supported by libiconv (%s).",
                 "msConvertWideStringToUTF8()",
                 encoding);
      msFree(output);
      return NULL;
    }

  } else {
    /* we were given a NULL wide string, nothing we can do here */
    return NULL;
  }

  /* NULL-terminate the output string */
  output[nBufferSize - nOutSize] = '\0';
  return output;
#else
  msSetError(MS_MISCERR, "Not implemented since Iconv is not enabled.", "msConvertWideStringToUTF8()");
  return NULL;
#endif
}

/*
** Returns the next glyph in string and advances *in_ptr to the next
** character.
**
** If out_string is not NULL then the character (bytes) is copied to this
** buffer and null-terminated. out_string must be a pre-allocated buffer of
** at least 11 bytes.
**
** The function returns the number of bytes in this glyph.
**
** This function treats 3 types of glyph encodings:
*   - as an html entity, for example &#123; , &#x1af; , or &eacute;
*   - as an utf8 encoded character
*   - if utf8 decoding fails, as a raw character
*
** This function mimics the character decoding function used in gdft.c of
* libGD. It is necessary to have the same behaviour, as input strings must be
* split into the same glyphs as what gd does.
**
** In UTF-8, the number of leading 1 bits in the first byte specifies the
** number of bytes in the entire sequence.
** Source: http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
**
** U-00000000 U-0000007F: 0xxxxxxx
** U-00000080 U-000007FF: 110xxxxx 10xxxxxx
** U-00000800 U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
** U-00010000 U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
** U-00200000 U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
** U-04000000 U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
int msGetNextGlyph(const char **in_ptr, char *out_string)
{
  unsigned char in;
  int numbytes=0;
  unsigned int unicode;
  int i;

  in = (unsigned char)**in_ptr;

  if (in == 0)
    return -1;  /* Empty string */
  if((numbytes=msGetUnicodeEntity(*in_ptr,&unicode))>0) {
    if(out_string) {
      for(i=0; i<numbytes; i++) {
        out_string[i]=(*in_ptr)[i];
      }
      out_string[numbytes]='\0';
    }
    *in_ptr+=numbytes;
    return numbytes;
  }
  if (in < 0xC0) {
    /*
    * Handles properly formed UTF-8 characters between
    * 0x01 and 0x7F.  Also treats \0 and naked trail
    * bytes 0x80 to 0xBF as valid characters representing
    * themselves.
    */
    /*goto end of loop to return just the char*/
  } else if (in < 0xE0) {
    if (((*in_ptr)[1]& 0xC0) == 0x80) {
      if(out_string) {
        out_string[0]=in;
        out_string[1]=(*in_ptr)[1];
        out_string[2]='\0';
      }
      *in_ptr+=2;
      return 2; /*110xxxxx 10xxxxxx*/
    }
  } else if (in < 0xF0) {
    if (((*in_ptr)[1]& 0xC0) == 0x80 && ((*in_ptr)[2]& 0xC0) == 0x80) {
      if(out_string) {
        out_string[0]=in;
        *in_ptr+=numbytes;
        out_string[1]=(*in_ptr)[1];
        out_string[2]=(*in_ptr)[2];
        out_string[3]='\0';
      }
      *in_ptr+=3;
      return 3;   /* 1110xxxx 10xxxxxx 10xxxxxx */
    }
  } else if (in < 0xF8) {
    if (((*in_ptr)[1]& 0xC0) == 0x80 && ((*in_ptr)[2]& 0xC0) == 0x80
        && ((*in_ptr)[3]& 0xC0) == 0x80) {
      if(out_string) {
        out_string[0]=in;
        out_string[1]=(*in_ptr)[1];
        out_string[2]=(*in_ptr)[2];
        out_string[3]=(*in_ptr)[3];
        out_string[4]='\0';
      }
      *in_ptr+=4;
      return 4;   /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    }
  } else if (in < 0xFC) {
    if (((*in_ptr)[1]& 0xC0) == 0x80 && ((*in_ptr)[2]& 0xC0) == 0x80
        && ((*in_ptr)[3]& 0xC0) == 0x80 && ((*in_ptr)[4]& 0xC0) == 0x80) {
      if(out_string) {
        out_string[0]=in;
        out_string[1]=(*in_ptr)[1];
        out_string[2]=(*in_ptr)[2];
        out_string[3]=(*in_ptr)[3];
        out_string[4]=(*in_ptr)[4];
        out_string[5]='\0';
      }
      *in_ptr+=5;
      return 5;   /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
    }
  } else if (in < 0xFE) {
    if (((*in_ptr)[1]& 0xC0) == 0x80 && ((*in_ptr)[2]& 0xC0) == 0x80
        && ((*in_ptr)[3]& 0xC0) == 0x80 && ((*in_ptr)[4]& 0xC0) == 0x80
        && ((*in_ptr)[5]& 0xC0) == 0x80) {
      if(out_string) {
        out_string[0]=in;
        out_string[1]=(*in_ptr)[1];
        out_string[2]=(*in_ptr)[2];
        out_string[3]=(*in_ptr)[3];
        out_string[4]=(*in_ptr)[4];
        out_string[5]=(*in_ptr)[5];
        out_string[6]='\0';
      }
      *in_ptr+=6;
      return 6;   /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
    }
  }

  if (out_string) {
    out_string[0]=in;
    out_string[1] = '\0';   /* 0xxxxxxx */
  }
  (*in_ptr)++;
  return 1;
}

/*
** Returns the number of glyphs in string
*/
int msGetNumGlyphs(const char *in_ptr)
{
  int numchars=0;

  while( msGetNextGlyph(&in_ptr, NULL) != -1 )
    numchars++;

  return numchars;
}

static int cmp_entities(const void *e1, const void *e2)
{
  struct mapentities_s *en1 = (struct mapentities_s *) e1;
  struct mapentities_s *en2 = (struct mapentities_s *) e2;
  return strcmp(en1->name, en2->name);
}
/*
 * this function tests if the string pointed by inptr represents
 * an HTML entity, in decimal form ( e.g. &#197;), in hexadecimal
 * form ( e.g. &#x6C34; ), or from html 4.0 spec ( e.g. &eacute; )
 * - returns returns 0 if the string doesn't represent such an entity.
 * - if the string does start with such entity,it returns the number of
 * bytes occupied by said entity, and stores the unicode value in *unicode
 */
int msGetUnicodeEntity(const char *inptr, unsigned int *unicode)
{
  unsigned char *in = (unsigned char*)inptr;
  int l,val=0;
  if(*in=='&') {
    in++;
    if(*in=='#') {
      in++;
      if(*in=='x'||*in=='X') {
        in++;
        for(l=3; l<8; l++) {
          char byte;
          if(*in>='0'&&*in<='9')
            byte = *in - '0';
          else if(*in>='a'&&*in<='f')
            byte = *in - 'a' + 10;
          else if(*in>='A'&&*in<='F')
            byte = *in - 'A' + 10;
          else
            break;
          in++;
          val = (val * 16) + byte;
        }
        if(*in==';' && l>3 ) {
          *unicode=val;
          return ++l;
        }
      } else {
        for(l=2; l<8; l++) {
          if(*in>='0'&&*in<='9') {
            val = val*10+*in-'0';
            in++;
          } else
            break;
        }
        if(*in==';' && l>2 ) {
          *unicode=val;
          return ++l;
        }
      }
    } else {
      char entity_name_buf[MAP_ENTITY_NAME_LENGTH_MAX+1];
      char *p;
      struct mapentities_s key, *res;
      key.name = p = entity_name_buf;
      for (l = 1; l <=  MAP_ENTITY_NAME_LENGTH_MAX+1; l++) {
        if (*in == '\0') /*end of string before possible entity: return*/
          break;
        if (*in == ';') { /*possible end of entity: do a lookup*/
          *p++ = '\0';
          res = bsearch(&key, mapentities, MAP_NR_OF_ENTITIES,
                        sizeof(mapentities[0]), *cmp_entities);
          if (res) {
            *unicode = res->value;
            return ++l;
          }
          break; /*the string was of the form of an entity but didn't correspond to an existing one: return*/
        }
        *p++ = *in;
        in++;
      }
    }
  }
  return 0;
}

/**
 * msStringIsInteger()
 *
 * determines whether a given string is an integer
 *
 * @param string the string to be tested
 *
 * @return MS_SUCCESS or MS_FAILURE
 */

int msStringIsInteger(const char *string)
{
  int length, i;

  length = strlen(string);

  if (length == 0)
    return MS_FAILURE;

  for(i=0; i<length; i++) {
    if (!isdigit(string[i]))
      return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                             msStrdup()                               */
/************************************************************************/

/* Safe version of msStrdup(). This function is taken from gdal/cpl. */

char *msStrdup(const char * pszString)
{
    size_t nStringLength; 
    char *pszReturn;

    if (pszString == NULL)
        pszString = "";

    nStringLength = strlen(pszString) + 1; /* null terminated byte */
    pszReturn = malloc(nStringLength);

    if (pszReturn == NULL) {
        fprintf(stderr, "msSmallMalloc(): Out of memory allocating %ld bytes.\n",
            (long)strlen(pszString));
        exit(1);
    }

    memcpy(pszReturn, pszString, nStringLength);

    return pszReturn;
}


/************************************************************************/
/*                             msStringEscape()                         */
/************************************************************************/

/* Checks if a string contains single or double quotes and escape them.
   NOTE: the user must free the returned char* if it is different than the
   one passed in */

char* msStringEscape( const char * pszString )
{
  char *string_tmp, *string_ptr;
  int i,ncharstoescape=0;

  if (pszString ==  NULL || strlen(pszString) == 0)
    return msStrdup("");

  for (i=0; pszString[i]; i++)
    ncharstoescape += ((pszString[i] == '\"')||(pszString[i] == '\''));
  
  if(!ncharstoescape) {
    return (char*)pszString;
  }

  string_tmp = (char*)msSmallMalloc(strlen(pszString)+ncharstoescape+1);
  for (string_ptr=(char*)pszString,i=0; *string_ptr!='\0'; ++string_ptr,++i) {
    if ( (*string_ptr == '\"') || (*string_ptr == '\'') ) {
      string_tmp[i] = '\\';
      ++i;
    }
    string_tmp[i] = *string_ptr;
  }

  string_tmp[i] = '\0';
  return string_tmp;
}

/************************************************************************/
/*                             msStringInArray()                        */
/************************************************************************/

/* Check if a string is in a array */
int msStringInArray( const char * pszString, char **array, int numelements)
{
  int i;
  for (i=0; i<numelements; ++i) {
    if (strcasecmp(pszString, array[i])==0)
      return MS_TRUE;
  }
  return MS_FALSE;
}

int msLayerEncodeShapeAttributes( layerObj *layer, shapeObj *shape) {

#ifdef USE_ICONV
  iconv_t cd = NULL;
  const char *inp;
  char *outp, *out = NULL;
  size_t len, bufsize, bufleft;
  int i;

  if( !layer->encoding || !*layer->encoding || !strcasecmp(layer->encoding, "UTF-8"))
    return MS_SUCCESS;

  cd = iconv_open("UTF-8", layer->encoding);
  if(cd == (iconv_t)-1) {
    msSetError(MS_IDENTERR, "Encoding not supported by libiconv (%s).",
               "msGetEncodedString()", layer->encoding);
    return MS_FAILURE;
  }

  for(i=0;i <shape->numvalues; i++) {
    int failedIconv = FALSE;
    if(!shape->values[i] || (len = strlen(shape->values[i]))==0) {
      continue;    /* Nothing to do */
    }

    bufsize = len * 6 + 1; /* Each UTF-8 char can be up to 6 bytes */
    inp = shape->values[i];
    out = (char*) msSmallMalloc(bufsize);

    strlcpy(out, shape->values[i], bufsize);
    outp = out;

    bufleft = bufsize;

    while (len > 0) {
      const size_t iconv_status = iconv(cd, (char**)&inp, &len, &outp, &bufleft);
      if(iconv_status == (size_t)(-1)) {
        failedIconv = TRUE;
        break;
      }
    }
    if( failedIconv ) {
      msFree(out);
      continue; /* silently ignore failed conversions */
    }
    out[bufsize - bufleft] = '\0';
    msFree(shape->values[i]);
    shape->values[i] = out;
  }
  iconv_close(cd);

  return MS_SUCCESS;
#else
  if( !layer->encoding || !*layer->encoding || !strcasecmp(layer->encoding, "UTF-8"))
    return MS_SUCCESS;
  msSetError(MS_MISCERR, "Not implemented since Iconv is not enabled.", "msGetEncodedString()");
  return MS_FAILURE;
#endif
}

/************************************************************************/
/*                             msStringBuffer                           */
/************************************************************************/

struct msStringBuffer
{
    size_t alloc_size;
    size_t length;
    char  *str;
};

/************************************************************************/
/*                         msStringBufferAlloc()                        */
/************************************************************************/

msStringBuffer* msStringBufferAlloc(void)
{
    return (msStringBuffer*)msSmallCalloc(sizeof(msStringBuffer), 1);
}

/************************************************************************/
/*                         msStringBufferFree()                         */
/************************************************************************/

void msStringBufferFree(msStringBuffer* sb)
{
    if( sb )
        msFree(sb->str);
    msFree(sb);
}

/************************************************************************/
/*                       msStringBufferGetString()                      */
/************************************************************************/

const char* msStringBufferGetString(msStringBuffer* sb)
{
    return sb->str;
}

/************************************************************************/
/*                   msStringBufferReleaseStringAndFree()               */
/************************************************************************/

char* msStringBufferReleaseStringAndFree(msStringBuffer* sb)
{
    char* str = sb->str;
    sb->str = NULL;
    sb->alloc_size = 0;
    sb->length = 0;
    msStringBufferFree(sb);
    return str;
}

/************************************************************************/
/*                        msStringBufferAppend()                        */
/************************************************************************/

int msStringBufferAppend(msStringBuffer* sb, const char* pszAppendedString)
{
    size_t nAppendLen = strlen(pszAppendedString);
    if( sb->length + nAppendLen >= sb->alloc_size )
    {
        size_t newAllocSize1 = sb->alloc_size + sb->alloc_size / 3;
        size_t newAllocSize2 = sb->length + nAppendLen + 1;
        size_t newAllocSize = MAX(newAllocSize1, newAllocSize2);
        void* newStr = realloc(sb->str, newAllocSize);
        if( newStr == NULL ) {
            msSetError(MS_MEMERR, "Not enough memory", "msStringBufferAppend()");
            return MS_FAILURE;
        }
        sb->alloc_size = newAllocSize;
        sb->str = (char*) newStr;
    }
    memcpy(sb->str + sb->length, pszAppendedString, nAppendLen + 1);
    sb->length += nAppendLen;
    return MS_SUCCESS;
}
