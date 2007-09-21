/* A Bison parser, made from mapparser.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse msyyparse
#define yylex msyylex
#define yyerror msyyerror
#define yylval msyylval
#define yychar msyychar
#define yydebug msyydebug
#define yynerrs msyynerrs
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

#line 5 "mapparser.y"

/* C declarations */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "mapserver.h" /* for TRUE/FALSE and REGEX includes */
#include "maptime.h" /* for time comparison routines */

extern int msyylex(void); /* lexer globals */
extern int msyyerror(const char *);

int msyyresult;

#line 24 "mapparser.y"
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
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		109
#define	YYFLAG		-32768
#define	YYNTBASE	31

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 276 ? yytranslate[x] : 37)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    26,     2,     2,
      29,    30,    24,    22,     2,    23,     2,    25,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    28,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    27
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     3,     5,     7,    11,    15,    19,    23,
      27,    31,    35,    39,    42,    45,    49,    53,    57,    61,
      65,    69,    73,    77,    81,    85,    89,    93,    97,   101,
     105,   109,   113,   117,   121,   125,   129,   133,   137,   141,
     145,   147,   151,   155,   159,   163,   167,   170,   174,   178,
     183,   185,   189,   193,   195
};
static const short yyrhs[] =
{
      -1,    33,     0,    34,     0,     7,     0,    33,     9,    33,
       0,    33,    10,    33,     0,    33,     9,    34,     0,    33,
      10,    34,     0,    34,     9,    33,     0,    34,    10,    33,
       0,    34,     9,    34,     0,    34,    10,    34,     0,    11,
      33,     0,    11,    34,     0,    35,    12,    32,     0,    34,
      13,    34,     0,    34,    14,    34,     0,    34,    16,    34,
       0,    34,    15,    34,     0,    34,    18,    34,     0,    34,
      17,    34,     0,    29,    33,    30,     0,    35,    13,    35,
       0,    35,    14,    35,     0,    35,    16,    35,     0,    35,
      15,    35,     0,    35,    18,    35,     0,    35,    17,    35,
       0,    36,    13,    36,     0,    36,    14,    36,     0,    36,
      16,    36,     0,    36,    15,    36,     0,    36,    18,    36,
       0,    36,    17,    36,     0,    35,    19,    35,     0,    34,
      19,    35,     0,    34,    20,    34,     0,    35,    20,    35,
       0,    36,    20,    36,     0,     3,     0,    34,    22,    34,
       0,    34,    23,    34,     0,    34,    24,    34,     0,    34,
      26,    34,     0,    34,    25,    34,     0,    23,    34,     0,
      34,    28,    34,     0,    29,    34,    30,     0,    21,    29,
      35,    30,     0,     4,     0,    29,    35,    30,     0,    35,
      22,    35,     0,     6,     0,    29,    36,    30,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,    57,    58,    61,    69,    71,    80,    89,    97,   106,
     114,   123,   131,   140,   141,   142,   155,   161,   167,   173,
     179,   185,   191,   192,   200,   208,   217,   225,   233,   241,
     247,   253,   259,   265,   271,   277,   298,   319,   325,   333,
     341,   342,   343,   344,   345,   346,   353,   354,   355,   356,
     359,   360,   361,   364,   365
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "NUMBER", "STRING", "ISTRING", "TIME", 
  "REGEX", "IREGEX", "OR", "AND", "NOT", "RE", "EQ", "NE", "LT", "GT", 
  "LE", "GE", "IN", "IEQ", "LENGTH", "'+'", "'-'", "'*'", "'/'", "'%'", 
  "NEG", "'^'", "'('", "')'", "input", "regular_exp", "logical_exp", 
  "math_exp", "string_exp", "time_exp", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    31,    31,    31,    32,    33,    33,    33,    33,    33,
      33,    33,    33,    33,    33,    33,    33,    33,    33,    33,
      33,    33,    33,    33,    33,    33,    33,    33,    33,    33,
      33,    33,    33,    33,    33,    33,    33,    33,    33,    33,
      34,    34,    34,    34,    34,    34,    34,    34,    34,    34,
      35,    35,    35,    36,    36
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     0,     1,     1,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       1,     3,     3,     3,     3,     3,     2,     3,     3,     4,
       1,     3,     3,     1,     3
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       1,    40,    50,    53,     0,     0,     0,     0,     2,     3,
       0,     0,    13,    14,     0,     0,    46,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    22,
      48,    51,    54,     5,     7,     6,     8,     9,    11,    10,
      12,    16,    17,    19,    18,    21,    20,    36,    37,    41,
      42,    43,    45,    44,    47,     4,    15,    23,    24,    26,
      25,    28,    27,    35,    38,    52,     0,    29,    30,    32,
      31,    34,    33,    39,     0,    49,     0,     0,     0,     0
};

