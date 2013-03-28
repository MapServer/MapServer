/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     BOOLEAN = 258,
     NUMBER = 259,
     STRING = 260,
     TIME = 261,
     SHAPE = 262,
     OR = 263,
     AND = 264,
     NOT = 265,
     IRE = 266,
     IEQ = 267,
     IN = 268,
     GE = 269,
     LE = 270,
     GT = 271,
     LT = 272,
     NE = 273,
     EQ = 274,
     RE = 275,
     DWITHIN = 276,
     BEYOND = 277,
     EQUALS = 278,
     CONTAINS = 279,
     WITHIN = 280,
     CROSSES = 281,
     OVERLAPS = 282,
     TOUCHES = 283,
     DISJOINT = 284,
     INTERSECTS = 285,
     ROUND = 286,
     COMMIFY = 287,
     LENGTH = 288,
     AREA = 289,
     FIRSTCAP = 290,
     INITCAP = 291,
     LOWER = 292,
     UPPER = 293,
     TOSTRING = 294,
     JAVASCRIPT = 295,
     SMOOTHSIA = 296,
     GENERALIZE = 297,
     SIMPLIFYPT = 298,
     SIMPLIFY = 299,
     DIFFERENCE = 300,
     YYBUFFER = 301,
     NEG = 302
   };
#endif
/* Tokens.  */
#define BOOLEAN 258
#define NUMBER 259
#define STRING 260
#define TIME 261
#define SHAPE 262
#define OR 263
#define AND 264
#define NOT 265
#define IRE 266
#define IEQ 267
#define IN 268
#define GE 269
#define LE 270
#define GT 271
#define LT 272
#define NE 273
#define EQ 274
#define RE 275
#define DWITHIN 276
#define BEYOND 277
#define EQUALS 278
#define CONTAINS 279
#define WITHIN 280
#define CROSSES 281
#define OVERLAPS 282
#define TOUCHES 283
#define DISJOINT 284
#define INTERSECTS 285
#define ROUND 286
#define COMMIFY 287
#define LENGTH 288
#define AREA 289
#define FIRSTCAP 290
#define INITCAP 291
#define LOWER 292
#define UPPER 293
#define TOSTRING 294
#define JAVASCRIPT 295
#define SMOOTHSIA 296
#define GENERALIZE 297
#define SIMPLIFYPT 298
#define SIMPLIFY 299
#define DIFFERENCE 300
#define YYBUFFER 301
#define NEG 302




/* Copy the first part of user declarations.  */
#line 5 "/Users/sdlime/mapserver/mapserver/mapparser.y"

/* C declarations */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "mapserver.h" /* for TRUE/FALSE and REGEX includes */
#include "maptime.h" /* for time comparison routines */
#include "mapprimitive.h" /* for shapeObj */

#include "mapparser.h" /* for YYSTYPE in the function prototype for yylex() */

