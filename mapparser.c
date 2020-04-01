/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 5 "mapparser.y" /* yacc.c:339  */

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

#line 84 "mapparser.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "mapparser.h".  */
#ifndef YY_YY_MAPPARSER_H_INCLUDED
# define YY_YY_MAPPARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    BOOLEAN = 258,
    NUMBER = 259,
    STRING = 260,
    TIME = 261,
    SHAPE = 262,
    OR = 263,
    AND = 264,
    NOT = 265,
    RE = 266,
    EQ = 267,
    NE = 268,
    LT = 269,
    GT = 270,
    LE = 271,
    GE = 272,
    IN = 273,
    IEQ = 274,
    IRE = 275,
    INTERSECTS = 276,
    DISJOINT = 277,
    TOUCHES = 278,
    OVERLAPS = 279,
    CROSSES = 280,
    WITHIN = 281,
    CONTAINS = 282,
    EQUALS = 283,
    BEYOND = 284,
    DWITHIN = 285,
    AREA = 286,
    LENGTH = 287,
    COMMIFY = 288,
    ROUND = 289,
    UPPER = 290,
    LOWER = 291,
    INITCAP = 292,
    FIRSTCAP = 293,
    TOSTRING = 294,
    YYBUFFER = 295,
    DIFFERENCE = 296,
    SIMPLIFY = 297,
    SIMPLIFYPT = 298,
    GENERALIZE = 299,
    SMOOTHSIA = 300,
    JAVASCRIPT = 301,
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
#define RE 266
#define EQ 267
#define NE 268
#define LT 269
#define GT 270
#define LE 271
#define GE 272
#define IN 273
#define IEQ 274
#define IRE 275
#define INTERSECTS 276
#define DISJOINT 277
#define TOUCHES 278
#define OVERLAPS 279
#define CROSSES 280
#define WITHIN 281
#define CONTAINS 282
#define EQUALS 283
#define BEYOND 284
#define DWITHIN 285
#define AREA 286
#define LENGTH 287
#define COMMIFY 288
#define ROUND 289
#define UPPER 290
#define LOWER 291
#define INITCAP 292
#define FIRSTCAP 293
#define TOSTRING 294
#define YYBUFFER 295
#define DIFFERENCE 296
#define SIMPLIFY 297
#define SIMPLIFYPT 298
#define GENERALIZE 299
#define SMOOTHSIA 300
#define JAVASCRIPT 301
#define NEG 302

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 30 "mapparser.y" /* yacc.c:355  */

  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
  shapeObj *shpval;

#line 226 "mapparser.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (parseObj *p);

#endif /* !YY_YY_MAPPARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 240 "mapparser.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
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
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  78
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   511

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  57
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  7
/* YYNRULES -- Number of rules.  */
#define YYNRULES  98
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  288

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   302

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
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
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
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
     665,   675,   685,   695,   705,   715,   725,   736,   754,   755,
     756,   760,   765,   769,   773,   777,   781,   787,   788
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "BOOLEAN", "NUMBER", "STRING", "TIME",
  "SHAPE", "OR", "AND", "NOT", "RE", "EQ", "NE", "LT", "GT", "LE", "GE",
  "IN", "IEQ", "IRE", "INTERSECTS", "DISJOINT", "TOUCHES", "OVERLAPS",
  "CROSSES", "WITHIN", "CONTAINS", "EQUALS", "BEYOND", "DWITHIN", "AREA",
  "LENGTH", "COMMIFY", "ROUND", "UPPER", "LOWER", "INITCAP", "FIRSTCAP",
  "TOSTRING", "YYBUFFER", "DIFFERENCE", "SIMPLIFY", "SIMPLIFYPT",
  "GENERALIZE", "SMOOTHSIA", "JAVASCRIPT", "'+'", "'-'", "'*'", "'/'",
  "'%'", "NEG", "'^'", "'('", "')'", "','", "$accept", "input",
  "logical_exp", "math_exp", "shape_exp", "string_exp", "time_exp", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
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