static const short yydefgoto[] =
{
     107,    86,     8,     9,    10,    11
};

static const short yypact[] =
{
      -2,-32768,-32768,-32768,    -2,   -24,     3,    -2,    49,   121,
     192,   202,-32768,   175,    -1,     3,   -20,     4,    99,   155,
     166,    -2,    -2,    -2,    -2,     3,     3,     3,     3,     3,
       3,    -1,     3,     3,     3,     3,     3,     3,     3,     8,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,
       1,     1,     1,     1,     1,     1,    -1,   -12,    44,-32768,
  -32768,-32768,-32768,     7,   138,-32768,   175,     7,   138,-32768,
     175,    13,    13,    13,    13,    13,    13,   -11,    13,    57,
      57,   -20,   -20,   -20,   -20,-32768,-32768,   -11,   -11,   -11,
     -11,   -11,   -11,   -11,   -11,-32768,     1,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,   -10,-32768,    -8,    25,    29,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,    56,    69,     9,    -7
};


#define	YYLAST		222


static const short yytable[] =
{
      20,     1,     2,     2,     3,    14,     1,     3,    38,     4,
      48,    48,    48,    21,    22,    85,    19,    22,   105,     5,
      61,     6,    62,    57,     5,   108,     6,     7,    56,   109,
      96,     0,    15,     0,    59,    33,    34,    35,    36,    37,
      77,    38,    97,    98,    99,   100,   101,   102,   103,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    21,    22,
      12,     0,     0,    17,     0,   104,    33,    34,    35,    36,
      37,     0,    38,    13,    60,    16,    18,    63,    65,    67,
      69,    35,    36,    37,    58,    38,     0,     0,     0,   106,
      64,    66,    68,    70,    71,    72,    73,    74,    75,    76,
       0,    78,    79,    80,    81,    82,    83,    84,    23,    24,
       0,     0,    25,    26,    27,    28,    29,    30,    31,    32,
       0,    33,    34,    35,    36,    37,     0,    38,     0,    60,
      23,    24,     0,     0,    25,    26,    27,    28,    29,    30,
      31,    32,     0,    33,    34,    35,    36,    37,    24,    38,
       0,    25,    26,    27,    28,    29,    30,    31,    32,     0,
      33,    34,    35,    36,    37,     0,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,     0,    48,     0,    49,
      50,    51,    52,    53,    54,    61,    55,     0,    25,    26,
      27,    28,    29,    30,    31,    32,    62,    33,    34,    35,
      36,    37,     0,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,     0,    48,    49,    50,    51,    52,    53,
      54,     0,    55
};