int yylex(YYSTYPE *, parseObj *); /* prototype functions, defined after the grammar */
int yyerror(parseObj *, const char *);


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 30 "/Users/sdlime/mapserver/mapserver/mapparser.y"
{
  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
  shapeObj *shpval;
}
/* Line 193 of yacc.c.  */
#line 216 "/Users/sdlime/mapserver/mapserver/mapparser.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 229 "/Users/sdlime/mapserver/mapserver/mapparser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
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
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  78
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   510

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  57
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  7
/* YYNRULES -- Number of rules.  */
#define YYNRULES  97
/* YYNRULES -- Number of states.  */
#define YYNSTATES  287

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   302

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    51,     2,     2,
      54,    55,    49,    47,    56,    48,     2,    50,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    53,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    52
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     6,     8,    10,    12,    14,    18,
      22,    26,    30,    34,    38,    42,    46,    50,    54,    57,
      60,    64,    68,    72,    76,    80,    84,    88,    92,    96,
     100,   104,   108,   112,   116,   120,   124,   128,   132,   136,
     140,   144,   148,   152,   156,   160,   164,   171,   178,   182,
     189,   193,   200,   204,   211,   215,   222,   226,   233,   237,
     244,   248,   257,   266,   268,   272,   276,   280,   284,   288,
     292,   295,   299,   304,   309,   316,   318,   322,   329,   336,
     343,   350,   357,   362,   369,   378,   389,   396,   398,   402,
     406,   413,   418,   423,   428,   433,   438,   440
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      58,     0,    -1,    -1,    59,    -1,    60,    -1,    62,    -1,
      61,    -1,     3,    -1,    54,    59,    55,    -1,    59,    19,
      59,    -1,    59,     8,    59,    -1,    59,     9,    59,    -1,
      59,     8,    60,    -1,    59,     9,    60,    -1,    60,     8,
      59,    -1,    60,     9,    59,    -1,    60,     8,    60,    -1,
      60,     9,    60,    -1,    10,    59,    -1,    10,    60,    -1,
      62,    20,    62,    -1,    62,    11,    62,    -1,    60,    19,
      60,    -1,    60,    18,    60,    -1,    60,    16,    60,    -1,
      60,    17,    60,    -1,    60,    14,    60,    -1,    60,    15,
      60,    -1,    62,    19,    62,    -1,    62,    18,    62,    -1,
      62,    16,    62,    -1,    62,    17,    62,    -1,    62,    14,
      62,    -1,    62,    15,    62,    -1,    63,    19,    63,    -1,
      63,    18,    63,    -1,    63,    16,    63,    -1,    63,    17,
      63,    -1,    63,    14,    63,    -1,    63,    15,    63,    -1,
      62,    13,    62,    -1,    60,    13,    62,    -1,    60,    12,
      60,    -1,    62,    12,    62,    -1,    63,    12,    63,    -1,
      61,    19,    61,    -1,    23,    54,    61,    56,    61,    55,
      -1,    30,    54,    61,    56,    61,    55,    -1,    61,    30,
      61,    -1,    29,    54,    61,    56,    61,    55,    -1,    61,
      29,    61,    -1,    28,    54,    61,    56,    61,    55,    -1,
      61,    28,    61,    -1,    27,    54,    61,    56,    61,    55,
      -1,    61,    27,    61,    -1,    26,    54,    61,    56,    61,
      55,    -1,    61,    26,    61,    -1,    25,    54,    61,    56,
      61,    55,    -1,    61,    25,    61,    -1,    24,    54,    61,
      56,    61,    55,    -1,    61,    24,    61,    -1,    21,    54,
      61,    56,    61,    56,    60,    55,    -1,    22,    54,    61,
      56,    61,    56,    60,    55,    -1,     4,    -1,    54,    60,
      55,    -1,    60,    47,    60,    -1,    60,    48,    60,    -1,
      60,    49,    60,    -1,    60,    51,    60,    -1,    60,    50,
      60,    -1,    48,    60,    -1,    60,    53,    60,    -1,    33,
      54,    62,    55,    -1,    34,    54,    61,    55,    -1,    31,
      54,    60,    56,    60,    55,    -1,     7,    -1,    54,    61,
      55,    -1,    46,    54,    61,    56,    60,    55,    -1,    45,
      54,    61,    56,    61,    55,    -1,    44,    54,    61,    56,
      60,    55,    -1,    43,    54,    61,    56,    60,    55,    -1,
      42,    54,    61,    56,    60,    55,    -1,    41,    54,    61,
      55,    -1,    41,    54,    61,    56,    60,    55,    -1,    41,
      54,    61,    56,    60,    56,    60,    55,    -1,    41,    54,
      61,    56,    60,    56,    60,    56,    62,    55,    -1,    40,
      54,    61,    56,    62,    55,    -1,     5,    -1,    54,    62,
      55,    -1,    62,    47,    62,    -1,    39,    54,    60,    56,
      62,    55,    -1,    32,    54,    62,    55,    -1,    38,    54,
      62,    55,    -1,    37,    54,    62,    55,    -1,    36,    54,
      62,    55,    -1,    35,    54,    62,    55,    -1,     6,    -1,
      54,    63,    55,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    68,    68,    69,    82,    96,   109,   119,   120,   121,
     125,   133,   142,   150,   159,   167,   176,   184,   193,   194,
     195,   215,   235,   241,   247,   253,   259,   265,   271,   279,
     287,   295,   303,   311,   319,   325,   331,   337,   343,   349,
     355,   376,   396,   402,   410,   416,   427,   438,   449,   460,
     471,   482,   493,   504,   515,   526,   537,   548,   559,   570,
     581,   592,   602,   614,   615,   616,   617,   618,   619,   620,
     627,   628,   629,   630,   638,   641,   642,   643,   653,   663,
     673,   683,   693,   703,   713,   723,   734,   752,   753,   754,
     758,   762,   766,   770,   774,   778,   784,   785
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "BOOLEAN", "NUMBER", "STRING", "TIME",
  "SHAPE", "OR", "AND", "NOT", "IRE", "IEQ", "IN", "GE", "LE", "GT", "LT",
  "NE", "EQ", "RE", "DWITHIN", "BEYOND", "EQUALS", "CONTAINS", "WITHIN",
  "CROSSES", "OVERLAPS", "TOUCHES", "DISJOINT", "INTERSECTS", "ROUND",
  "COMMIFY", "LENGTH", "AREA", "FIRSTCAP", "INITCAP", "LOWER", "UPPER",
  "TOSTRING", "JAVASCRIPT", "SMOOTHSIA", "GENERALIZE", "SIMPLIFYPT",
  "SIMPLIFY", "DIFFERENCE", "YYBUFFER", "'+'", "'-'", "'*'", "'/'", "'%'",
  "NEG", "'^'", "'('", "')'", "','", "$accept", "input", "logical_exp",
  "math_exp", "shape_exp", "string_exp", "time_exp", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,    43,    45,    42,
      47,    37,   302,    94,    40,    41,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    57,    58,    58,    58,    58,    58,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    61,    61,    61,    61,    61,
      61,    61,    61,    61,    61,    61,    61,    62,    62,    62,
      62,    62,    62,    62,    62,    62,    63,    63
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     1,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     6,     6,     3,     6,
       3,     6,     3,     6,     3,     6,     3,     6,     3,     6,
       3,     8,     8,     1,     3,     3,     3,     3,     3,     3,
       2,     3,     4,     4,     6,     1,     3,     6,     6,     6,
       6,     6,     4,     6,     8,    10,     6,     1,     3,     3,
       6,     4,     4,     4,     4,     4,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     7,    63,    87,    96,    75,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     3,     4,     6,     5,
       0,    18,    19,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    70,     0,     0,     0,     0,     0,     1,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     8,    64,    76,    88,    97,    10,    12,
      11,    13,     9,     0,    14,    16,    15,    17,    42,    41,
      26,    27,    24,    25,    23,    22,    65,    66,    67,    69,
      68,    71,    45,    60,    58,    56,    54,    52,    50,    48,
      21,    43,    40,    32,    33,    30,    31,    29,    28,    20,
      89,     0,    44,    38,    39,    36,    37,    35,    34,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    91,    72,    73,    95,    94,    93,    92,     0,
       0,    82,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      46,    59,    57,    55,    53,    51,    49,    47,    74,    90,
      86,    83,     0,    81,    80,    79,    78,    77,     0,     0,
       0,    61,    62,    84,     0,     0,    85
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    35,    36,    37,    43,    44,    40
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -50
static const yytype_int16 yypact[] =
{
     310,   -50,   -50,   -50,   -50,   -50,   310,   -32,   -30,   -17,
     -15,   -14,   -11,   -10,    10,    12,    14,    15,    16,    72,
      79,    95,    96,    98,    99,   100,   112,   113,   122,   123,
     129,   134,   135,    -1,   310,    27,    -4,   238,   399,   372,
     491,    35,   246,   399,   372,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,    -1,    13,    13,   178,    13,
      13,    13,    13,    -1,   178,   178,   178,   178,   178,   178,
     178,    -1,     2,    17,   156,   386,   362,   219,   -50,   310,
     310,   310,   310,   310,    -1,    13,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   178,   178,
     178,   178,   178,   178,   178,   178,    13,    13,    13,    13,
      13,    13,    13,    13,    13,    13,    13,     4,     4,     4,
       4,     4,     4,     4,   178,   143,   144,   145,   152,   154,
     159,   169,   170,   171,   183,    81,    13,   -39,   -38,    80,
     -36,   -34,   -27,   -24,   274,   188,   -49,   192,   193,   212,
     220,   223,   383,   -50,   -50,   -50,   -50,   -50,    -7,   353,
      35,   246,   -50,   238,    -7,   353,    35,   246,    33,   108,
      33,    33,    33,    33,    33,    33,   163,   163,     2,     2,
       2,     2,   -50,   -50,   -50,   -50,   -50,   -50,   -50,   -50,
     108,   108,   108,   108,   108,   108,   108,   108,   108,   108,
     -50,     4,   -50,   -50,   -50,   -50,   -50,   -50,   -50,   226,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
      -1,    -9,   -50,   -50,   -50,   -50,   -50,   -50,   -50,    13,
      13,   -50,    -1,    -1,    -1,    -1,   178,    -1,   227,   234,
     236,   228,   243,   245,   247,   263,   264,   271,   273,   395,
      18,    70,   131,   404,   413,   422,   302,   431,    -1,    -1,
     -50,   -50,   -50,   -50,   -50,   -50,   -50,   -50,   -50,   -50,
     -50,   -50,    -1,   -50,   -50,   -50,   -50,   -50,   440,   449,
     222,   -50,   -50,   -50,    13,   101,   -50
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -50,   -50,    -5,     8,    93,     0,     1
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      39,    41,    80,     2,    79,    80,   231,   232,   116,   116,
       4,   116,    81,   116,    42,    81,   222,   223,     3,   225,
     116,   226,    45,   116,    46,    79,    80,    78,   227,    73,
      17,   228,    19,    20,    76,    77,    81,    47,   116,    48,
      49,    72,    74,    50,    51,    18,   156,    33,    21,    22,
      23,    24,    25,    71,    81,    97,   137,   138,   201,   140,
     141,   142,   143,   135,    52,   116,    53,   136,    54,    55,
      56,   144,   153,   269,   158,   160,   162,   164,   166,   152,
      92,    93,    94,    95,    96,   169,    97,   159,   161,   163,
     165,   167,   168,    38,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   116,   202,   203,
     204,   205,   206,   207,   208,   270,    57,    75,    92,    93,
      94,    95,    96,    58,    97,   224,   221,   220,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   116,    59,
      60,   139,    61,    62,    63,   116,   286,   145,   146,   147,
     148,   149,   150,   151,    82,    83,    64,    65,    84,    85,
      86,    87,    88,    89,    90,    91,    66,    67,    92,    93,
      94,    95,    96,    68,    97,     5,   271,   272,    69,    70,
       0,   182,   183,   184,   185,   186,   187,   188,   189,   210,
     211,   212,   238,    92,    93,    94,    95,    96,   213,    97,
     214,   154,    94,    95,    96,   215,    97,   209,    26,    27,
      28,    29,    30,    31,    32,   216,   217,   218,   249,   250,
     251,   117,   124,   118,   119,   120,   121,   122,   123,   219,
     252,   253,   254,   255,   230,   257,    82,    83,   233,   234,
      84,    85,    86,    87,    88,    89,    90,    91,    84,    85,
      86,    87,    88,    89,    90,    91,   278,   279,   235,    92,
      93,    94,    95,    96,   157,    97,   236,   283,   284,   237,
     280,   155,   157,   260,   285,    92,    93,    94,    95,    96,
     258,    97,   259,    92,    93,    94,    95,    96,   261,    97,
     262,     0,   263,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,     1,     2,     3,     4,     5,   264,   265,
       6,    92,    93,    94,    95,    96,   266,    97,   267,   256,
     229,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,   276,    33,     0,
       0,     0,    83,     0,    34,    84,    85,    86,    87,    88,
      89,    90,    91,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,     0,     0,     0,     0,     0,     0,     0,
      92,    93,    94,    95,    96,    98,    97,     0,     0,   116,
      99,   100,   101,   102,   103,   104,   105,   156,    98,   116,
       0,     0,     0,    99,   100,   101,   102,   103,   104,   105,
      92,    93,    94,    95,    96,     0,    97,     0,   154,     0,
       0,   155,    92,    93,    94,    95,    96,     0,    97,     0,
     268,    92,    93,    94,    95,    96,     0,    97,     0,   273,
      92,    93,    94,    95,    96,     0,    97,     0,   274,    92,
      93,    94,    95,    96,     0,    97,     0,   275,    92,    93,
      94,    95,    96,     0,    97,     0,   277,    92,    93,    94,
      95,    96,     0,    97,     0,   281,    92,    93,    94,    95,
      96,     0,    97,   117,   282,   118,   119,   120,   121,   122,
     123
};

