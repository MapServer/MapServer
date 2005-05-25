#ifndef MAP_REGEX_H
#define MAP_REGEX_H

#ifdef __cplusplus
extern "C" {
#endif
  
  /* We want these to match the POSIX standard, so we need these*/
  /* === regex2.h === */
#ifdef WIN32
#define API_EXPORT(type)    __declspec(dllexport) type __stdcall
#else
#define API_EXPORT(type)    type
#endif
  
  typedef struct {
    void* sys_regex;
  } ms_regex_t;
  
  typedef int ms_regoff_t;
  typedef struct
  {
    ms_regoff_t rm_so;  /* Byte offset from string's start to substring's start.  */
    ms_regoff_t rm_eo;  /* Byte offset from string's start to substring's end.  */
  } ms_regmatch_t;
  
  API_EXPORT(int) ms_regcomp(ms_regex_t *, const char *, int);
  API_EXPORT(size_t) ms_regerror(int, const ms_regex_t *, char *, size_t);
  API_EXPORT(int) ms_regexec(const ms_regex_t *, const char *, size_t, ms_regmatch_t [], int);
  API_EXPORT(void) ms_regfree(ms_regex_t *);

#ifndef BUILDING_REGEX_PROXY

#define regcomp ms_regcomp
#define regerror ms_regerror
#define regexec ms_regexec
#define regfree ms_regfree

#define regex_t ms_regex_t
#define regmatch_t ms_regmatch_t
#define regoff_t ms_regoff_t

  /* === regcomp.c === */
#define	REG_BASIC	0000
#define	REG_EXTENDED	0001
#define	REG_ICASE	0002
#define	REG_NOSUB	0004
#define	REG_NEWLINE	0010
#define	REG_NOSPEC	0020
#define	REG_PEND	0040
#define	REG_DUMP	0200


/* === regerror.c === */
#define	REG_OKAY	 0
#define	REG_NOMATCH	 1
#define	REG_BADPAT	 2
#define	REG_ECOLLATE	 3
#define	REG_ECTYPE	 4
#define	REG_EESCAPE	 5
#define	REG_ESUBREG	 6
#define	REG_EBRACK	 7
#define	REG_EPAREN	 8
#define	REG_EBRACE	 9
#define	REG_BADBR	10
#define	REG_ERANGE	11
#define	REG_ESPACE	12
#define	REG_BADRPT	13
#define	REG_EMPTY	14
#define	REG_ASSERT	15
#define	REG_INVARG	16
#define	REG_ATOI	255	/* convert name to number (!) */
#define	REG_ITOA	0400	/* convert number to name (!) */


/* === regexec.c === */
#define	REG_NOTBOL	00001
#define	REG_NOTEOL	00002
#define	REG_STARTEND	00004
#define	REG_TRACE	00400	/* tracing of execution */
#define	REG_LARGE	01000	/* force large representation */
#define	REG_BACKR	02000	/* force use of backref code */

#endif


#ifdef __cplusplus
}
#endif

#endif