static const short yycheck[] =
{
       7,     3,     4,     4,     6,    29,     3,     6,    28,    11,
      22,    22,    22,     9,    10,     7,     7,    10,    30,    21,
      30,    23,    30,    14,    21,     0,    23,    29,    29,     0,
      29,    -1,    29,    -1,    30,    22,    23,    24,    25,    26,
      31,    28,    49,    50,    51,    52,    53,    54,    55,    40,
      41,    42,    43,    44,    45,    46,    47,    48,     9,    10,
       4,    -1,    -1,     7,    -1,    56,    22,    23,    24,    25,
      26,    -1,    28,     4,    30,     6,     7,    21,    22,    23,
      24,    24,    25,    26,    15,    28,    -1,    -1,    -1,    96,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      -1,    32,    33,    34,    35,    36,    37,    38,     9,    10,
      -1,    -1,    13,    14,    15,    16,    17,    18,    19,    20,
      -1,    22,    23,    24,    25,    26,    -1,    28,    -1,    30,
       9,    10,    -1,    -1,    13,    14,    15,    16,    17,    18,
      19,    20,    -1,    22,    23,    24,    25,    26,    10,    28,
      -1,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      22,    23,    24,    25,    26,    -1,    28,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    -1,    22,    -1,    13,
      14,    15,    16,    17,    18,    30,    20,    -1,    13,    14,
      15,    16,    17,    18,    19,    20,    30,    22,    23,    24,
      25,    26,    -1,    28,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    22,    13,    14,    15,    16,    17,
      18,    -1,    20
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/sw/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif
#endif

#line 315 "/sw/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 2:
#line 58 "mapparser.y"
{ 
			    msyyresult = yyvsp[0].intval; 
			  }
    break;
case 3:
#line 61 "mapparser.y"
{
			    if(yyvsp[0].dblval != 0)
			      msyyresult = MS_TRUE;
			    else
			      msyyresult = MS_FALSE;			    
			  }
    break;
case 5:
#line 72 "mapparser.y"
{
	                                 if(yyvsp[-2].intval == MS_TRUE)
		                           yyval.intval = MS_TRUE;
		                         else if(yyvsp[0].intval == MS_TRUE)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 6:
#line 80 "mapparser.y"
{
	                                 if(yyvsp[-2].intval == MS_TRUE) {
			                   if(yyvsp[0].intval == MS_TRUE)
			                     yyval.intval = MS_TRUE;
			                   else
			                     yyval.intval = MS_FALSE;
			                 } else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 7:
#line 89 "mapparser.y"
{
	                                 if(yyvsp[-2].intval == MS_TRUE)
		                           yyval.intval = MS_TRUE;
		                         else if(yyvsp[0].dblval != 0)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 8:
#line 97 "mapparser.y"
{
	                                 if(yyvsp[-2].intval == MS_TRUE) {
			                   if(yyvsp[0].dblval != 0)
			                     yyval.intval = MS_TRUE;
			                   else
			                     yyval.intval = MS_FALSE;
			                 } else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 9:
#line 106 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval != 0)
		                           yyval.intval = MS_TRUE;
		                         else if(yyvsp[0].intval == MS_TRUE)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 10:
#line 114 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval != 0) {
			                   if(yyvsp[0].intval == MS_TRUE)
			                     yyval.intval = MS_TRUE;
			                   else
			                     yyval.intval = MS_FALSE;
			                 } else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 11:
#line 123 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval != 0)
		                           yyval.intval = MS_TRUE;
		                         else if(yyvsp[0].dblval != 0)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 12:
#line 131 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval != 0) {
			                   if(yyvsp[0].dblval != 0)
			                     yyval.intval = MS_TRUE;
			                   else
			                     yyval.intval = MS_FALSE;
			                 } else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 13:
#line 140 "mapparser.y"
{ yyval.intval = !yyvsp[0].intval; }
    break;
case 14:
#line 141 "mapparser.y"
{ yyval.intval = !yyvsp[0].dblval; }
    break;
case 15:
#line 142 "mapparser.y"
{
                                         ms_regex_t re;

                                         if(ms_regcomp(&re, yyvsp[0].strval, MS_REG_EXTENDED|MS_REG_NOSUB) != 0) 
                                           yyval.intval = MS_FALSE;

                                         if(ms_regexec(&re, yyvsp[-2].strval, 0, NULL, 0) == 0)
                                  	   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;

                                         ms_regfree(&re);
                                       }
    break;
case 16:
#line 155 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval == yyvsp[0].dblval)
	 		                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 17:
#line 161 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval != yyvsp[0].dblval)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 18:
#line 167 "mapparser.y"
{	                                 
	                                 if(yyvsp[-2].dblval > yyvsp[0].dblval)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 19:
#line 173 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval < yyvsp[0].dblval)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 20:
#line 179 "mapparser.y"
{	                                 
	                                 if(yyvsp[-2].dblval >= yyvsp[0].dblval)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 21:
#line 185 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval <= yyvsp[0].dblval)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 22:
#line 191 "mapparser.y"
{ yyval.intval = yyvsp[-1].intval; }
    break;
case 23:
#line 192 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) == 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
				       }
    break;
case 24:
#line 200 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) != 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
				       }
    break;
case 25:
#line 208 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) > 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   /* printf("Not freeing: %s >= %s\n",$1, $3); */
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
                                       }
    break;
case 26:
#line 217 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) < 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
                                       }
    break;
case 27:
#line 225 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) >= 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
                                       }
    break;
