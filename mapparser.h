#ifndef BISON_MAPPARSER_H
# define BISON_MAPPARSER_H

#ifndef YYSTYPE
typedef union {
  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	NUMBER	257
# define	STRING	258
# define	ISTRING	259
# define	TIME	260
# define	REGEX	261
# define	IREGEX	262
# define	OR	263
# define	AND	264
# define	NOT	265
# define	RE	266
# define	EQ	267
# define	NE	268
# define	LT	269
# define	GT	270
# define	LE	271
# define	GE	272
# define	IN	273
# define	IEQ	274
# define	LENGTH	275
# define	NEG	276


extern YYSTYPE msyylval;

#endif /* not BISON_MAPPARSER_H */
