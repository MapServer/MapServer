#include "map.h"

#ifdef NEED_STRDUP
char	*strdup(char *s)
{
  char	*s1;

  if(!s)
    return(NULL);
  s1 = (char *)malloc(strlen(s) + 1);
  if(!s1)
    return(NULL);

  strcpy(s1,s);
  return(s1);
}
#endif

#ifdef NEED_STRNCASECMP
int strncasecmp(char *s1, char *s2, int len)
{
  register char *cp1, *cp2;

  cp1 = s1;
  cp2 = s2;
  while(*cp1 && *cp2 && len) {
    if(toupper(*cp1) != toupper(*cp2))
      return(-1);
    cp1++;
    cp2++;
    len--;
  }
  if(len == 0) {
    return(0);
  }
  if(*cp1 || *cp2)
    return(-1);
  return(0);
}
#endif

#ifdef NEED_STRCASECMP
int strcasecmp(char *s1, char *s2)
{
  register char *cp1, *cp2;

  cp1 = s1;
  cp2 = s2;
  while(*cp1 && *cp2) {
    if(toupper(*cp1) != toupper(*cp2))
      return(-1);
    cp1++;
    cp2++;
  }
  if(*cp1 || *cp2)
    return(-1);
  return(0);
}
#endif

char *chop(char *string) {  
  int n;

  n = strlen(string);
  if(n>0)
    string[n-1] = '\0';

  return(string);
}

/* ------------------------------------------------------------------------------- */
/*       Trims leading blanks from a string                                        */
/* ------------------------------------------------------------------------------- */
void trimBlanks(char *string)
{
   int i,n;

   n = strlen(string);
   for(i=n-1;i>=0;i--) { /* step backwards through the string */
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
void trimEOL(char *string)
{
  int i;

  for(i=0;i<strlen(string);i++) {
    if(string[i] == '\n') {
      string[i] = '\0'; /* Terminate the string at the newline */
      return;
    }
  }
}

/* ------------------------------------------------------------------------------- */
/*       Replace all occurances of old with new in str.                            */
/*       If new contains old then NO replacement is done.                          */
/*       It is assumed that str was dynamically created using malloc.              */
/* ------------------------------------------------------------------------------- */
char *gsub(char *str, const char *old, const char *new)
{
      size_t str_len, old_len, new_len, tmp_len;
      char *tmp;
      int i,j,k;

      if(new == NULL)
	return(str);

      /*
      ** If old is not found then leave str alone
      */
      if(strstr(str, old) == NULL)
	return(str);

      /*
      ** To avoid an infinite loop, make sure new doesn't contain old
      ** and if it does just leave str alone
      */
      if(strstr(new, old) != NULL)
	return(str);

      /*
      ** Grab some info about incoming strings
      */
      old_len = strlen(old);
      new_len = strlen(new);

      /*
      ** Now loop until old is NOT found in new
      */
      while((tmp = strstr(str, old)) != NULL) {

	/*
	** Save length of tmp, tells us where to do the replacement
        */
	tmp_len = strlen(tmp);

	/*
	** re-allocate memory for buf assuming 1 replacement of old with new
	*/
	str_len = strlen(str);
	
	tmp = strdup(str);
	str = (char *)realloc(str, (str_len - old_len + new_len + 1)); /* make new space for a copy */

	j=0;
	k=0;
	for(i=0; i<(str_len - tmp_len); i++, j++, k++)
	  str[j] = tmp[k];
	
	k+=old_len; /* skip past the old string */

	for(i=0; i<new_len; i++, j++)
	  str[j] = new[i];

	for(i=0; i<(tmp_len-old_len); i++, j++, k++)
	  str[j] = tmp[k];

	str[j] = '\0'; /* terminate the string */

	free(tmp);
      }

      return(str);
}

/*
** how many times does ch occur in str
*/
int countChars(char *str, char ch) 
{
  int i, l, n=0;

  l = strlen(str);
  for(i=0;i<l;i++)
    if(str[i] == ch) n++;

  return(n);
}

/* ------------------------------------------------------------------------------- */
/*       Strip filename from a full path                                           */
/* ------------------------------------------------------------------------------- */
char *stripPath(char *fn)
{
  char *str;

  if((str = strrchr(fn,'/')) != NULL) { /* return pointer to last "slash" */
    str++; /* skip past the "slash" */
    return(str);
  } else
    return(fn);
}

/*
** Returns the *path* portion of the filename fn. Memory is allocated using malloc.
*/
char *getPath(char *fn)
{
  char *str;
  int i, length;
  
  length = strlen(fn);
  if((str = strdup(fn)) == NULL)
    return(NULL);
  
  for(i=length-1; i>=0; i--) { /* step backwards through the string */
    if((str[i] == '/') || (str[i] == '\\')) { 
      str[i+1] = '\0'; 
      break;
    }
  }

#if defined(_WIN32) && !defined(__CYGWIN__)  
  if(strcmp(str, fn) == 0)
    strcpy(str, ".\\");
#else
  if(strcmp(str, fn) == 0)
    strcpy(str, "./");
#endif  

  return(str);
}

/*
** Splits a string into multiple strings based on ch. Consecutive ch's are ignored.
*/
char **split(const char *string, char ch, int *num_tokens) 
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

  token = (char **) malloc(sizeof(char *)*n);
  if(!token) return(NULL);
  
  k = 0;
  token[k] = (char *)malloc(sizeof(char)*(length+1));
  if(!token[k]) return(NULL);

  j = 0;
  last_ch='\0';
  for(i=0; i<length; i++) {
    if(string[i] == ch) {
      
      if(last_ch == ch)
	continue;
      
      token[k][j] = '\0'; /* terminate current token */      
      
      k++;
      token[k] = (char *)malloc(sizeof(char)*(length+1));
      if(!token[k]) return(NULL);
      
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