#define YYPACT_NINF -44

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-44)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     276,   -44,   -44,   -44,   -44,   -44,   276,    -3,    -2,     1,
      12,    19,    32,    33,    34,    35,    37,    47,    48,    49,
      50,    51,    64,    65,    66,    67,    69,    81,    83,   110,
     123,   125,   126,    -1,   276,    45,     6,   325,    58,   244,
     193,    42,   388,    58,   244,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   117,   117,     4,     4,    -1,     4,
       4,     4,     4,    -1,   117,   117,   117,   117,   117,   117,
     117,    -1,    79,     8,   173,   219,   379,     9,   -44,   276,
     276,   276,   276,   276,    -1,    -1,    -1,    -1,    -1,    -1,
       4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   117,
     117,   117,   117,   117,   117,   117,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,    -4,    -4,    -4,
      -4,    -4,    -4,    -4,   117,    16,   137,   155,   176,   183,
     209,   210,   213,   214,   215,   179,     4,   -28,   -11,   -43,
      -9,   118,   129,   178,   361,   216,   222,   231,   232,   233,
      -7,   239,   372,   -44,   -44,   -44,   -44,   -44,    23,   336,
      42,   388,   -44,   325,    23,   336,    42,   388,   458,   458,
     458,   458,   458,   458,   189,   458,    18,    18,    79,    79,
      79,    79,   -44,   -44,   -44,   -44,   -44,   -44,   -44,   -44,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     -44,    -4,   -44,   -44,   -44,   -44,   -44,   -44,   -44,   268,
     117,   117,   117,   117,   117,   117,   117,   117,   117,   117,
     -44,   180,   -44,   -44,   -44,    -1,   -44,   -44,   -44,   -44,
       4,    -1,   117,    -1,    -1,    -1,   -44,    -1,     4,   270,
     273,   274,   277,   280,   291,   292,   301,   302,   275,   313,
     395,   182,   404,   303,   413,   422,   431,    78,   220,   -44,
     -44,   -44,   -44,   -44,   -44,   -44,   -44,    -1,    -1,   -44,
     -44,   -44,   -44,   -44,   -44,   -44,   -44,    -1,   -44,   440,
     449,   119,   -44,   -44,   -44,     4,   221,   -44
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     7,    63,    88,    97,    76,     0,     0,     0,     0,
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
       0,     0,     0,     8,    64,    77,    89,    98,    10,    12,
      11,    13,     9,     0,    14,    16,    15,    17,    22,    23,
      25,    24,    27,    26,    41,    42,    65,    66,    67,    69,
      68,    71,    45,    48,    50,    52,    54,    56,    58,    60,
      20,    28,    29,    31,    30,    33,    32,    40,    43,    21,
      90,     0,    34,    35,    37,    36,    39,    38,    44,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      73,     0,    72,    92,    75,     0,    93,    94,    95,    96,
       0,     0,     0,     0,     0,     0,    83,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    47,
      49,    51,    53,    55,    57,    59,    46,     0,     0,    74,
      91,    78,    79,    80,    81,    82,    84,     0,    87,     0,
       0,     0,    62,    61,    85,     0,     0,    86
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -44,   -44,    -5,    59,   149,     0,   -23
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    35,    36,    37,    43,    44,    40
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
      39,    41,     4,     2,    92,    93,    94,    95,    96,     3,
      97,    77,   224,   225,    79,    80,    79,    80,    81,   116,
      81,   117,   118,   119,   120,   121,   122,   222,   123,    73,
      17,    18,    80,    20,    76,    81,   116,    19,   116,    21,
      22,    23,    24,    25,   223,    78,   226,    33,   236,   237,
     201,    45,    46,    71,    81,    47,   137,   138,   136,   140,
     141,   142,   143,   153,   157,    42,    48,    94,    95,    96,
      98,    97,   210,    49,   158,   160,   162,   164,   166,    99,
     100,   101,   102,   103,   104,   105,    50,    51,    52,    53,
     174,    54,    72,    74,   202,   203,   204,   205,   206,   207,
     208,    55,    56,    57,    58,    59,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   139,    60,    61,
      62,    63,   144,    64,     5,    92,    93,    94,    95,    96,
     152,    97,    97,   276,   277,    65,   221,    66,   159,   161,
     163,   165,   167,   168,   169,   170,   171,   172,   173,    38,
     175,   176,   177,   178,   179,   180,   181,    26,    27,    28,
      29,    30,    31,    32,    67,   116,    92,    93,    94,    95,
      96,   124,    97,   227,   284,   285,   116,    68,   239,    69,
      70,    82,    83,    75,   228,    84,    85,    86,    87,    88,
      89,    90,    91,   211,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   117,   118,   119,   120,   121,
     122,   212,   123,   145,   146,   147,   148,   149,   150,   151,
      92,    93,    94,    95,    96,   116,    97,   116,   154,   116,
     251,    98,   213,   229,   220,   156,   116,   270,   258,   214,
      99,   100,   101,   102,   103,   104,   105,   182,   183,   184,
     185,   186,   187,   188,   189,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   215,   216,   116,   116,   217,
     218,   219,   231,   209,   155,   278,   287,     0,   232,     1,
       2,     3,     4,     5,   250,   286,     6,   233,   234,   235,
     252,   116,   254,   255,   256,   238,   257,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,   155,    33,   157,   279,   280,   259,   260,
      34,   267,   261,    82,    83,   262,   281,    84,    85,    86,
      87,    88,    89,    90,    91,    83,   263,   264,    84,    85,
      86,    87,    88,    89,    90,    91,   265,   266,   272,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   268,
       0,     0,    92,    93,    94,    95,    96,     0,    97,     0,
       0,   253,     0,    92,    93,    94,    95,    96,     0,    97,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,     0,    97,     0,     0,   230,     0,    92,
      93,    94,    95,    96,     0,    97,   116,   154,     0,     0,
       0,     0,     0,     0,   156,    92,    93,    94,    95,    96,
       0,    97,    92,    93,    94,    95,    96,     0,    97,     0,
     269,    92,    93,    94,    95,    96,     0,    97,     0,   271,
      92,    93,    94,    95,    96,     0,    97,     0,   273,    92,
      93,    94,    95,    96,     0,    97,     0,   274,    92,    93,
      94,    95,    96,     0,    97,     0,   275,    92,    93,    94,
      95,    96,     0,    97,     0,   282,    92,    93,    94,    95,
      96,     0,    97,     0,   283,    92,    93,    94,    95,    96,
       0,    97
};