static const yytype_int16 yycheck[] =
{
       0,     6,     9,     4,     8,     9,    55,    56,    47,    47,
       6,    47,    19,    47,     6,    19,    55,    55,     5,    55,
      47,    55,    54,    47,    54,     8,     9,     0,    55,    34,
      31,    55,    33,    34,    34,    34,    19,    54,    47,    54,
      54,    33,    34,    54,    54,    32,    55,    48,    35,    36,
      37,    38,    39,    54,    19,    53,    56,    57,    54,    59,
      60,    61,    62,    55,    54,    47,    54,    54,    54,    54,
      54,    63,    55,    55,    79,    80,    81,    82,    83,    71,
      47,    48,    49,    50,    51,    85,    53,    79,    80,    81,
      82,    83,    84,     0,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,    47,   117,   118,
     119,   120,   121,   122,   123,    55,    54,    34,    47,    48,
      49,    50,    51,    54,    53,    55,   136,    56,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    47,    54,
      54,    58,    54,    54,    54,    47,    55,    64,    65,    66,
      67,    68,    69,    70,     8,     9,    54,    54,    12,    13,
      14,    15,    16,    17,    18,    19,    54,    54,    47,    48,
      49,    50,    51,    54,    53,     7,    55,    56,    54,    54,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,    56,
      56,    56,   201,    47,    48,    49,    50,    51,    56,    53,
      56,    55,    49,    50,    51,    56,    53,   124,    40,    41,
      42,    43,    44,    45,    46,    56,    56,    56,   220,   229,
     230,    12,    54,    14,    15,    16,    17,    18,    19,    56,
     232,   233,   234,   235,    56,   237,     8,     9,    56,    56,
      12,    13,    14,    15,    16,    17,    18,    19,    12,    13,
      14,    15,    16,    17,    18,    19,   258,   259,    56,    47,
      48,    49,    50,    51,    55,    53,    56,    55,    56,    56,
     272,    55,    55,    55,   284,    47,    48,    49,    50,    51,
      56,    53,    56,    47,    48,    49,    50,    51,    55,    53,
      55,    -1,    55,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,     3,     4,     5,     6,     7,    55,    55,
      10,    47,    48,    49,    50,    51,    55,    53,    55,   236,
      56,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    55,    48,    -1,
      -1,    -1,     9,    -1,    54,    12,    13,    14,    15,    16,
      17,    18,    19,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    48,    49,    50,    51,    19,    53,    -1,    -1,    47,
      24,    25,    26,    27,    28,    29,    30,    55,    19,    47,
      -1,    -1,    -1,    24,    25,    26,    27,    28,    29,    30,
      47,    48,    49,    50,    51,    -1,    53,    -1,    55,    -1,
      -1,    55,    47,    48,    49,    50,    51,    -1,    53,    -1,
      55,    47,    48,    49,    50,    51,    -1,    53,    -1,    55,
      47,    48,    49,    50,    51,    -1,    53,    -1,    55,    47,
      48,    49,    50,    51,    -1,    53,    -1,    55,    47,    48,
      49,    50,    51,    -1,    53,    -1,    55,    47,    48,    49,
      50,    51,    -1,    53,    -1,    55,    47,    48,    49,    50,
      51,    -1,    53,    12,    55,    14,    15,    16,    17,    18,
      19
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,    10,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    48,    54,    58,    59,    60,    61,    62,
      63,    59,    60,    61,    62,    54,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54,    54,    54,    54,    54,
      54,    54,    60,    59,    60,    61,    62,    63,     0,     8,
       9,    19,     8,     9,    12,    13,    14,    15,    16,    17,
      18,    19,    47,    48,    49,    50,    51,    53,    19,    24,
      25,    26,    27,    28,    29,    30,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    47,    12,    14,    15,
      16,    17,    18,    19,    54,    61,    61,    61,    61,    61,
      61,    61,    61,    61,    61,    60,    54,    62,    62,    61,
      62,    62,    62,    62,    60,    61,    61,    61,    61,    61,
      61,    61,    60,    55,    55,    55,    55,    55,    59,    60,
      59,    60,    59,    60,    59,    60,    59,    60,    60,    62,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    61,    61,    61,    61,    61,    61,    61,    61,
      62,    62,    62,    62,    62,    62,    62,    62,    62,    62,
      62,    54,    63,    63,    63,    63,    63,    63,    63,    61,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    62,    55,    55,    55,    55,    55,    55,    55,    56,
      56,    55,    56,    56,    56,    56,    56,    56,    63,    61,
      61,    61,    61,    61,    61,    61,    61,    61,    61,    60,
      62,    62,    60,    60,    60,    60,    61,    60,    56,    56,
      55,    55,    55,    55,    55,    55,    55,    55,    55,    55,
      55,    55,    56,    55,    55,    55,    55,    55,    60,    60,
      60,    55,    55,    55,    56,    62,    55
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


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
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (p, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, p)
#endif

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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, p); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parseObj *p)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, p)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    parseObj *p;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (p);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parseObj *p)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, p)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    parseObj *p;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, p);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, parseObj *p)
#else
static void
yy_reduce_print (yyvsp, yyrule, p)
    YYSTYPE *yyvsp;
    int yyrule;
    parseObj *p;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , p);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, p); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, parseObj *p)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, p)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    parseObj *p;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (p);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (parseObj *p);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (parseObj *p)