case 28:
#line 233 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) <= 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
                                       }
    break;
case 29:
#line 241 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) == 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
				   }
    break;
case 30:
#line 247 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) != 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
				   }
    break;
case 31:
#line 253 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) > 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
                                   }
    break;
case 32:
#line 259 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) < 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
                                   }
    break;
case 33:
#line 265 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) >= 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
                                   }
    break;
case 34:
#line 271 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) <= 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
                                   }
    break;
case 35:
#line 277 "mapparser.y"
{
					 char *delim,*bufferp;

					 yyval.intval = MS_FALSE;
					 bufferp=yyvsp[0].strval;

					 while((delim=strchr(bufferp,',')) != NULL) {
					   *delim='\0';
					   if(strcmp(yyvsp[-2].strval,bufferp) == 0) {
					     yyval.intval = MS_TRUE;
					     break;
					   } 
					   *delim=',';
					   bufferp=delim+1;
					 }

					 if(strcmp(yyvsp[-2].strval,bufferp) == 0) // is this test necessary?
					   yyval.intval = MS_TRUE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
				       }
    break;
case 36:
#line 298 "mapparser.y"
{
					 char *delim,*bufferp;

					 yyval.intval = MS_FALSE;
					 bufferp=yyvsp[0].strval;

					 while((delim=strchr(bufferp,',')) != NULL) {
					   *delim='\0';
					   if(yyvsp[-2].dblval == atof(bufferp)) {
					     yyval.intval = MS_TRUE;
					     break;
					   } 
					   *delim=',';
					   bufferp=delim+1;
					 }

					 if(yyvsp[-2].dblval == atof(bufferp)) // is this test necessary?
					   yyval.intval = MS_TRUE;
					   
					   free(yyvsp[0].strval);
				       }
    break;
case 37:
#line 319 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval == yyvsp[0].dblval)
	 		                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
    break;
case 38:
#line 325 "mapparser.y"
{
                                         if(strcasecmp(yyvsp[-2].strval, yyvsp[0].strval) == 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
				       }
    break;
case 39:
#line 333 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) == 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
				   }
    break;
case 41:
#line 342 "mapparser.y"
{ yyval.dblval = yyvsp[-2].dblval + yyvsp[0].dblval; }
    break;
case 42:
#line 343 "mapparser.y"
{ yyval.dblval = yyvsp[-2].dblval - yyvsp[0].dblval; }
    break;
case 43:
#line 344 "mapparser.y"
{ yyval.dblval = yyvsp[-2].dblval * yyvsp[0].dblval; }
    break;
case 44:
#line 345 "mapparser.y"
{ yyval.dblval = (int)yyvsp[-2].dblval % (int)yyvsp[0].dblval; }
    break;
case 45:
#line 346 "mapparser.y"
{
	         if(yyvsp[0].dblval == 0.0) {
				     msyyerror("division by zero");
				     return(-1);
				   } else
	                             yyval.dblval = yyvsp[-2].dblval / yyvsp[0].dblval; 
				 }
    break;
case 46:
#line 353 "mapparser.y"
{ yyval.dblval = yyvsp[0].dblval; }
    break;
case 47:
#line 354 "mapparser.y"
{ yyval.dblval = pow(yyvsp[-2].dblval, yyvsp[0].dblval); }
    break;
case 48:
#line 355 "mapparser.y"
{ yyval.dblval = yyvsp[-1].dblval; }
    break;
case 49:
#line 356 "mapparser.y"
{ yyval.dblval = strlen(yyvsp[-1].strval); }
    break;
case 51:
#line 360 "mapparser.y"
{ yyval.strval = yyvsp[-1].strval; free(yyvsp[-1].strval); }
    break;
case 52:
#line 361 "mapparser.y"
{ sprintf(yyval.strval, "%s%s", yyvsp[-2].strval, yyvsp[0].strval); free(yyvsp[-2].strval); free(yyvsp[0].strval); }
    break;
case 54:
#line 365 "mapparser.y"
{ yyval.tmval = yyvsp[-1].tmval; }
    break;
}

#line 705 "/sw/share/bison/bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
yyerrhandle:
  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
#line 368 "mapparser.y"