static const yytype_int16 yycheck[] =
{
       0,     6,     6,     4,    47,    48,    49,    50,    51,     5,
      53,    34,    55,    56,     8,     9,     8,     9,    12,    47,
      12,    12,    13,    14,    15,    16,    17,    55,    19,    34,
      31,    32,     9,    34,    34,    12,    47,    33,    47,    35,
      36,    37,    38,    39,    55,     0,    55,    48,    55,    56,
      54,    54,    54,    54,    12,    54,    56,    57,    54,    59,
      60,    61,    62,    55,    55,     6,    54,    49,    50,    51,
      12,    53,    56,    54,    79,    80,    81,    82,    83,    21,
      22,    23,    24,    25,    26,    27,    54,    54,    54,    54,
      90,    54,    33,    34,   117,   118,   119,   120,   121,   122,
     123,    54,    54,    54,    54,    54,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,    58,    54,    54,
      54,    54,    63,    54,     7,    47,    48,    49,    50,    51,
      71,    53,    53,    55,    56,    54,   136,    54,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,     0,
      91,    92,    93,    94,    95,    96,    97,    40,    41,    42,
      43,    44,    45,    46,    54,    47,    47,    48,    49,    50,
      51,    54,    53,    55,    55,    56,    47,    54,   201,    54,
      54,     8,     9,    34,    55,    12,    13,    14,    15,    16,
      17,    18,    19,    56,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    12,    13,    14,    15,    16,
      17,    56,    19,    64,    65,    66,    67,    68,    69,    70,
      47,    48,    49,    50,    51,    47,    53,    47,    55,    47,
     230,    12,    56,    55,    55,    55,    47,    55,   238,    56,
      21,    22,    23,    24,    25,    26,    27,    98,    99,   100,
     101,   102,   103,   104,   105,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    56,    56,    47,    47,    56,
      56,    56,    56,   124,    55,    55,    55,    -1,    56,     3,
       4,     5,     6,     7,   225,   285,    10,    56,    56,    56,
     231,    47,   233,   234,   235,    56,   237,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    55,    48,    55,   267,   268,    55,    55,
      54,    56,    55,     8,     9,    55,   277,    12,    13,    14,
      15,    16,    17,    18,    19,     9,    55,    55,    12,    13,
      14,    15,    16,    17,    18,    19,    55,    55,    55,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,    56,
      -1,    -1,    47,    48,    49,    50,    51,    -1,    53,    -1,
      -1,   232,    -1,    47,    48,    49,    50,    51,    -1,    53,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      12,    13,    14,    15,    16,    17,    18,    19,    47,    48,
      49,    50,    51,    -1,    53,    -1,    -1,    56,    -1,    47,
      48,    49,    50,    51,    -1,    53,    47,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    55,    47,    48,    49,    50,    51,
      -1,    53,    47,    48,    49,    50,    51,    -1,    53,    -1,
      55,    47,    48,    49,    50,    51,    -1,    53,    -1,    55,
      47,    48,    49,    50,    51,    -1,    53,    -1,    55,    47,
      48,    49,    50,    51,    -1,    53,    -1,    55,    47,    48,
      49,    50,    51,    -1,    53,    -1,    55,    47,    48,    49,
      50,    51,    -1,    53,    -1,    55,    47,    48,    49,    50,
      51,    -1,    53,    -1,    55,    47,    48,    49,    50,    51,
      -1,    53
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
       9,    12,     8,     9,    12,    13,    14,    15,    16,    17,
      18,    19,    47,    48,    49,    50,    51,    53,    12,    21,
      22,    23,    24,    25,    26,    27,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    47,    12,    13,    14,
      15,    16,    17,    19,    54,    61,    61,    61,    61,    61,
      61,    61,    61,    61,    61,    61,    54,    62,    62,    60,
      62,    62,    62,    62,    60,    61,    61,    61,    61,    61,
      61,    61,    60,    55,    55,    55,    55,    55,    59,    60,
      59,    60,    59,    60,    59,    60,    59,    60,    60,    60,
      60,    60,    60,    60,    62,    60,    60,    60,    60,    60,
      60,    60,    61,    61,    61,    61,    61,    61,    61,    61,
      62,    62,    62,    62,    62,    62,    62,    62,    62,    62,
      62,    54,    63,    63,    63,    63,    63,    63,    63,    61,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      55,    62,    55,    55,    55,    56,    55,    55,    55,    55,
      56,    56,    56,    56,    56,    56,    55,    56,    56,    63,
      61,    61,    61,    61,    61,    61,    61,    61,    61,    61,
      60,    62,    60,    61,    60,    60,    60,    60,    62,    55,
      55,    55,    55,    55,    55,    55,    55,    56,    56,    55,
      55,    55,    55,    55,    55,    55,    55,    56,    55,    60,
      60,    60,    55,    55,    55,    56,    62,    55
};

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
      60,    60,    60,    60,    60,    60,    61,    61,    61,    61,
      61,    61,    61,    61,    61,    61,    61,    61,    62,    62,
      62,    62,    62,    62,    62,    62,    62,    63,    63
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     1,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     6,     6,     3,     6,
       3,     6,     3,     6,     3,     6,     3,     6,     3,     6,
       3,     8,     8,     1,     3,     3,     3,     3,     3,     3,
       2,     3,     4,     4,     6,     4,     1,     3,     6,     6,
       6,     6,     6,     4,     6,     8,    10,     6,     1,     3,
       3,     6,     4,     4,     4,     4,     4,     1,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (p, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, p); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parseObj *p)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (p);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parseObj *p)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, p);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, parseObj *p)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              , p);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, p); \
} while (0)

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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, parseObj *p)
{
  YYUSE (yyvaluep);
  YYUSE (p);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (parseObj *p)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, p);
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 69 "mapparser.y" /* yacc.c:1646  */
    {
    switch(p->type) {
    case(MS_PARSE_TYPE_BOOLEAN):
      p->result.intval = (yyvsp[0].intval); 
      break;
    case(MS_PARSE_TYPE_STRING):
      if((yyvsp[0].intval)) 
        p->result.strval = msStrdup("true");
      else
        p->result.strval = msStrdup("false");
      break;
    }
  }