#else
int
yyparse (p)
    parseObj *p;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

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

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 69 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    switch(p->type) {
    case(MS_PARSE_TYPE_BOOLEAN):
      p->result.intval = (yyvsp[(1) - (1)].intval); 
      break;
    case(MS_PARSE_TYPE_STRING):
      if((yyvsp[(1) - (1)].intval)) 
        p->result.strval = strdup("true");
      else
        p->result.strval = strdup("false");
      break;
    }
  }
    break;

  case 4:
#line 82 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    switch(p->type) {
    case(MS_PARSE_TYPE_BOOLEAN):
      if((yyvsp[(1) - (1)].dblval) != 0)
        p->result.intval = MS_TRUE;
      else
        p->result.intval = MS_FALSE;			    
      break;
    case(MS_PARSE_TYPE_STRING):
      p->result.strval = (char *)malloc(64); /* large enough for a double */
      snprintf(p->result.strval, 64, "%g", (yyvsp[(1) - (1)].dblval));
      break;
    }
  }
    break;

  case 5:
#line 96 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    switch(p->type) {
    case(MS_PARSE_TYPE_BOOLEAN):
      if((yyvsp[(1) - (1)].strval)) /* string is not NULL */
        p->result.intval = MS_TRUE;
      else
        p->result.intval = MS_FALSE;
      break;
    case(MS_PARSE_TYPE_STRING):
      p->result.strval = (yyvsp[(1) - (1)].strval); // strdup($1);
      break;
    }
  }
    break;

  case 6:
#line 109 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    switch(p->type) {
    case(MS_PARSE_TYPE_SHAPE):
      p->result.shpval = (yyvsp[(1) - (1)].shpval);
      p->result.shpval->scratch = MS_FALSE;
      break;
    }
  }
    break;

  case 8:
#line 120 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.intval) = (yyvsp[(2) - (3)].intval); }
    break;

  case 9:
#line 121 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    (yyval.intval) = MS_FALSE;
    if((yyvsp[(1) - (3)].intval) == (yyvsp[(3) - (3)].intval)) (yyval.intval) = MS_TRUE;
  }
    break;

  case 10:
#line 125 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].intval) == MS_TRUE)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[(3) - (3)].intval) == MS_TRUE)          
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 11:
#line 133 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].intval) == MS_TRUE) {
      if((yyvsp[(3) - (3)].intval) == MS_TRUE)
        (yyval.intval) = MS_TRUE;
      else
        (yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 12:
#line 142 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].intval) == MS_TRUE)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[(3) - (3)].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 13:
#line 150 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].intval) == MS_TRUE) {
      if((yyvsp[(3) - (3)].dblval) != 0)
        (yyval.intval) = MS_TRUE;
      else
        (yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 14:
#line 159 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[(3) - (3)].intval) == MS_TRUE)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 15:
#line 167 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) != 0) {
      if((yyvsp[(3) - (3)].intval) == MS_TRUE)
        (yyval.intval) = MS_TRUE;
      else
        (yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 16:
#line 176 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[(3) - (3)].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 17:
#line 184 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) != 0) {
      if((yyvsp[(3) - (3)].dblval) != 0)
        (yyval.intval) = MS_TRUE;
      else
	(yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 18:
#line 193 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.intval) = !(yyvsp[(2) - (2)].intval); }
    break;

  case 19:
#line 194 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.intval) = !(yyvsp[(2) - (2)].dblval); }
    break;

  case 20:
#line 195 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    ms_regex_t re;

    if(MS_STRING_IS_NULL_OR_EMPTY((yyvsp[(1) - (3)].strval)) == MS_TRUE) {
      (yyval.intval) = MS_FALSE;
    } else {
      if(ms_regcomp(&re, (yyvsp[(3) - (3)].strval), MS_REG_EXTENDED|MS_REG_NOSUB) != 0) {      
        (yyval.intval) = MS_FALSE;
      } else {
        if(ms_regexec(&re, (yyvsp[(1) - (3)].strval), 0, NULL, 0) == 0)
          (yyval.intval) = MS_TRUE;
        else
          (yyval.intval) = MS_FALSE;
        ms_regfree(&re);
      }
    }

    free((yyvsp[(1) - (3)].strval));
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 21:
#line 215 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    ms_regex_t re;

    if(MS_STRING_IS_NULL_OR_EMPTY((yyvsp[(1) - (3)].strval)) == MS_TRUE) {
      (yyval.intval) = MS_FALSE;
    } else {
      if(ms_regcomp(&re, (yyvsp[(3) - (3)].strval), MS_REG_EXTENDED|MS_REG_NOSUB|MS_REG_ICASE) != 0) {
        (yyval.intval) = MS_FALSE;
      } else {
        if(ms_regexec(&re, (yyvsp[(1) - (3)].strval), 0, NULL, 0) == 0)
          (yyval.intval) = MS_TRUE;
        else
          (yyval.intval) = MS_FALSE;
        ms_regfree(&re);
      }
    }

    free((yyvsp[(1) - (3)].strval));
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 22:
#line 235 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) == (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 23:
#line 241 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) != (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 24:
#line 247 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {    
    if((yyvsp[(1) - (3)].dblval) > (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 25:
#line 253 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) < (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 26:
#line 259 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) >= (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 27:
#line 265 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) <= (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 28:
#line 271 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(strcmp((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].strval)) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[(1) - (3)].strval));
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 29:
#line 279 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(strcmp((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].strval)) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[(1) - (3)].strval));
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 30:
#line 287 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(strcmp((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].strval)) > 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[(1) - (3)].strval));
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 31:
#line 295 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(strcmp((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].strval)) < 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[(1) - (3)].strval));
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 32:
#line 303 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(strcmp((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].strval)) >= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[(1) - (3)].strval));
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 33:
#line 311 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(strcmp((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].strval)) <= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[(1) - (3)].strval));
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 34:
#line 319 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 35:
#line 325 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 36:
#line 331 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) > 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 37:
#line 337 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) < 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 38:
#line 343 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) >= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 39:
#line 349 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) <= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 40:
#line 355 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    char *delim, *bufferp;

    (yyval.intval) = MS_FALSE;
    bufferp=(yyvsp[(3) - (3)].strval);

    while((delim=strchr(bufferp,',')) != NULL) {
      *delim='\0';
      if(strcmp((yyvsp[(1) - (3)].strval),bufferp) == 0) {
        (yyval.intval) = MS_TRUE;
        break;
      } 
      *delim=',';
      bufferp=delim+1;
    }

    if((yyval.intval) == MS_FALSE && strcmp((yyvsp[(1) - (3)].strval),bufferp) == 0) // test for last (or only) item
      (yyval.intval) = MS_TRUE;
    free((yyvsp[(1) - (3)].strval));
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 41:
#line 376 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    char *delim,*bufferp;

    (yyval.intval) = MS_FALSE;
    bufferp=(yyvsp[(3) - (3)].strval);

    while((delim=strchr(bufferp,',')) != NULL) {
      *delim='\0';
      if((yyvsp[(1) - (3)].dblval) == atof(bufferp)) {
        (yyval.intval) = MS_TRUE;
        break;
      } 
      *delim=',';
      bufferp=delim+1;
    }

    if((yyvsp[(1) - (3)].dblval) == atof(bufferp)) // is this test necessary?
      (yyval.intval) = MS_TRUE;  
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 42:
#line 396 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) == (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 43:
#line 402 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(strcasecmp((yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].strval)) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[(1) - (3)].strval));
    free((yyvsp[(3) - (3)].strval));
  }
    break;

  case 44:
#line 410 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 45:
#line 416 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSEquals((yyvsp[(1) - (3)].shpval), (yyvsp[(3) - (3)].shpval));
    if((yyvsp[(1) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(1) - (3)].shpval));
    if((yyvsp[(3) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (3)].shpval));
    if(rval == -1) {
      yyerror(p, "Equals (EQ or ==) operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 46:
#line 427 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSEquals((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].shpval));
    if((yyvsp[(3) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (6)].shpval));
    if((yyvsp[(5) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(5) - (6)].shpval));
    if(rval == -1) {
      yyerror(p, "Equals function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 47:
#line 438 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSIntersects((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].shpval));
    if((yyvsp[(3) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (6)].shpval));
    if((yyvsp[(5) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(5) - (6)].shpval));
    if(rval == -1) {
      yyerror(p, "Intersects function failed.");
      return(-1);
    } else
    (yyval.intval) = rval;
  }
    break;

  case 48:
#line 449 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSIntersects((yyvsp[(1) - (3)].shpval), (yyvsp[(3) - (3)].shpval));
    if((yyvsp[(1) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(1) - (3)].shpval));
    if((yyvsp[(3) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (3)].shpval)); 
    if(rval == -1) {
      yyerror(p, "Intersects operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 49:
#line 460 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSDisjoint((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].shpval));
    if((yyvsp[(3) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (6)].shpval));
    if((yyvsp[(5) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(5) - (6)].shpval));
    if(rval == -1) {
      yyerror(p, "Disjoint function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 50:
#line 471 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSDisjoint((yyvsp[(1) - (3)].shpval), (yyvsp[(3) - (3)].shpval));
    if((yyvsp[(1) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(1) - (3)].shpval));
    if((yyvsp[(3) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (3)].shpval));
    if(rval == -1) {
      yyerror(p, "Disjoint operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 51:
#line 482 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSTouches((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].shpval));
    if((yyvsp[(3) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (6)].shpval));
    if((yyvsp[(5) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(5) - (6)].shpval));
    if(rval == -1) {
      yyerror(p, "Touches function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 52:
#line 493 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSTouches((yyvsp[(1) - (3)].shpval), (yyvsp[(3) - (3)].shpval));
    if((yyvsp[(1) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(1) - (3)].shpval));
    if((yyvsp[(3) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (3)].shpval));
    if(rval == -1) {
      yyerror(p, "Touches operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 53:
#line 504 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSOverlaps((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].shpval));
    if((yyvsp[(3) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (6)].shpval));
    if((yyvsp[(5) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(5) - (6)].shpval));
    if(rval == -1) {
      yyerror(p, "Overlaps function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 54:
#line 515 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
     rval = msGEOSOverlaps((yyvsp[(1) - (3)].shpval), (yyvsp[(3) - (3)].shpval));
    if((yyvsp[(1) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(1) - (3)].shpval));
    if((yyvsp[(3) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (3)].shpval));
    if(rval == -1) {
      yyerror(p, "Overlaps operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 55:
#line 526 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSCrosses((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].shpval));
    if((yyvsp[(3) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (6)].shpval));
    if((yyvsp[(5) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(5) - (6)].shpval));
    if(rval == -1) {
      yyerror(p, "Crosses function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 56:
#line 537 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSCrosses((yyvsp[(1) - (3)].shpval), (yyvsp[(3) - (3)].shpval));
    if((yyvsp[(1) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(1) - (3)].shpval));
    if((yyvsp[(3) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (3)].shpval));
    if(rval == -1) {
      yyerror(p, "Crosses operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 57:
#line 548 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSWithin((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].shpval));
    if((yyvsp[(3) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (6)].shpval));
    if((yyvsp[(5) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(5) - (6)].shpval));
    if(rval == -1) {
      yyerror(p, "Within function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 58:
#line 559 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSWithin((yyvsp[(1) - (3)].shpval), (yyvsp[(3) - (3)].shpval));
    if((yyvsp[(1) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(1) - (3)].shpval));
    if((yyvsp[(3) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (3)].shpval));
    if(rval == -1) {
      yyerror(p, "Within operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 59:
#line 570 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSContains((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].shpval));
    if((yyvsp[(3) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (6)].shpval));
    if((yyvsp[(5) - (6)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(5) - (6)].shpval));
    if(rval == -1) {
      yyerror(p, "Contains function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 60:
#line 581 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    int rval;
    rval = msGEOSContains((yyvsp[(1) - (3)].shpval), (yyvsp[(3) - (3)].shpval));
    if((yyvsp[(1) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(1) - (3)].shpval));
    if((yyvsp[(3) - (3)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (3)].shpval));
    if(rval == -1) {
      yyerror(p, "Contains operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
    break;

  case 61:
#line 592 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    double d;
    d = msGEOSDistance((yyvsp[(3) - (8)].shpval), (yyvsp[(5) - (8)].shpval));    
    if((yyvsp[(3) - (8)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (8)].shpval));
    if((yyvsp[(5) - (8)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(5) - (8)].shpval));
    if(d <= (yyvsp[(7) - (8)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 62:
#line 602 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    double d;
    d = msGEOSDistance((yyvsp[(3) - (8)].shpval), (yyvsp[(5) - (8)].shpval));
    if((yyvsp[(3) - (8)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (8)].shpval));
    if((yyvsp[(5) - (8)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(5) - (8)].shpval));
    if(d > (yyvsp[(7) - (8)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 64:
#line 615 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.dblval) = (yyvsp[(2) - (3)].dblval); }
    break;

  case 65:
#line 616 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.dblval) = (yyvsp[(1) - (3)].dblval) + (yyvsp[(3) - (3)].dblval); }
    break;

  case 66:
#line 617 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.dblval) = (yyvsp[(1) - (3)].dblval) - (yyvsp[(3) - (3)].dblval); }
    break;

  case 67:
#line 618 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.dblval) = (yyvsp[(1) - (3)].dblval) * (yyvsp[(3) - (3)].dblval); }
    break;

  case 68:
#line 619 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.dblval) = (int)(yyvsp[(1) - (3)].dblval) % (int)(yyvsp[(3) - (3)].dblval); }
    break;

  case 69:
#line 620 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(3) - (3)].dblval) == 0.0) {
      yyerror(p, "Division by zero.");
      return(-1);
    } else
      (yyval.dblval) = (yyvsp[(1) - (3)].dblval) / (yyvsp[(3) - (3)].dblval); 
  }
    break;

  case 70:
#line 627 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.dblval) = (yyvsp[(2) - (2)].dblval); }
    break;

  case 71:
#line 628 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.dblval) = pow((yyvsp[(1) - (3)].dblval), (yyvsp[(3) - (3)].dblval)); }
    break;

  case 72:
#line 629 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.dblval) = strlen((yyvsp[(3) - (4)].strval)); }
    break;

  case 73:
#line 630 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    if((yyvsp[(3) - (4)].shpval)->type != MS_SHAPE_POLYGON) {
      yyerror(p, "Area can only be computed for polygon shapes.");
      return(-1);
    }
    (yyval.dblval) = msGetPolygonArea((yyvsp[(3) - (4)].shpval));
    if((yyvsp[(3) - (4)].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[(3) - (4)].shpval));
  }
    break;

  case 74:
#line 638 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.dblval) = (MS_NINT((yyvsp[(3) - (6)].dblval)/(yyvsp[(5) - (6)].dblval)))*(yyvsp[(5) - (6)].dblval); }
    break;

  case 76:
#line 642 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.shpval) = (yyvsp[(2) - (3)].shpval); }
    break;

  case 77:
#line 643 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    shapeObj *s;
    s = msGEOSBuffer((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].dblval));
    if(!s) {
      yyerror(p, "Executing buffer failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 78:
#line 653 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    shapeObj *s;
    s = msGEOSDifference((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].shpval));
    if(!s) {
      yyerror(p, "Executing difference failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 79:
#line 663 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    shapeObj *s;
    s = msGEOSSimplify((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].dblval));
    if(!s) {
      yyerror(p, "Executing simplify failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 80:
#line 673 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    shapeObj *s;
    s = msGEOSTopologyPreservingSimplify((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].dblval));
    if(!s) {
      yyerror(p, "Executing simplifypt failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 81:
#line 683 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    shapeObj *s;
    s = msGeneralize((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].dblval));
    if(!s) {
      yyerror(p, "Executing generalize failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 82:
#line 693 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[(3) - (4)].shpval), 3, 1, NULL);
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 83:
#line 703 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].dblval), 1, NULL);
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 84:
#line 713 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[(3) - (8)].shpval), (yyvsp[(5) - (8)].dblval), (yyvsp[(7) - (8)].dblval), NULL);
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 85:
#line 723 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[(3) - (10)].shpval), (yyvsp[(5) - (10)].dblval), (yyvsp[(7) - (10)].dblval), (yyvsp[(9) - (10)].strval));
    free((yyvsp[(9) - (10)].strval));
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 86:
#line 734 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
#ifdef USE_V8_MAPSCRIPT
    shapeObj *s;
    s = msV8TransformShape((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].strval));
    free((yyvsp[(5) - (6)].strval));
    if(!s) {
      yyerror(p, "Executing javascript failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
#else
    yyerror(p, "Javascript support not compiled in");
    return(-1);
#endif
  }
    break;

  case 88:
#line 753 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.strval) = (yyvsp[(2) - (3)].strval); }
    break;

  case 89:
#line 754 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { 
    (yyval.strval) = (char *)malloc(strlen((yyvsp[(1) - (3)].strval)) + strlen((yyvsp[(3) - (3)].strval)) + 1);
    sprintf((yyval.strval), "%s%s", (yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].strval)); free((yyvsp[(1) - (3)].strval)); free((yyvsp[(3) - (3)].strval)); 
  }
    break;

  case 90:
#line 758 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {
    (yyval.strval) = (char *) malloc(strlen((yyvsp[(5) - (6)].strval)) + 64); /* Plenty big? Should use snprintf below... */
    sprintf((yyval.strval), (yyvsp[(5) - (6)].strval), (yyvsp[(3) - (6)].dblval));
  }
    break;

  case 91:
#line 762 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {  
    (yyvsp[(3) - (4)].strval) = msCommifyString((yyvsp[(3) - (4)].strval)); 
    (yyval.strval) = (yyvsp[(3) - (4)].strval); 
  }
    break;

  case 92:
#line 766 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {  
    msStringToUpper((yyvsp[(3) - (4)].strval)); 
    (yyval.strval) = (yyvsp[(3) - (4)].strval); 
  }
    break;

  case 93:
#line 770 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {  
    msStringToLower((yyvsp[(3) - (4)].strval)); 
    (yyval.strval) = (yyvsp[(3) - (4)].strval); 
  }
    break;

  case 94:
#line 774 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {  
    msStringInitCap((yyvsp[(3) - (4)].strval)); 
    (yyval.strval) = (yyvsp[(3) - (4)].strval); 
  }
    break;

  case 95:
#line 778 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    {  
    msStringFirstCap((yyvsp[(3) - (4)].strval)); 
    (yyval.strval) = (yyvsp[(3) - (4)].strval); 
  }
    break;

  case 97:
#line 785 "/Users/sdlime/mapserver/mapserver/mapparser.y"
    { (yyval.tmval) = (yyvsp[(2) - (3)].tmval); }
    break;


/* Line 1267 of yacc.c.  */
#line 2759 "/Users/sdlime/mapserver/mapserver/mapparser.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (p, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (p, yymsg);
	  }
	else
	  {
	    yyerror (p, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, p);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, p);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (p, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, p);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, p);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 788 "/Users/sdlime/mapserver/mapserver/mapparser.y"


/*
** Any extra C functions
*/

int yylex(YYSTYPE *lvalp, parseObj *p) 
{
  int token;
  
  if(p->expr->curtoken == NULL) return(0); /* done */

  // fprintf(stderr, "in yylex() - curtoken=%d...\n", p->expr->curtoken->token);

  token = p->expr->curtoken->token; /* may override */
  switch(p->expr->curtoken->token) {
  case MS_TOKEN_LITERAL_BOOLEAN:
    token = BOOLEAN;
    (*lvalp).intval = p->expr->curtoken->tokenval.dblval;
    break;
  case MS_TOKEN_LITERAL_NUMBER:
    token = NUMBER;
    (*lvalp).dblval = p->expr->curtoken->tokenval.dblval;
    break;
  case MS_TOKEN_LITERAL_SHAPE:
    token = SHAPE;
    // fprintf(stderr, "token value = %s\n", msShapeToWKT(p->expr->curtoken->tokenval.shpval));
    (*lvalp).shpval = p->expr->curtoken->tokenval.shpval;
    break;
  case MS_TOKEN_LITERAL_STRING:
    // printf("token value = %s\n", p->expr->curtoken->tokenval.strval); 
    token = STRING;
    (*lvalp).strval = strdup(p->expr->curtoken->tokenval.strval);    
    break;
  case MS_TOKEN_LITERAL_TIME:
    token = TIME;
    (*lvalp).tmval = p->expr->curtoken->tokenval.tmval;
    break;

  case MS_TOKEN_COMPARISON_EQ: token = EQ; break;
  case MS_TOKEN_COMPARISON_IEQ: token = IEQ; break;
  case MS_TOKEN_COMPARISON_NE: token = NE; break;
  case MS_TOKEN_COMPARISON_LT: token = LT; break;
  case MS_TOKEN_COMPARISON_GT: token = GT; break;
  case MS_TOKEN_COMPARISON_LE: token = LE; break;
  case MS_TOKEN_COMPARISON_GE: token = GE; break;
  case MS_TOKEN_COMPARISON_RE: token = RE; break;
  case MS_TOKEN_COMPARISON_IRE: token = IRE; break;

  case MS_TOKEN_COMPARISON_INTERSECTS: token = INTERSECTS; break;
  case MS_TOKEN_COMPARISON_DISJOINT: token = DISJOINT; break;
  case MS_TOKEN_COMPARISON_TOUCHES: token = TOUCHES; break;
  case MS_TOKEN_COMPARISON_OVERLAPS: token = OVERLAPS; break; 
  case MS_TOKEN_COMPARISON_CROSSES: token = CROSSES; break;
  case MS_TOKEN_COMPARISON_WITHIN: token = WITHIN; break; 
  case MS_TOKEN_COMPARISON_CONTAINS: token = CONTAINS; break;
  case MS_TOKEN_COMPARISON_EQUALS: token = EQUALS; break;
  case MS_TOKEN_COMPARISON_BEYOND: token = BEYOND; break;
  case MS_TOKEN_COMPARISON_DWITHIN: token = DWITHIN; break;

  case MS_TOKEN_LOGICAL_AND: token = AND; break;
  case MS_TOKEN_LOGICAL_OR: token = OR; break;
  case MS_TOKEN_LOGICAL_NOT: token = NOT; break;

  case MS_TOKEN_BINDING_DOUBLE:
  case MS_TOKEN_BINDING_INTEGER:
    token = NUMBER;
    (*lvalp).dblval = atof(p->shape->values[p->expr->curtoken->tokenval.bindval.index]);
    break;
  case MS_TOKEN_BINDING_STRING:
    token = STRING;
    (*lvalp).strval = strdup(p->shape->values[p->expr->curtoken->tokenval.bindval.index]);
    break;
  case MS_TOKEN_BINDING_SHAPE:
    token = SHAPE;
    // fprintf(stderr, "token value = %s\n", msShapeToWKT(p->shape));
    (*lvalp).shpval = p->shape;
    break;
  case MS_TOKEN_BINDING_MAP_CELLSIZE:
    token = NUMBER;
    (*lvalp).dblval = p->dblval;
    break;
  case MS_TOKEN_BINDING_DATA_CELLSIZE:
    token = NUMBER;
    (*lvalp).dblval = p->dblval2;
    break;    
  case MS_TOKEN_BINDING_TIME:
    token = TIME;
    msTimeInit(&((*lvalp).tmval));
    if(msParseTime(p->shape->values[p->expr->curtoken->tokenval.bindval.index], &((*lvalp).tmval)) != MS_TRUE) {
      yyerror(p, "Parsing time value failed.");
      return(-1);
    }
    break;

  case MS_TOKEN_FUNCTION_AREA: token = AREA; break;
  case MS_TOKEN_FUNCTION_LENGTH: token = LENGTH; break;
  case MS_TOKEN_FUNCTION_TOSTRING: token = TOSTRING; break;
  case MS_TOKEN_FUNCTION_COMMIFY: token = COMMIFY; break;
  case MS_TOKEN_FUNCTION_UPPER: token = UPPER; break;
  case MS_TOKEN_FUNCTION_LOWER: token = LOWER; break;
  case MS_TOKEN_FUNCTION_INITCAP: token = INITCAP; break;
  case MS_TOKEN_FUNCTION_FIRSTCAP: token = FIRSTCAP; break;
  case MS_TOKEN_FUNCTION_ROUND: token = ROUND; break;

  case MS_TOKEN_FUNCTION_BUFFER: token = YYBUFFER; break;
  case MS_TOKEN_FUNCTION_DIFFERENCE: token = DIFFERENCE; break;
  case MS_TOKEN_FUNCTION_SIMPLIFY: token = SIMPLIFY; break;
  case MS_TOKEN_FUNCTION_SIMPLIFYPT: token = SIMPLIFYPT; break;
  case MS_TOKEN_FUNCTION_GENERALIZE: token = GENERALIZE; break;
  case MS_TOKEN_FUNCTION_SMOOTHSIA: token = SMOOTHSIA; break;
  case MS_TOKEN_FUNCTION_JAVASCRIPT: token = JAVASCRIPT; break;

  default:
    break;
  }

  p->expr->curtoken = p->expr->curtoken->next; /* re-position */ 
  return(token);
}

int yyerror(parseObj *p, const char *s) {
  msSetError(MS_PARSEERR, "%s", "yyparse()", s);
  return(0);
}

