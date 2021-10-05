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
     CENTERLINE = 296,
     SMOOTHSIA = 297,
     GENERALIZE = 298,
     SIMPLIFYPT = 299,
     SIMPLIFY = 300,
     DENSIFY = 301,
     DIFFERENCE = 302,
     OUTER = 303,
     INNER = 304,
     YYBUFFER = 305,
     NEG = 306
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
#define CENTERLINE 296
#define SMOOTHSIA 297
#define GENERALIZE 298
#define SIMPLIFYPT 299
#define SIMPLIFY 300
#define DENSIFY 301
#define DIFFERENCE 302
#define OUTER 303
#define INNER 304
#define YYBUFFER 305
#define NEG 306




/* Copy the first part of user declarations.  */
#line 5 "mapparser.y"

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
#line 30 "mapparser.y"
{
  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
  shapeObj *shpval;
}
/* Line 193 of yacc.c.  */
#line 224 "/Users/sdlime/mapserver/sdlime/mapserver/mapparser.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 237 "/Users/sdlime/mapserver/sdlime/mapserver/mapparser.c"

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
#define YYFINAL  86
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   556

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  61
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  7
/* YYNRULES -- Number of rules.  */
#define YYNRULES  102
/* YYNRULES -- Number of states.  */
#define YYNSTATES  306

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   306

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    55,     2,     2,
      58,    59,    53,    51,    60,    52,     2,    54,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    57,     2,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    56
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
     292,   295,   299,   304,   309,   316,   321,   323,   327,   334,
     339,   344,   349,   356,   363,   370,   377,   384,   389,   396,
     405,   416,   423,   425,   429,   433,   440,   445,   450,   455,
     460,   465,   467
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      62,     0,    -1,    -1,    63,    -1,    64,    -1,    66,    -1,
      65,    -1,     3,    -1,    58,    63,    59,    -1,    63,    19,
      63,    -1,    63,     8,    63,    -1,    63,     9,    63,    -1,
      63,     8,    64,    -1,    63,     9,    64,    -1,    64,     8,
      63,    -1,    64,     9,    63,    -1,    64,     8,    64,    -1,
      64,     9,    64,    -1,    10,    63,    -1,    10,    64,    -1,
      66,    20,    66,    -1,    66,    11,    66,    -1,    64,    19,
      64,    -1,    64,    18,    64,    -1,    64,    16,    64,    -1,
      64,    17,    64,    -1,    64,    14,    64,    -1,    64,    15,
      64,    -1,    66,    19,    66,    -1,    66,    18,    66,    -1,
      66,    16,    66,    -1,    66,    17,    66,    -1,    66,    14,
      66,    -1,    66,    15,    66,    -1,    67,    19,    67,    -1,
      67,    18,    67,    -1,    67,    16,    67,    -1,    67,    17,
      67,    -1,    67,    14,    67,    -1,    67,    15,    67,    -1,
      66,    13,    66,    -1,    64,    13,    66,    -1,    64,    12,
      64,    -1,    66,    12,    66,    -1,    67,    12,    67,    -1,
      65,    19,    65,    -1,    23,    58,    65,    60,    65,    59,
      -1,    30,    58,    65,    60,    65,    59,    -1,    65,    30,
      65,    -1,    29,    58,    65,    60,    65,    59,    -1,    65,
      29,    65,    -1,    28,    58,    65,    60,    65,    59,    -1,
      65,    28,    65,    -1,    27,    58,    65,    60,    65,    59,
      -1,    65,    27,    65,    -1,    26,    58,    65,    60,    65,
      59,    -1,    65,    26,    65,    -1,    25,    58,    65,    60,
      65,    59,    -1,    65,    25,    65,    -1,    24,    58,    65,
      60,    65,    59,    -1,    65,    24,    65,    -1,    21,    58,
      65,    60,    65,    60,    64,    59,    -1,    22,    58,    65,
      60,    65,    60,    64,    59,    -1,     4,    -1,    58,    64,
      59,    -1,    64,    51,    64,    -1,    64,    52,    64,    -1,
      64,    53,    64,    -1,    64,    55,    64,    -1,    64,    54,
      64,    -1,    52,    64,    -1,    64,    57,    64,    -1,    33,
      58,    66,    59,    -1,    34,    58,    65,    59,    -1,    31,
      58,    64,    60,    64,    59,    -1,    31,    58,    64,    59,
      -1,     7,    -1,    58,    65,    59,    -1,    50,    58,    65,
      60,    64,    59,    -1,    49,    58,    65,    59,    -1,    48,
      58,    65,    59,    -1,    41,    58,    65,    59,    -1,    47,
      58,    65,    60,    65,    59,    -1,    46,    58,    65,    60,
      64,    59,    -1,    45,    58,    65,    60,    64,    59,    -1,
      44,    58,    65,    60,    64,    59,    -1,    43,    58,    65,
      60,    64,    59,    -1,    42,    58,    65,    59,    -1,    42,
      58,    65,    60,    64,    59,    -1,    42,    58,    65,    60,
      64,    60,    64,    59,    -1,    42,    58,    65,    60,    64,
      60,    64,    60,    66,    59,    -1,    40,    58,    65,    60,
      66,    59,    -1,     5,    -1,    58,    66,    59,    -1,    66,
      51,    66,    -1,    39,    58,    64,    60,    66,    59,    -1,
      32,    58,    66,    59,    -1,    38,    58,    66,    59,    -1,
      37,    58,    66,    59,    -1,    36,    58,    66,    59,    -1,
      35,    58,    66,    59,    -1,     6,    -1,    58,    67,    59,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    68,    68,    69,    82,    96,   110,   120,   121,   122,
     126,   134,   143,   151,   160,   168,   177,   185,   194,   195,
     196,   216,   236,   242,   248,   254,   260,   266,   272,   280,
     288,   296,   304,   312,   320,   326,   332,   338,   344,   350,
     356,   377,   397,   403,   411,   417,   428,   439,   450,   461,
     472,   483,   494,   505,   516,   527,   538,   549,   560,   571,
     582,   593,   603,   615,   616,   617,   618,   619,   620,   621,
     628,   629,   630,   631,   639,   640,   643,   644,   645,   655,
     665,   675,   685,   695,   705,   715,   725,   735,   745,   755,
     765,   776,   794,   795,   796,   800,   805,   809,   813,   817,
     821,   827,   828
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
  "TOSTRING", "JAVASCRIPT", "CENTERLINE", "SMOOTHSIA", "GENERALIZE",
  "SIMPLIFYPT", "SIMPLIFY", "DENSIFY", "DIFFERENCE", "OUTER", "INNER",
  "YYBUFFER", "'+'", "'-'", "'*'", "'/'", "'%'", "NEG", "'^'", "'('",
  "')'", "','", "$accept", "input", "logical_exp", "math_exp", "shape_exp",
  "string_exp", "time_exp", 0
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
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,    43,    45,    42,    47,    37,   306,    94,    40,    41,
      44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    61,    62,    62,    62,    62,    62,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    67,    67
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
       2,     3,     4,     4,     6,     4,     1,     3,     6,     4,
       4,     4,     6,     6,     6,     6,     6,     4,     6,     8,
      10,     6,     1,     3,     3,     6,     4,     4,     4,     4,
       4,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     7,    63,    92,   101,    76,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       3,     4,     6,     5,     0,    18,    19,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      70,     0,     0,     0,     0,     0,     1,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,    64,    77,    93,   102,
      10,    12,    11,    13,     9,     0,    14,    16,    15,    17,
      42,    41,    26,    27,    24,    25,    23,    22,    65,    66,
      67,    69,    68,    71,    45,    60,    58,    56,    54,    52,
      50,    48,    21,    43,    40,    32,    33,    30,    31,    29,
      28,    20,    94,     0,    44,    38,    39,    36,    37,    35,
      34,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    75,     0,     0,    96,    72,    73,   100,    99,
      98,    97,     0,     0,    81,    87,     0,     0,     0,     0,
       0,     0,    80,    79,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    46,    59,
      57,    55,    53,    51,    49,    47,    74,    95,    91,    88,
       0,    86,    85,    84,    83,    82,    78,     0,     0,     0,
      61,    62,    89,     0,     0,    90
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    39,    40,    41,    47,    48,    44
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -55
static const yytype_int16 yypact[] =
{
     332,   -55,   -55,   -55,   -55,   -55,   332,   -54,   -52,   -28,
     -21,   -14,    13,    18,    19,    20,    21,    22,    23,    67,
      68,    87,    88,    89,    90,    91,   102,   103,   104,   106,
     107,   108,   109,   110,   124,   125,   135,    -2,   332,    58,
       1,   292,   426,   398,   537,    40,   389,   426,   398,   223,
     223,   223,   223,   223,   223,   223,   223,   223,   223,    -2,
      16,    16,   223,    16,    16,    16,    16,    -2,   223,   223,
     223,   223,   223,   223,   223,   223,   223,   223,   223,    -2,
      84,     3,   172,   400,   380,   232,   -55,   332,   332,   332,
     332,   332,    -2,    16,    -2,    -2,    -2,    -2,    -2,    -2,
      -2,    -2,    -2,    -2,    -2,    -2,   223,   223,   223,   223,
     223,   223,   223,   223,    16,    16,    16,    16,    16,    16,
      16,    16,    16,    16,    16,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   223,   134,   143,   144,   145,   146,   156,   157,
     158,   159,   160,    83,    16,   -44,   -43,    14,   -34,   -33,
     -32,   -23,   409,   161,   163,   -36,   168,   179,   185,   192,
     193,   173,   200,   201,   419,   -55,   -55,   -55,   -55,   -55,
      -6,   183,    40,   389,   -55,   292,    -6,   183,    40,   389,
      37,   209,    37,    37,    37,    37,    37,    37,    15,    15,
      84,    84,    84,    84,   -55,   -55,   -55,   -55,   -55,   -55,
     -55,   -55,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   -55,    -1,   -55,   -55,   -55,   -55,   -55,   -55,
     -55,   215,   223,   223,   223,   223,   223,   223,   223,   223,
     223,   223,   -55,    -2,   -17,   -55,   -55,   -55,   -55,   -55,
     -55,   -55,    16,    16,   -55,   -55,    -2,    -2,    -2,    -2,
      -2,   223,   -55,   -55,    -2,   216,   217,   218,   220,   221,
     224,   234,   237,   238,   240,   243,   428,   -16,   -12,   235,
     437,   446,   455,   464,   258,   473,    -2,    -2,   -55,   -55,
     -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,
      -2,   -55,   -55,   -55,   -55,   -55,   -55,   482,   491,   261,
     -55,   -55,   -55,    16,   -10,   -55
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -55,   -55,    -5,     8,   101,     0,     2
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      43,    45,     2,    88,    49,     4,    50,   124,   124,    87,
      88,    87,    88,    89,    46,   235,   236,   124,   124,   124,
      89,     3,    89,   245,   246,   238,   239,   240,   124,    17,
      51,    19,    20,    81,   124,   124,   241,    52,    84,   124,
      85,   124,   168,   287,    53,    80,    82,   288,    18,   305,
      37,    21,    22,    23,    24,    25,    79,   213,    86,    89,
     145,   146,   165,   148,   149,   150,   151,   143,   102,   103,
     104,    54,   105,   237,   144,   152,    55,    56,    57,    58,
      59,    60,   170,   172,   174,   176,   178,   164,   100,   101,
     102,   103,   104,   181,   105,   171,   173,   175,   177,   179,
     180,    42,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   202,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,    61,    62,   214,   215,   216,
     217,   218,   219,   220,   100,   101,   102,   103,   104,    83,
     105,   105,   232,   233,   234,    63,    64,    65,    66,    67,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
      68,    69,    70,   147,    71,    72,    73,    74,    75,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      90,    91,    76,    77,    92,    93,    94,    95,    96,    97,
      98,    99,    91,    78,   222,    92,    93,    94,    95,    96,
      97,    98,    99,   223,   224,   225,   226,   194,   195,   196,
     197,   198,   199,   200,   201,   255,   227,   228,   229,   230,
     231,   243,   244,   100,   101,   102,   103,   104,   247,   105,
       5,   166,   252,   221,   100,   101,   102,   103,   104,   248,
     105,   266,   267,   268,   125,   249,   126,   127,   128,   129,
     130,   131,   250,   251,   269,   270,   271,   272,   273,   253,
     124,   254,   275,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,   167,   169,     0,   276,   277,   278,
     279,   132,     0,   280,   297,   298,   100,   101,   102,   103,
     104,   169,   105,   281,   289,   290,   282,   283,   299,   284,
      90,    91,   285,   304,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   295,   105,     0,
     302,   303,     0,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,     0,     0,     1,     2,     3,     4,     5,
       0,     0,     6,   100,   101,   102,   103,   104,     0,   105,
       0,     0,   274,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,     0,    37,     0,     0,     0,     0,     0,
      38,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,    92,    93,    94,    95,    96,    97,    98,    99,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   106,
       0,     0,     0,     0,   107,   108,   109,   110,   111,   112,
     113,   124,     0,     0,     0,     0,     0,     0,     0,   168,
     100,   101,   102,   103,   104,   106,   105,     0,     0,   124,
     107,   108,   109,   110,   111,   112,   113,     0,     0,   167,
     100,   101,   102,   103,   104,     0,   105,     0,     0,   242,
     100,   101,   102,   103,   104,     0,   105,     0,   166,   100,
     101,   102,   103,   104,     0,   105,     0,   286,   100,   101,
     102,   103,   104,     0,   105,     0,   291,   100,   101,   102,
     103,   104,     0,   105,     0,   292,   100,   101,   102,   103,
     104,     0,   105,     0,   293,   100,   101,   102,   103,   104,
       0,   105,     0,   294,   100,   101,   102,   103,   104,     0,
     105,     0,   296,   100,   101,   102,   103,   104,     0,   105,
       0,   300,   100,   101,   102,   103,   104,     0,   105,   125,
     301,   126,   127,   128,   129,   130,   131
};