#line 1553 "mapparser.c" /* yacc.c:1646  */
    break;

  case 4:
#line 82 "mapparser.y" /* yacc.c:1646  */
    {
    switch(p->type) {
    case(MS_PARSE_TYPE_BOOLEAN):
      if((yyvsp[0].dblval) != 0)
        p->result.intval = MS_TRUE;
      else
        p->result.intval = MS_FALSE;			    
      break;
    case(MS_PARSE_TYPE_STRING):
      p->result.strval = (char *)malloc(64); /* large enough for a double */
      snprintf(p->result.strval, 64, "%g", (yyvsp[0].dblval));
      break;
    }
  }
#line 1572 "mapparser.c" /* yacc.c:1646  */
    break;

  case 5:
#line 96 "mapparser.y" /* yacc.c:1646  */
    {
    switch(p->type) {
    case(MS_PARSE_TYPE_BOOLEAN):
      if((yyvsp[0].strval)) /* string is not NULL */
        p->result.intval = MS_TRUE;
      else
        p->result.intval = MS_FALSE;
      break;
    case(MS_PARSE_TYPE_SLD):
    case(MS_PARSE_TYPE_STRING):
      p->result.strval = (yyvsp[0].strval); // msStrdup($1);
      break;
    }
  }
#line 1591 "mapparser.c" /* yacc.c:1646  */
    break;

  case 6:
#line 110 "mapparser.y" /* yacc.c:1646  */
    {
    switch(p->type) {
    case(MS_PARSE_TYPE_SHAPE):
      p->result.shpval = (yyvsp[0].shpval);
      p->result.shpval->scratch = MS_FALSE;
      break;
    }
  }
#line 1604 "mapparser.c" /* yacc.c:1646  */
    break;

  case 8:
#line 121 "mapparser.y" /* yacc.c:1646  */
    { (yyval.intval) = (yyvsp[-1].intval); }
#line 1610 "mapparser.c" /* yacc.c:1646  */
    break;

  case 9:
#line 122 "mapparser.y" /* yacc.c:1646  */
    {
    (yyval.intval) = MS_FALSE;
    if((yyvsp[-2].intval) == (yyvsp[0].intval)) (yyval.intval) = MS_TRUE;
  }
#line 1619 "mapparser.c" /* yacc.c:1646  */
    break;

  case 10:
#line 126 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].intval) == MS_TRUE)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[0].intval) == MS_TRUE)          
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1632 "mapparser.c" /* yacc.c:1646  */
    break;

  case 11:
#line 134 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].intval) == MS_TRUE) {
      if((yyvsp[0].intval) == MS_TRUE)
        (yyval.intval) = MS_TRUE;
      else
        (yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
#line 1646 "mapparser.c" /* yacc.c:1646  */
    break;

  case 12:
#line 143 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].intval) == MS_TRUE)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[0].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1659 "mapparser.c" /* yacc.c:1646  */
    break;

  case 13:
#line 151 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].intval) == MS_TRUE) {
      if((yyvsp[0].dblval) != 0)
        (yyval.intval) = MS_TRUE;
      else
        (yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
#line 1673 "mapparser.c" /* yacc.c:1646  */
    break;

  case 14:
#line 160 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[0].intval) == MS_TRUE)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1686 "mapparser.c" /* yacc.c:1646  */
    break;

  case 15:
#line 168 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].dblval) != 0) {
      if((yyvsp[0].intval) == MS_TRUE)
        (yyval.intval) = MS_TRUE;
      else
        (yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
#line 1700 "mapparser.c" /* yacc.c:1646  */
    break;

  case 16:
#line 177 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[0].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1713 "mapparser.c" /* yacc.c:1646  */
    break;

  case 17:
#line 185 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].dblval) != 0) {
      if((yyvsp[0].dblval) != 0)
        (yyval.intval) = MS_TRUE;
      else
	(yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
#line 1727 "mapparser.c" /* yacc.c:1646  */
    break;

  case 18:
#line 194 "mapparser.y" /* yacc.c:1646  */
    { (yyval.intval) = !(yyvsp[0].intval); }
#line 1733 "mapparser.c" /* yacc.c:1646  */
    break;

  case 19:
#line 195 "mapparser.y" /* yacc.c:1646  */
    { (yyval.intval) = !(yyvsp[0].dblval); }
#line 1739 "mapparser.c" /* yacc.c:1646  */
    break;

  case 20:
#line 196 "mapparser.y" /* yacc.c:1646  */
    {
    ms_regex_t re;

    if(MS_STRING_IS_NULL_OR_EMPTY((yyvsp[-2].strval)) == MS_TRUE) {
      (yyval.intval) = MS_FALSE;
    } else {
      if(ms_regcomp(&re, (yyvsp[0].strval), MS_REG_EXTENDED|MS_REG_NOSUB) != 0) {      
        (yyval.intval) = MS_FALSE;
      } else {
        if(ms_regexec(&re, (yyvsp[-2].strval), 0, NULL, 0) == 0)
          (yyval.intval) = MS_TRUE;
        else
          (yyval.intval) = MS_FALSE;
        ms_regfree(&re);
      }
    }

    free((yyvsp[-2].strval));
    free((yyvsp[0].strval));
  }
#line 1764 "mapparser.c" /* yacc.c:1646  */
    break;

  case 21:
#line 216 "mapparser.y" /* yacc.c:1646  */
    {
    ms_regex_t re;

    if(MS_STRING_IS_NULL_OR_EMPTY((yyvsp[-2].strval)) == MS_TRUE) {
      (yyval.intval) = MS_FALSE;
    } else {
      if(ms_regcomp(&re, (yyvsp[0].strval), MS_REG_EXTENDED|MS_REG_NOSUB|MS_REG_ICASE) != 0) {
        (yyval.intval) = MS_FALSE;
      } else {
        if(ms_regexec(&re, (yyvsp[-2].strval), 0, NULL, 0) == 0)
          (yyval.intval) = MS_TRUE;
        else
          (yyval.intval) = MS_FALSE;
        ms_regfree(&re);
      }
    }

    free((yyvsp[-2].strval));
    free((yyvsp[0].strval));
  }
#line 1789 "mapparser.c" /* yacc.c:1646  */
    break;

  case 22:
#line 236 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].dblval) == (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1800 "mapparser.c" /* yacc.c:1646  */
    break;

  case 23:
#line 242 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].dblval) != (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1811 "mapparser.c" /* yacc.c:1646  */
    break;

  case 24:
#line 248 "mapparser.y" /* yacc.c:1646  */
    {    
    if((yyvsp[-2].dblval) > (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1822 "mapparser.c" /* yacc.c:1646  */
    break;

  case 25:
#line 254 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].dblval) < (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1833 "mapparser.c" /* yacc.c:1646  */
    break;

  case 26:
#line 260 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].dblval) >= (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1844 "mapparser.c" /* yacc.c:1646  */
    break;

  case 27:
#line 266 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].dblval) <= (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1855 "mapparser.c" /* yacc.c:1646  */
    break;

  case 28:
#line 272 "mapparser.y" /* yacc.c:1646  */
    {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[-2].strval));
    free((yyvsp[0].strval));
  }
#line 1868 "mapparser.c" /* yacc.c:1646  */
    break;

  case 29:
#line 280 "mapparser.y" /* yacc.c:1646  */
    {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[-2].strval));
    free((yyvsp[0].strval));
  }
#line 1881 "mapparser.c" /* yacc.c:1646  */
    break;

  case 30:
#line 288 "mapparser.y" /* yacc.c:1646  */
    {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) > 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[-2].strval));
    free((yyvsp[0].strval));
  }
#line 1894 "mapparser.c" /* yacc.c:1646  */
    break;

  case 31:
#line 296 "mapparser.y" /* yacc.c:1646  */
    {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) < 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[-2].strval));
    free((yyvsp[0].strval));
  }
#line 1907 "mapparser.c" /* yacc.c:1646  */
    break;

  case 32:
#line 304 "mapparser.y" /* yacc.c:1646  */
    {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) >= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[-2].strval));
    free((yyvsp[0].strval));
  }
#line 1920 "mapparser.c" /* yacc.c:1646  */
    break;

  case 33:
#line 312 "mapparser.y" /* yacc.c:1646  */
    {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) <= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[-2].strval));
    free((yyvsp[0].strval));
  }
#line 1933 "mapparser.c" /* yacc.c:1646  */
    break;

  case 34:
#line 320 "mapparser.y" /* yacc.c:1646  */
    {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1944 "mapparser.c" /* yacc.c:1646  */
    break;

  case 35:
#line 326 "mapparser.y" /* yacc.c:1646  */
    {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1955 "mapparser.c" /* yacc.c:1646  */
    break;

  case 36:
#line 332 "mapparser.y" /* yacc.c:1646  */
    {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) > 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1966 "mapparser.c" /* yacc.c:1646  */
    break;

  case 37:
#line 338 "mapparser.y" /* yacc.c:1646  */
    {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) < 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1977 "mapparser.c" /* yacc.c:1646  */
    break;

  case 38:
#line 344 "mapparser.y" /* yacc.c:1646  */
    {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) >= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1988 "mapparser.c" /* yacc.c:1646  */
    break;

  case 39:
#line 350 "mapparser.y" /* yacc.c:1646  */
    {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) <= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1999 "mapparser.c" /* yacc.c:1646  */
    break;

  case 40:
#line 356 "mapparser.y" /* yacc.c:1646  */
    {
    char *delim, *bufferp;

    (yyval.intval) = MS_FALSE;
    bufferp=(yyvsp[0].strval);

    while((delim=strchr(bufferp,',')) != NULL) {
      *delim='\0';
      if(strcmp((yyvsp[-2].strval),bufferp) == 0) {
        (yyval.intval) = MS_TRUE;
        break;
      } 
      *delim=',';
      bufferp=delim+1;
    }

    if((yyval.intval) == MS_FALSE && strcmp((yyvsp[-2].strval),bufferp) == 0) // test for last (or only) item
      (yyval.intval) = MS_TRUE;
    free((yyvsp[-2].strval));
    free((yyvsp[0].strval));
  }
#line 2025 "mapparser.c" /* yacc.c:1646  */
    break;

  case 41:
#line 377 "mapparser.y" /* yacc.c:1646  */
    {
    char *delim,*bufferp;

    (yyval.intval) = MS_FALSE;
    bufferp=(yyvsp[0].strval);

    while((delim=strchr(bufferp,',')) != NULL) {
      *delim='\0';
      if((yyvsp[-2].dblval) == atof(bufferp)) {
        (yyval.intval) = MS_TRUE;
        break;
      } 
      *delim=',';
      bufferp=delim+1;
    }

    if((yyvsp[-2].dblval) == atof(bufferp)) // is this test necessary?
      (yyval.intval) = MS_TRUE;  
    free((yyvsp[0].strval));
  }
#line 2050 "mapparser.c" /* yacc.c:1646  */
    break;

  case 42:
#line 397 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-2].dblval) == (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 2061 "mapparser.c" /* yacc.c:1646  */
    break;

  case 43:
#line 403 "mapparser.y" /* yacc.c:1646  */
    {
    if(strcasecmp((yyvsp[-2].strval), (yyvsp[0].strval)) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    free((yyvsp[-2].strval));
    free((yyvsp[0].strval));
  }
#line 2074 "mapparser.c" /* yacc.c:1646  */
    break;

  case 44:
#line 411 "mapparser.y" /* yacc.c:1646  */
    {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 2085 "mapparser.c" /* yacc.c:1646  */
    break;

  case 45:
#line 417 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSEquals((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-2].shpval));
    if((yyvsp[0].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[0].shpval));
    if(rval == -1) {
      yyerror(p, "Equals (EQ or ==) operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2101 "mapparser.c" /* yacc.c:1646  */
    break;

  case 46:
#line 428 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSEquals((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-3].shpval));
    if((yyvsp[-1].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-1].shpval));
    if(rval == -1) {
      yyerror(p, "Equals function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2117 "mapparser.c" /* yacc.c:1646  */
    break;

  case 47:
#line 439 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSIntersects((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-3].shpval));
    if((yyvsp[-1].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-1].shpval));
    if(rval == -1) {
      yyerror(p, "Intersects function failed.");
      return(-1);
    } else
    (yyval.intval) = rval;
  }
#line 2133 "mapparser.c" /* yacc.c:1646  */
    break;

  case 48:
#line 450 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSIntersects((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-2].shpval));
    if((yyvsp[0].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[0].shpval)); 
    if(rval == -1) {
      yyerror(p, "Intersects operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2149 "mapparser.c" /* yacc.c:1646  */
    break;

  case 49:
#line 461 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSDisjoint((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-3].shpval));
    if((yyvsp[-1].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-1].shpval));
    if(rval == -1) {
      yyerror(p, "Disjoint function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2165 "mapparser.c" /* yacc.c:1646  */
    break;

  case 50:
#line 472 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSDisjoint((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-2].shpval));
    if((yyvsp[0].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[0].shpval));
    if(rval == -1) {
      yyerror(p, "Disjoint operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2181 "mapparser.c" /* yacc.c:1646  */
    break;

  case 51:
#line 483 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSTouches((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-3].shpval));
    if((yyvsp[-1].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-1].shpval));
    if(rval == -1) {
      yyerror(p, "Touches function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2197 "mapparser.c" /* yacc.c:1646  */
    break;

  case 52:
#line 494 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSTouches((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-2].shpval));
    if((yyvsp[0].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[0].shpval));
    if(rval == -1) {
      yyerror(p, "Touches operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2213 "mapparser.c" /* yacc.c:1646  */
    break;

  case 53:
#line 505 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSOverlaps((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-3].shpval));
    if((yyvsp[-1].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-1].shpval));
    if(rval == -1) {
      yyerror(p, "Overlaps function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2229 "mapparser.c" /* yacc.c:1646  */
    break;

  case 54:
#line 516 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
     rval = msGEOSOverlaps((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-2].shpval));
    if((yyvsp[0].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[0].shpval));
    if(rval == -1) {
      yyerror(p, "Overlaps operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2245 "mapparser.c" /* yacc.c:1646  */
    break;

  case 55:
#line 527 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSCrosses((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-3].shpval));
    if((yyvsp[-1].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-1].shpval));
    if(rval == -1) {
      yyerror(p, "Crosses function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2261 "mapparser.c" /* yacc.c:1646  */
    break;

  case 56:
#line 538 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSCrosses((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-2].shpval));
    if((yyvsp[0].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[0].shpval));
    if(rval == -1) {
      yyerror(p, "Crosses operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2277 "mapparser.c" /* yacc.c:1646  */
    break;

  case 57:
#line 549 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSWithin((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-3].shpval));
    if((yyvsp[-1].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-1].shpval));
    if(rval == -1) {
      yyerror(p, "Within function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2293 "mapparser.c" /* yacc.c:1646  */
    break;

  case 58:
#line 560 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSWithin((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-2].shpval));
    if((yyvsp[0].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[0].shpval));
    if(rval == -1) {
      yyerror(p, "Within operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2309 "mapparser.c" /* yacc.c:1646  */
    break;

  case 59:
#line 571 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSContains((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-3].shpval));
    if((yyvsp[-1].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-1].shpval));
    if(rval == -1) {
      yyerror(p, "Contains function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2325 "mapparser.c" /* yacc.c:1646  */
    break;

  case 60:
#line 582 "mapparser.y" /* yacc.c:1646  */
    {
    int rval;
    rval = msGEOSContains((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-2].shpval));
    if((yyvsp[0].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[0].shpval));
    if(rval == -1) {
      yyerror(p, "Contains operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2341 "mapparser.c" /* yacc.c:1646  */
    break;

  case 61:
#line 593 "mapparser.y" /* yacc.c:1646  */
    {
    double d;
    d = msGEOSDistance((yyvsp[-5].shpval), (yyvsp[-3].shpval));    
    if((yyvsp[-5].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-5].shpval));
    if((yyvsp[-3].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-3].shpval));
    if(d <= (yyvsp[-1].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 2356 "mapparser.c" /* yacc.c:1646  */
    break;

  case 62:
#line 603 "mapparser.y" /* yacc.c:1646  */
    {
    double d;
    d = msGEOSDistance((yyvsp[-5].shpval), (yyvsp[-3].shpval));
    if((yyvsp[-5].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-5].shpval));
    if((yyvsp[-3].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-3].shpval));
    if(d > (yyvsp[-1].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 2371 "mapparser.c" /* yacc.c:1646  */
    break;

  case 64:
#line 616 "mapparser.y" /* yacc.c:1646  */
    { (yyval.dblval) = (yyvsp[-1].dblval); }
#line 2377 "mapparser.c" /* yacc.c:1646  */
    break;

  case 65:
#line 617 "mapparser.y" /* yacc.c:1646  */
    { (yyval.dblval) = (yyvsp[-2].dblval) + (yyvsp[0].dblval); }
#line 2383 "mapparser.c" /* yacc.c:1646  */
    break;

  case 66:
#line 618 "mapparser.y" /* yacc.c:1646  */
    { (yyval.dblval) = (yyvsp[-2].dblval) - (yyvsp[0].dblval); }
#line 2389 "mapparser.c" /* yacc.c:1646  */
    break;

  case 67:
#line 619 "mapparser.y" /* yacc.c:1646  */
    { (yyval.dblval) = (yyvsp[-2].dblval) * (yyvsp[0].dblval); }
#line 2395 "mapparser.c" /* yacc.c:1646  */
    break;

  case 68:
#line 620 "mapparser.y" /* yacc.c:1646  */
    { (yyval.dblval) = (int)(yyvsp[-2].dblval) % (int)(yyvsp[0].dblval); }
#line 2401 "mapparser.c" /* yacc.c:1646  */
    break;

  case 69:
#line 621 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[0].dblval) == 0.0) {
      yyerror(p, "Division by zero.");
      return(-1);
    } else
      (yyval.dblval) = (yyvsp[-2].dblval) / (yyvsp[0].dblval); 
  }
#line 2413 "mapparser.c" /* yacc.c:1646  */
    break;

  case 70:
#line 628 "mapparser.y" /* yacc.c:1646  */
    { (yyval.dblval) = (yyvsp[0].dblval); }
#line 2419 "mapparser.c" /* yacc.c:1646  */
    break;

  case 71:
#line 629 "mapparser.y" /* yacc.c:1646  */
    { (yyval.dblval) = pow((yyvsp[-2].dblval), (yyvsp[0].dblval)); }
#line 2425 "mapparser.c" /* yacc.c:1646  */
    break;

  case 72:
#line 630 "mapparser.y" /* yacc.c:1646  */
    { (yyval.dblval) = strlen((yyvsp[-1].strval)); }
#line 2431 "mapparser.c" /* yacc.c:1646  */
    break;

  case 73:
#line 631 "mapparser.y" /* yacc.c:1646  */
    {
    if((yyvsp[-1].shpval)->type != MS_SHAPE_POLYGON) {
      yyerror(p, "Area can only be computed for polygon shapes.");
      return(-1);
    }
    (yyval.dblval) = msGetPolygonArea((yyvsp[-1].shpval));
    if((yyvsp[-1].shpval)->scratch == MS_TRUE) msFreeShape((yyvsp[-1].shpval));
  }
#line 2444 "mapparser.c" /* yacc.c:1646  */
    break;

  case 74:
#line 639 "mapparser.y" /* yacc.c:1646  */
    { (yyval.dblval) = (MS_NINT((yyvsp[-3].dblval)/(yyvsp[-1].dblval)))*(yyvsp[-1].dblval); }
#line 2450 "mapparser.c" /* yacc.c:1646  */
    break;

  case 75:
#line 640 "mapparser.y" /* yacc.c:1646  */
    { (yyval.dblval) = (MS_NINT((yyvsp[-1].dblval))); }
#line 2456 "mapparser.c" /* yacc.c:1646  */
    break;

  case 77:
#line 644 "mapparser.y" /* yacc.c:1646  */
    { (yyval.shpval) = (yyvsp[-1].shpval); }
#line 2462 "mapparser.c" /* yacc.c:1646  */
    break;

  case 78:
#line 645 "mapparser.y" /* yacc.c:1646  */
    {
    shapeObj *s;
    s = msGEOSBuffer((yyvsp[-3].shpval), (yyvsp[-1].dblval));
    if(!s) {
      yyerror(p, "Executing buffer failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2477 "mapparser.c" /* yacc.c:1646  */
    break;

  case 79:
#line 655 "mapparser.y" /* yacc.c:1646  */
    {
    shapeObj *s;
    s = msGEOSDifference((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if(!s) {
      yyerror(p, "Executing difference failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2492 "mapparser.c" /* yacc.c:1646  */
    break;

  case 80:
#line 665 "mapparser.y" /* yacc.c:1646  */
    {
    shapeObj *s;
    s = msGEOSSimplify((yyvsp[-3].shpval), (yyvsp[-1].dblval));
    if(!s) {
      yyerror(p, "Executing simplify failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2507 "mapparser.c" /* yacc.c:1646  */
    break;

  case 81:
#line 675 "mapparser.y" /* yacc.c:1646  */
    {
    shapeObj *s;
    s = msGEOSTopologyPreservingSimplify((yyvsp[-3].shpval), (yyvsp[-1].dblval));
    if(!s) {
      yyerror(p, "Executing simplifypt failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2522 "mapparser.c" /* yacc.c:1646  */
    break;

  case 82:
#line 685 "mapparser.y" /* yacc.c:1646  */
    {
    shapeObj *s;
    s = msGeneralize((yyvsp[-3].shpval), (yyvsp[-1].dblval));
    if(!s) {
      yyerror(p, "Executing generalize failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2537 "mapparser.c" /* yacc.c:1646  */
    break;

  case 83:
#line 695 "mapparser.y" /* yacc.c:1646  */
    {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[-1].shpval), 3, 1, NULL);
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2552 "mapparser.c" /* yacc.c:1646  */
    break;

  case 84:
#line 705 "mapparser.y" /* yacc.c:1646  */
    {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[-3].shpval), (yyvsp[-1].dblval), 1, NULL);
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2567 "mapparser.c" /* yacc.c:1646  */
    break;

  case 85:
#line 715 "mapparser.y" /* yacc.c:1646  */
    {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[-5].shpval), (yyvsp[-3].dblval), (yyvsp[-1].dblval), NULL);
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2582 "mapparser.c" /* yacc.c:1646  */
    break;

  case 86:
#line 725 "mapparser.y" /* yacc.c:1646  */
    {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[-7].shpval), (yyvsp[-5].dblval), (yyvsp[-3].dblval), (yyvsp[-1].strval));
    free((yyvsp[-1].strval));
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2598 "mapparser.c" /* yacc.c:1646  */
    break;

  case 87:
#line 736 "mapparser.y" /* yacc.c:1646  */
    {
#ifdef USE_V8_MAPSCRIPT
    shapeObj *s;
    s = msV8TransformShape((yyvsp[-3].shpval), (yyvsp[-1].strval));
    free((yyvsp[-1].strval));
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
#line 2619 "mapparser.c" /* yacc.c:1646  */
    break;

  case 89:
#line 755 "mapparser.y" /* yacc.c:1646  */
    { (yyval.strval) = (yyvsp[-1].strval); }
#line 2625 "mapparser.c" /* yacc.c:1646  */
    break;

  case 90:
#line 756 "mapparser.y" /* yacc.c:1646  */
    { 
    (yyval.strval) = (char *)malloc(strlen((yyvsp[-2].strval)) + strlen((yyvsp[0].strval)) + 1);
    sprintf((yyval.strval), "%s%s", (yyvsp[-2].strval), (yyvsp[0].strval)); free((yyvsp[-2].strval)); free((yyvsp[0].strval)); 
  }
#line 2634 "mapparser.c" /* yacc.c:1646  */
    break;

  case 91:
#line 760 "mapparser.y" /* yacc.c:1646  */
    {
    (yyval.strval) = (char *) malloc(strlen((yyvsp[-1].strval)) + 64); /* Plenty big? Should use snprintf below... */
    sprintf((yyval.strval), (yyvsp[-1].strval), (yyvsp[-3].dblval));
    free((yyvsp[-1].strval));
  }
#line 2644 "mapparser.c" /* yacc.c:1646  */
    break;

  case 92:
#line 765 "mapparser.y" /* yacc.c:1646  */
    {  
    (yyvsp[-1].strval) = msCommifyString((yyvsp[-1].strval)); 
    (yyval.strval) = (yyvsp[-1].strval); 
  }
#line 2653 "mapparser.c" /* yacc.c:1646  */
    break;

  case 93:
#line 769 "mapparser.y" /* yacc.c:1646  */
    {  
    msStringToUpper((yyvsp[-1].strval)); 
    (yyval.strval) = (yyvsp[-1].strval); 
  }
#line 2662 "mapparser.c" /* yacc.c:1646  */
    break;

  case 94:
#line 773 "mapparser.y" /* yacc.c:1646  */
    {  
    msStringToLower((yyvsp[-1].strval)); 
    (yyval.strval) = (yyvsp[-1].strval); 
  }
#line 2671 "mapparser.c" /* yacc.c:1646  */
    break;

  case 95:
#line 777 "mapparser.y" /* yacc.c:1646  */
    {  
    msStringInitCap((yyvsp[-1].strval)); 
    (yyval.strval) = (yyvsp[-1].strval); 
  }
#line 2680 "mapparser.c" /* yacc.c:1646  */
    break;

  case 96:
#line 781 "mapparser.y" /* yacc.c:1646  */
    {  
    msStringFirstCap((yyvsp[-1].strval)); 
    (yyval.strval) = (yyvsp[-1].strval); 
  }
#line 2689 "mapparser.c" /* yacc.c:1646  */
    break;

  case 98:
#line 788 "mapparser.y" /* yacc.c:1646  */
    { (yyval.tmval) = (yyvsp[-1].tmval); }
#line 2695 "mapparser.c" /* yacc.c:1646  */
    break;


#line 2699 "mapparser.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (p, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (p, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
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

  /* Else will try to reuse lookahead token after shifting the error
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (p, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, p);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
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
  return yyresult;
}
#line 791 "mapparser.y" /* yacc.c:1906  */


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