static const yytype_int16 yycheck[] =
{
       0,     6,     4,     9,    58,     6,    58,    51,    51,     8,
       9,     8,     9,    19,     6,    59,    59,    51,    51,    51,
      19,     5,    19,    59,    60,    59,    59,    59,    51,    31,
      58,    33,    34,    38,    51,    51,    59,    58,    38,    51,
      38,    51,    59,    59,    58,    37,    38,    59,    32,    59,
      52,    35,    36,    37,    38,    39,    58,    58,     0,    19,
      60,    61,    59,    63,    64,    65,    66,    59,    53,    54,
      55,    58,    57,    59,    58,    67,    58,    58,    58,    58,
      58,    58,    87,    88,    89,    90,    91,    79,    51,    52,
      53,    54,    55,    93,    57,    87,    88,    89,    90,    91,
      92,     0,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,    58,    58,   125,   126,   127,
     128,   129,   130,   131,    51,    52,    53,    54,    55,    38,
      57,    57,    59,    60,   144,    58,    58,    58,    58,    58,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      58,    58,    58,    62,    58,    58,    58,    58,    58,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
       8,     9,    58,    58,    12,    13,    14,    15,    16,    17,
      18,    19,     9,    58,    60,    12,    13,    14,    15,    16,
      17,    18,    19,    60,    60,    60,    60,   106,   107,   108,
     109,   110,   111,   112,   113,   213,    60,    60,    60,    60,
      60,    60,    59,    51,    52,    53,    54,    55,    60,    57,
       7,    59,    59,   132,    51,    52,    53,    54,    55,    60,
      57,   233,   242,   243,    12,    60,    14,    15,    16,    17,
      18,    19,    60,    60,   246,   247,   248,   249,   250,    59,
      51,    60,   254,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    59,    59,    -1,    60,    60,    59,
      59,    58,    -1,    59,   276,   277,    51,    52,    53,    54,
      55,    59,    57,    59,    59,    60,    59,    59,   290,    59,
       8,     9,    59,   303,    12,    13,    14,    15,    16,    17,
      18,    19,    51,    52,    53,    54,    55,    59,    57,    -1,
      59,    60,    -1,   222,   223,   224,   225,   226,   227,   228,
     229,   230,   231,    -1,    -1,     3,     4,     5,     6,     7,
      -1,    -1,    10,    51,    52,    53,    54,    55,    -1,    57,
      -1,    -1,   251,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    -1,    52,    -1,    -1,    -1,    -1,    -1,
      58,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    12,    13,    14,    15,    16,    17,    18,    19,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    19,
      -1,    -1,    -1,    -1,    24,    25,    26,    27,    28,    29,
      30,    51,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,
      51,    52,    53,    54,    55,    19,    57,    -1,    -1,    51,
      24,    25,    26,    27,    28,    29,    30,    -1,    -1,    59,
      51,    52,    53,    54,    55,    -1,    57,    -1,    -1,    60,
      51,    52,    53,    54,    55,    -1,    57,    -1,    59,    51,
      52,    53,    54,    55,    -1,    57,    -1,    59,    51,    52,
      53,    54,    55,    -1,    57,    -1,    59,    51,    52,    53,
      54,    55,    -1,    57,    -1,    59,    51,    52,    53,    54,
      55,    -1,    57,    -1,    59,    51,    52,    53,    54,    55,
      -1,    57,    -1,    59,    51,    52,    53,    54,    55,    -1,
      57,    -1,    59,    51,    52,    53,    54,    55,    -1,    57,
      -1,    59,    51,    52,    53,    54,    55,    -1,    57,    12,
      59,    14,    15,    16,    17,    18,    19
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,    10,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    52,    58,    62,
      63,    64,    65,    66,    67,    63,    64,    65,    66,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      64,    63,    64,    65,    66,    67,     0,     8,     9,    19,
       8,     9,    12,    13,    14,    15,    16,    17,    18,    19,
      51,    52,    53,    54,    55,    57,    19,    24,    25,    26,
      27,    28,    29,    30,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    51,    12,    14,    15,    16,    17,
      18,    19,    58,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    64,    58,    66,    66,    65,    66,    66,
      66,    66,    64,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    64,    59,    59,    59,    59,    59,
      63,    64,    63,    64,    63,    64,    63,    64,    63,    64,
      64,    66,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    65,    65,    65,    65,    65,    65,
      65,    65,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    58,    67,    67,    67,    67,    67,    67,
      67,    65,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    59,    60,    66,    59,    59,    59,    59,    59,
      59,    59,    60,    60,    59,    59,    60,    60,    60,    60,
      60,    60,    59,    59,    60,    67,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    64,    66,    66,    64,
      64,    64,    64,    64,    65,    64,    60,    60,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      60,    59,    59,    59,    59,    59,    59,    64,    64,    64,
      59,    59,    59,    60,    66,    59
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
#line 69 "mapparser.y"
    {
    switch(p->type) {
    case(MS_PARSE_TYPE_BOOLEAN):
      p->result.intval = (yyvsp[(1) - (1)].intval); 
      break;
    case(MS_PARSE_TYPE_STRING):
      if((yyvsp[(1) - (1)].intval)) 
        p->result.strval = msStrdup("true");
      else
        p->result.strval = msStrdup("false");
      break;
    }
  }
    break;

  case 4:
#line 82 "mapparser.y"
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
#line 96 "mapparser.y"
    {
    switch(p->type) {
    case(MS_PARSE_TYPE_BOOLEAN):
      if((yyvsp[(1) - (1)].strval)) /* string is not NULL */
        p->result.intval = MS_TRUE;
      else
        p->result.intval = MS_FALSE;
      break;
    case(MS_PARSE_TYPE_SLD):
    case(MS_PARSE_TYPE_STRING):
      p->result.strval = (yyvsp[(1) - (1)].strval); // msStrdup($1);
      break;
    }
  }
    break;

  case 6:
#line 110 "mapparser.y"
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
#line 121 "mapparser.y"
    { (yyval.intval) = (yyvsp[(2) - (3)].intval); }
    break;

  case 9:
#line 122 "mapparser.y"
    {
    (yyval.intval) = MS_FALSE;
    if((yyvsp[(1) - (3)].intval) == (yyvsp[(3) - (3)].intval)) (yyval.intval) = MS_TRUE;
  }
    break;

  case 10:
#line 126 "mapparser.y"
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
#line 134 "mapparser.y"
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
#line 143 "mapparser.y"
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
#line 151 "mapparser.y"
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
#line 160 "mapparser.y"
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
#line 168 "mapparser.y"
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
#line 177 "mapparser.y"
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
#line 185 "mapparser.y"
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
#line 194 "mapparser.y"
    { (yyval.intval) = !(yyvsp[(2) - (2)].intval); }
    break;

  case 19:
#line 195 "mapparser.y"
    { (yyval.intval) = !(yyvsp[(2) - (2)].dblval); }
    break;

  case 20:
#line 196 "mapparser.y"
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
#line 216 "mapparser.y"
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
#line 236 "mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) == (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 23:
#line 242 "mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) != (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 24:
#line 248 "mapparser.y"
    {    
    if((yyvsp[(1) - (3)].dblval) > (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 25:
#line 254 "mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) < (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 26:
#line 260 "mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) >= (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 27:
#line 266 "mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) <= (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 28:
#line 272 "mapparser.y"
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
#line 280 "mapparser.y"
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
#line 288 "mapparser.y"
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
#line 296 "mapparser.y"
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
#line 304 "mapparser.y"
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
#line 312 "mapparser.y"
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
#line 320 "mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 35:
#line 326 "mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 36:
#line 332 "mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) > 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 37:
#line 338 "mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) < 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 38:
#line 344 "mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) >= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 39:
#line 350 "mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) <= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 40:
#line 356 "mapparser.y"
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
#line 377 "mapparser.y"
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
#line 397 "mapparser.y"
    {
    if((yyvsp[(1) - (3)].dblval) == (yyvsp[(3) - (3)].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 43:
#line 403 "mapparser.y"
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
#line 411 "mapparser.y"
    {
    if(msTimeCompare(&((yyvsp[(1) - (3)].tmval)), &((yyvsp[(3) - (3)].tmval))) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
    break;

  case 45:
#line 417 "mapparser.y"
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
#line 428 "mapparser.y"
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
#line 439 "mapparser.y"
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
#line 450 "mapparser.y"
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
#line 461 "mapparser.y"
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
#line 472 "mapparser.y"
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
#line 483 "mapparser.y"
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
#line 494 "mapparser.y"
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
#line 505 "mapparser.y"
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
#line 516 "mapparser.y"
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
#line 527 "mapparser.y"
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
#line 538 "mapparser.y"
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
#line 549 "mapparser.y"
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
#line 560 "mapparser.y"
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
#line 571 "mapparser.y"
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
#line 582 "mapparser.y"
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
#line 593 "mapparser.y"
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
#line 603 "mapparser.y"
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
#line 616 "mapparser.y"
    { (yyval.dblval) = (yyvsp[(2) - (3)].dblval); }
    break;

  case 65:
#line 617 "mapparser.y"
    { (yyval.dblval) = (yyvsp[(1) - (3)].dblval) + (yyvsp[(3) - (3)].dblval); }
    break;

  case 66:
#line 618 "mapparser.y"
    { (yyval.dblval) = (yyvsp[(1) - (3)].dblval) - (yyvsp[(3) - (3)].dblval); }
    break;

  case 67:
#line 619 "mapparser.y"
    { (yyval.dblval) = (yyvsp[(1) - (3)].dblval) * (yyvsp[(3) - (3)].dblval); }
    break;

  case 68:
#line 620 "mapparser.y"
    { (yyval.dblval) = (int)(yyvsp[(1) - (3)].dblval) % (int)(yyvsp[(3) - (3)].dblval); }
    break;

  case 69:
#line 621 "mapparser.y"
    {
    if((yyvsp[(3) - (3)].dblval) == 0.0) {
      yyerror(p, "Division by zero.");
      return(-1);
    } else
      (yyval.dblval) = (yyvsp[(1) - (3)].dblval) / (yyvsp[(3) - (3)].dblval); 
  }
    break;

  case 70:
#line 628 "mapparser.y"
    { (yyval.dblval) = (yyvsp[(2) - (2)].dblval); }
    break;

  case 71:
#line 629 "mapparser.y"
    { (yyval.dblval) = pow((yyvsp[(1) - (3)].dblval), (yyvsp[(3) - (3)].dblval)); }
    break;

  case 72:
#line 630 "mapparser.y"
    { (yyval.dblval) = strlen((yyvsp[(3) - (4)].strval)); }
    break;

  case 73:
#line 631 "mapparser.y"
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
#line 639 "mapparser.y"
    { (yyval.dblval) = (MS_NINT((yyvsp[(3) - (6)].dblval)/(yyvsp[(5) - (6)].dblval)))*(yyvsp[(5) - (6)].dblval); }
    break;

  case 75:
#line 640 "mapparser.y"
    { (yyval.dblval) = (MS_NINT((yyvsp[(3) - (4)].dblval))); }
    break;

  case 77:
#line 644 "mapparser.y"
    { (yyval.shpval) = (yyvsp[(2) - (3)].shpval); }
    break;

  case 78:
#line 645 "mapparser.y"
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

  case 79:
#line 655 "mapparser.y"
    {
    shapeObj *s;
    s = msRings2Shape((yyvsp[(3) - (4)].shpval), MS_FALSE);
    if(!s) {
      yyerror(p, "Executing inner failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 80:
#line 665 "mapparser.y"
    {
    shapeObj *s;
    s = msRings2Shape((yyvsp[(3) - (4)].shpval), MS_TRUE);
    if(!s) {
      yyerror(p, "Executing outer failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 81:
#line 675 "mapparser.y"
    {
    shapeObj *s;
    s = msGEOSCenterline((yyvsp[(3) - (4)].shpval));
    if(!s) {
      yyerror(p, "Executing centerline failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 82:
#line 685 "mapparser.y"
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

  case 83:
#line 695 "mapparser.y"
    {
    shapeObj *s;
    s = msDensify((yyvsp[(3) - (6)].shpval), (yyvsp[(5) - (6)].dblval));
    if(!s) {
      yyerror(p, "Executing densify failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
    break;

  case 84:
#line 705 "mapparser.y"
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

  case 85:
#line 715 "mapparser.y"
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

  case 86:
#line 725 "mapparser.y"
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

  case 87:
#line 735 "mapparser.y"
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

  case 88:
#line 745 "mapparser.y"
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

  case 89:
#line 755 "mapparser.y"
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

  case 90:
#line 765 "mapparser.y"
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

  case 91:
#line 776 "mapparser.y"
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

  case 93:
#line 795 "mapparser.y"
    { (yyval.strval) = (yyvsp[(2) - (3)].strval); }
    break;

  case 94:
#line 796 "mapparser.y"
    { 
    (yyval.strval) = (char *)malloc(strlen((yyvsp[(1) - (3)].strval)) + strlen((yyvsp[(3) - (3)].strval)) + 1);
    sprintf((yyval.strval), "%s%s", (yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].strval)); free((yyvsp[(1) - (3)].strval)); free((yyvsp[(3) - (3)].strval)); 
  }
    break;

  case 95:
#line 800 "mapparser.y"
    {
    (yyval.strval) = (char *) malloc(strlen((yyvsp[(5) - (6)].strval)) + 64); /* Plenty big? Should use snprintf below... */
    sprintf((yyval.strval), (yyvsp[(5) - (6)].strval), (yyvsp[(3) - (6)].dblval));
    free((yyvsp[(5) - (6)].strval));
  }
    break;

  case 96:
#line 805 "mapparser.y"
    {  
    (yyvsp[(3) - (4)].strval) = msCommifyString((yyvsp[(3) - (4)].strval)); 
    (yyval.strval) = (yyvsp[(3) - (4)].strval); 
  }
    break;

  case 97:
#line 809 "mapparser.y"
    {  
    msStringToUpper((yyvsp[(3) - (4)].strval)); 
    (yyval.strval) = (yyvsp[(3) - (4)].strval); 
  }
    break;

  case 98:
#line 813 "mapparser.y"
    {  
    msStringToLower((yyvsp[(3) - (4)].strval)); 
    (yyval.strval) = (yyvsp[(3) - (4)].strval); 
  }
    break;

  case 99:
#line 817 "mapparser.y"
    {  
    msStringInitCap((yyvsp[(3) - (4)].strval)); 
    (yyval.strval) = (yyvsp[(3) - (4)].strval); 
  }
    break;

  case 100:
#line 821 "mapparser.y"
    {  
    msStringFirstCap((yyvsp[(3) - (4)].strval)); 
    (yyval.strval) = (yyvsp[(3) - (4)].strval); 
  }
    break;

  case 102:
#line 828 "mapparser.y"
    { (yyval.tmval) = (yyvsp[(2) - (3)].tmval); }
    break;


/* Line 1267 of yacc.c.  */
#line 2853 "/Users/sdlime/mapserver/sdlime/mapserver/mapparser.c"
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


#line 831 "mapparser.y"


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
    if (p->type == MS_PARSE_TYPE_SLD)
    {
      token = STRING;
      if (*(p->expr->curtoken->tokenval.strval))
      {
        (*lvalp).strval = msStrdup("<![CDATA[");
        (*lvalp).strval = msStringConcatenate((*lvalp).strval,p->expr->curtoken->tokenval.strval);
        (*lvalp).strval = msStringConcatenate((*lvalp).strval,"]]>\n");
      }
      else
      {
        (*lvalp).strval = msStrdup(p->expr->curtoken->tokenval.strval);
      }
    }
    else
    {
      token = STRING;
      (*lvalp).strval = msStrdup(p->expr->curtoken->tokenval.strval);    
    }
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

  case MS_TOKEN_COMPARISON_IN: token = IN; break;

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
    if (p->type == MS_PARSE_TYPE_SLD)
    {
      token = STRING;
      (*lvalp).strval = msStrdup("<ogc:PropertyName>");
      (*lvalp).strval = msStringConcatenate((*lvalp).strval,p->expr->curtoken->tokenval.bindval.item);
      (*lvalp).strval = msStringConcatenate((*lvalp).strval,"</ogc:PropertyName>\n");
    }
    else
    {
      token = NUMBER;
      (*lvalp).dblval = atof(p->shape->values[p->expr->curtoken->tokenval.bindval.index]);
    }
    break;
  case MS_TOKEN_BINDING_STRING:
    if (p->type == MS_PARSE_TYPE_SLD)
    {
      token = STRING;
      (*lvalp).strval = msStrdup("<ogc:PropertyName>");
      (*lvalp).strval = msStringConcatenate((*lvalp).strval,p->expr->curtoken->tokenval.bindval.item);
      (*lvalp).strval = msStringConcatenate((*lvalp).strval,"</ogc:PropertyName>\n");
    }
    else
    {
      token = STRING;
      (*lvalp).strval = msStrdup(p->shape->values[p->expr->curtoken->tokenval.bindval.index]);
    }
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
  case MS_TOKEN_FUNCTION_CENTERLINE: token = CENTERLINE; break;
  case MS_TOKEN_FUNCTION_DENSIFY: token = DENSIFY; break;
  case MS_TOKEN_FUNCTION_INNER: token = INNER; break;
  case MS_TOKEN_FUNCTION_OUTER: token = OUTER; break;
  case MS_TOKEN_FUNCTION_JAVASCRIPT: token = JAVASCRIPT; break;

  default:
    break;
  }

  p->expr->curtoken = p->expr->curtoken->next; /* re-position */ 
  return(token);
}

int yyerror(parseObj *p, const char *s) {
  (void)p;
  msSetError(MS_PARSEERR, "%s", "yyparse()", s);
  return(0);
}

