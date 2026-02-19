/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 5 "src/mapparser.y"

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

#line 89 "/Users/gfabbian/projects/MapServer/src/mapparser.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "mapparser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_BOOLEAN = 3,                    /* BOOLEAN  */
  YYSYMBOL_NUMBER = 4,                     /* NUMBER  */
  YYSYMBOL_STRING = 5,                     /* STRING  */
  YYSYMBOL_TIME = 6,                       /* TIME  */
  YYSYMBOL_SHAPE = 7,                      /* SHAPE  */
  YYSYMBOL_OR = 8,                         /* OR  */
  YYSYMBOL_AND = 9,                        /* AND  */
  YYSYMBOL_NOT = 10,                       /* NOT  */
  YYSYMBOL_RE = 11,                        /* RE  */
  YYSYMBOL_EQ = 12,                        /* EQ  */
  YYSYMBOL_NE = 13,                        /* NE  */
  YYSYMBOL_LT = 14,                        /* LT  */
  YYSYMBOL_GT = 15,                        /* GT  */
  YYSYMBOL_LE = 16,                        /* LE  */
  YYSYMBOL_GE = 17,                        /* GE  */
  YYSYMBOL_IN = 18,                        /* IN  */
  YYSYMBOL_IEQ = 19,                       /* IEQ  */
  YYSYMBOL_IRE = 20,                       /* IRE  */
  YYSYMBOL_INTERSECTS = 21,                /* INTERSECTS  */
  YYSYMBOL_DISJOINT = 22,                  /* DISJOINT  */
  YYSYMBOL_TOUCHES = 23,                   /* TOUCHES  */
  YYSYMBOL_OVERLAPS = 24,                  /* OVERLAPS  */
  YYSYMBOL_CROSSES = 25,                   /* CROSSES  */
  YYSYMBOL_WITHIN = 26,                    /* WITHIN  */
  YYSYMBOL_CONTAINS = 27,                  /* CONTAINS  */
  YYSYMBOL_EQUALS = 28,                    /* EQUALS  */
  YYSYMBOL_BEYOND = 29,                    /* BEYOND  */
  YYSYMBOL_DWITHIN = 30,                   /* DWITHIN  */
  YYSYMBOL_AREA = 31,                      /* AREA  */
  YYSYMBOL_LENGTH = 32,                    /* LENGTH  */
  YYSYMBOL_COMMIFY = 33,                   /* COMMIFY  */
  YYSYMBOL_ROUND = 34,                     /* ROUND  */
  YYSYMBOL_UPPER = 35,                     /* UPPER  */
  YYSYMBOL_LOWER = 36,                     /* LOWER  */
  YYSYMBOL_INITCAP = 37,                   /* INITCAP  */
  YYSYMBOL_FIRSTCAP = 38,                  /* FIRSTCAP  */
  YYSYMBOL_TOSTRING = 39,                  /* TOSTRING  */
  YYSYMBOL_YYBUFFER = 40,                  /* YYBUFFER  */
  YYSYMBOL_INNER = 41,                     /* INNER  */
  YYSYMBOL_OUTER = 42,                     /* OUTER  */
  YYSYMBOL_DIFFERENCE = 43,                /* DIFFERENCE  */
  YYSYMBOL_DENSIFY = 44,                   /* DENSIFY  */
  YYSYMBOL_SIMPLIFY = 45,                  /* SIMPLIFY  */
  YYSYMBOL_SIMPLIFYPT = 46,                /* SIMPLIFYPT  */
  YYSYMBOL_GENERALIZE = 47,                /* GENERALIZE  */
  YYSYMBOL_SMOOTHSIA = 48,                 /* SMOOTHSIA  */
  YYSYMBOL_CENTERLINE = 49,                /* CENTERLINE  */
  YYSYMBOL_JAVASCRIPT = 50,                /* JAVASCRIPT  */
  YYSYMBOL_51_ = 51,                       /* '+'  */
  YYSYMBOL_52_ = 52,                       /* '-'  */
  YYSYMBOL_53_ = 53,                       /* '*'  */
  YYSYMBOL_54_ = 54,                       /* '/'  */
  YYSYMBOL_55_ = 55,                       /* '%'  */
  YYSYMBOL_NEG = 56,                       /* NEG  */
  YYSYMBOL_57_ = 57,                       /* '^'  */
  YYSYMBOL_58_ = 58,                       /* '('  */
  YYSYMBOL_59_ = 59,                       /* ')'  */
  YYSYMBOL_60_ = 60,                       /* ','  */
  YYSYMBOL_YYACCEPT = 61,                  /* $accept  */
  YYSYMBOL_input = 62,                     /* input  */
  YYSYMBOL_logical_exp = 63,               /* logical_exp  */
  YYSYMBOL_math_exp = 64,                  /* math_exp  */
  YYSYMBOL_shape_exp = 65,                 /* shape_exp  */
  YYSYMBOL_string_exp = 66,                /* string_exp  */
  YYSYMBOL_time_exp = 67                   /* time_exp  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

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


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
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

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
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
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  86
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   540

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  61
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  7
/* YYNRULES -- Number of rules.  */
#define YYNRULES  102
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  306

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   306


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
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
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    68,    68,    69,    82,    96,   110,   120,   121,   122,
     126,   134,   143,   151,   160,   168,   177,   185,   194,   195,
     196,   216,   236,   242,   248,   254,   260,   266,   272,   280,
     288,   296,   304,   312,   320,   326,   332,   338,   344,   350,
     356,   377,   397,   403,   411,   417,   434,   451,   468,   485,
     502,   519,   536,   553,   570,   587,   604,   621,   638,   655,
     672,   689,   705,   723,   724,   725,   726,   727,   728,   729,
     736,   737,   738,   739,   750,   751,   754,   755,   756,   770,
     784,   798,   812,   826,   840,   854,   868,   882,   896,   910,
     924,   939,   961,   962,   963,   969,   978,   982,   986,   990,
     994,  1000,  1001
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "BOOLEAN", "NUMBER",
  "STRING", "TIME", "SHAPE", "OR", "AND", "NOT", "RE", "EQ", "NE", "LT",
  "GT", "LE", "GE", "IN", "IEQ", "IRE", "INTERSECTS", "DISJOINT",
  "TOUCHES", "OVERLAPS", "CROSSES", "WITHIN", "CONTAINS", "EQUALS",
  "BEYOND", "DWITHIN", "AREA", "LENGTH", "COMMIFY", "ROUND", "UPPER",
  "LOWER", "INITCAP", "FIRSTCAP", "TOSTRING", "YYBUFFER", "INNER", "OUTER",
  "DIFFERENCE", "DENSIFY", "SIMPLIFY", "SIMPLIFYPT", "GENERALIZE",
  "SMOOTHSIA", "CENTERLINE", "JAVASCRIPT", "'+'", "'-'", "'*'", "'/'",
  "'%'", "NEG", "'^'", "'('", "')'", "','", "$accept", "input",
  "logical_exp", "math_exp", "shape_exp", "string_exp", "time_exp", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-56)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     355,   -56,   -56,   -56,   -56,   -56,   355,   -55,   -48,   -35,
     -33,     1,    19,    39,    48,    53,    54,    71,    72,    74,
     116,   128,   130,   131,   132,   133,   134,   135,   157,   158,
     159,   160,   173,   182,   183,   197,   198,    -3,   355,    51,
     119,   211,    69,    56,   521,    44,   412,    69,    56,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
       4,     4,    -3,     4,     4,     4,     4,    -3,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,    -3,
     102,    -1,   154,    -6,   403,   304,   -56,   355,   355,   355,
     355,   355,    -3,    -3,    -3,    -3,    -3,    -3,     4,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,   203,   203,   203,   203,
     203,   203,   203,   203,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,    -4,    -4,    -4,    -4,    -4,
      -4,    -4,   203,   152,   161,   199,   200,   209,   225,   228,
     230,   242,   244,   247,     4,   -46,   -37,   101,   -27,   -25,
     -24,   106,   381,   248,   255,   263,   264,   265,   266,   267,
     268,    50,   270,   272,   316,   -56,   -56,   -56,   -56,   -56,
      21,   258,    44,   412,   -56,   211,    21,   258,    44,   412,
      -7,    -7,    -7,    -7,    -7,    -7,     6,    -7,    25,    25,
     102,   102,   102,   102,   -56,   -56,   -56,   -56,   -56,   -56,
     -56,   -56,     6,     6,     6,     6,     6,     6,     6,     6,
       6,     6,   -56,    -4,   -56,   -56,   -56,   -56,   -56,   -56,
     -56,   274,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   -56,   163,   -56,   -56,   -56,    -3,   -56,   -56,
     -56,   -56,     4,    -3,   -56,   -56,   203,    -3,    -3,    -3,
      -3,   -56,    -3,   -56,     4,   280,   285,   286,   287,   288,
     305,   307,   315,   347,   348,   349,   391,   240,   419,   351,
     428,   437,   446,   455,   227,   241,   -56,   -56,   -56,   -56,
     -56,   -56,   -56,   -56,    -3,    -3,   -56,   -56,   -56,   -56,
     -56,   -56,   -56,   -56,   -56,    -3,   -56,   464,   473,   283,
     -56,   -56,   -56,     4,   246,   -56
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
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
      22,    23,    25,    24,    27,    26,    41,    42,    65,    66,
      67,    69,    68,    71,    45,    48,    50,    52,    54,    56,
      58,    60,    20,    28,    29,    31,    30,    33,    32,    40,
      43,    21,    94,     0,    34,    35,    37,    36,    39,    38,
      44,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    73,     0,    72,    96,    75,     0,    97,    98,
      99,   100,     0,     0,    79,    80,     0,     0,     0,     0,
       0,    87,     0,    81,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    47,    49,    51,    53,
      55,    57,    59,    46,     0,     0,    74,    95,    78,    82,
      83,    84,    85,    86,    88,     0,    91,     0,     0,     0,
      62,    61,    89,     0,     0,    90
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -56,   -56,    -2,    46,   126,     0,   -26
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    39,    40,    41,    47,    48,    44
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      43,     2,     4,    49,    45,   124,   106,    87,    88,     3,
      50,    89,    85,   234,   124,   107,   108,   109,   110,   111,
     112,   113,   235,    51,   124,    52,   124,   124,    17,    18,
      88,    20,   238,    89,   239,   240,    81,    19,    84,    21,
      22,    23,    24,    25,   100,   101,   102,   103,   104,    37,
     105,    86,    46,   167,   213,    79,    89,   124,   165,    53,
     145,   146,   144,   148,   149,   150,   151,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,    54,   102,   103,
     104,   106,   105,    80,    82,   170,   172,   174,   176,   178,
     107,   108,   109,   110,   111,   112,   113,    55,   186,   214,
     215,   216,   217,   218,   219,   220,    56,   124,   147,   251,
     252,    57,    58,   152,   202,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   164,    42,    87,    88,    59,
      60,    89,    61,   171,   173,   175,   177,   179,   180,   181,
     182,   183,   184,   185,   233,   187,   188,   189,   190,   191,
     192,   193,   100,   101,   102,   103,   104,   124,   105,   105,
     236,   237,    90,    91,    83,   241,    92,    93,    94,    95,
      96,    97,    98,    99,    62,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,    63,   255,    64,    65,
      66,    67,    68,    69,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   100,   101,   102,   103,   104,
       5,   105,   222,   166,   124,    70,    71,    72,    73,    90,
      91,   223,   168,    92,    93,    94,    95,    96,    97,    98,
      99,    74,   194,   195,   196,   197,   198,   199,   200,   201,
      75,    76,   267,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,   275,    77,    78,     0,   221,   224,
     225,   132,   100,   101,   102,   103,   104,    91,   105,   226,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   266,   105,   227,   294,   295,   228,   268,
     229,   124,   124,   270,   271,   272,   273,   124,   274,   287,
     296,     0,   230,   304,   231,   305,   232,     0,   243,   100,
     101,   102,   103,   104,   244,   105,   125,   126,   127,   128,
     129,   130,   245,   131,   246,   247,   248,   249,   250,   253,
     297,   298,   254,   167,   100,   101,   102,   103,   104,   169,
     105,   299,   302,   303,   276,   277,   278,   279,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,     1,     2,
       3,     4,     5,   169,   280,     6,   281,   100,   101,   102,
     103,   104,   269,   105,   282,   166,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,   283,    37,   284,   285,
     289,     0,     0,    38,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,     0,   105,     0,
       0,   242,   100,   101,   102,   103,   104,     0,   105,     0,
     286,     0,     0,     0,   124,     0,     0,     0,     0,     0,
       0,     0,   168,   100,   101,   102,   103,   104,     0,   105,
     100,   101,   102,   103,   104,     0,   105,     0,   288,   100,
     101,   102,   103,   104,     0,   105,     0,   290,   100,   101,
     102,   103,   104,     0,   105,     0,   291,   100,   101,   102,
     103,   104,     0,   105,     0,   292,   100,   101,   102,   103,
     104,     0,   105,     0,   293,   100,   101,   102,   103,   104,
       0,   105,     0,   300,   100,   101,   102,   103,   104,     0,
     105,     0,   301,   125,   126,   127,   128,   129,   130,     0,
     131
};

static const yytype_int16 yycheck[] =
{
       0,     4,     6,    58,     6,    51,    12,     8,     9,     5,
      58,    12,    38,    59,    51,    21,    22,    23,    24,    25,
      26,    27,    59,    58,    51,    58,    51,    51,    31,    32,
       9,    34,    59,    12,    59,    59,    38,    33,    38,    35,
      36,    37,    38,    39,    51,    52,    53,    54,    55,    52,
      57,     0,     6,    59,    58,    58,    12,    51,    59,    58,
      60,    61,    58,    63,    64,    65,    66,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    58,    53,    54,
      55,    12,    57,    37,    38,    87,    88,    89,    90,    91,
      21,    22,    23,    24,    25,    26,    27,    58,    98,   125,
     126,   127,   128,   129,   130,   131,    58,    51,    62,    59,
      60,    58,    58,    67,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,    79,     0,     8,     9,    58,
      58,    12,    58,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,   144,    99,   100,   101,   102,   103,
     104,   105,    51,    52,    53,    54,    55,    51,    57,    57,
      59,    60,     8,     9,    38,    59,    12,    13,    14,    15,
      16,    17,    18,    19,    58,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    58,   213,    58,    58,
      58,    58,    58,    58,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    51,    52,    53,    54,    55,
       7,    57,    60,    59,    51,    58,    58,    58,    58,     8,
       9,    60,    59,    12,    13,    14,    15,    16,    17,    18,
      19,    58,   106,   107,   108,   109,   110,   111,   112,   113,
      58,    58,   242,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,   254,    58,    58,    -1,   132,    60,
      60,    58,    51,    52,    53,    54,    55,     9,    57,    60,
      12,    13,    14,    15,    16,    17,    18,    19,    51,    52,
      53,    54,    55,   237,    57,    60,    59,    60,    60,   243,
      60,    51,    51,   247,   248,   249,   250,    51,   252,    59,
      59,    -1,    60,   303,    60,    59,    59,    -1,    60,    51,
      52,    53,    54,    55,    59,    57,    12,    13,    14,    15,
      16,    17,    59,    19,    60,    60,    60,    60,    60,    59,
     284,   285,    60,    59,    51,    52,    53,    54,    55,    59,
      57,   295,    59,    60,    59,    59,    59,    59,   222,   223,
     224,   225,   226,   227,   228,   229,   230,   231,     3,     4,
       5,     6,     7,    59,    59,    10,    59,    51,    52,    53,
      54,    55,   246,    57,    59,    59,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    59,    52,    60,    60,
      59,    -1,    -1,    58,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    12,    13,    14,    15,    16,    17,
      18,    19,    51,    52,    53,    54,    55,    -1,    57,    -1,
      -1,    60,    51,    52,    53,    54,    55,    -1,    57,    -1,
      59,    -1,    -1,    -1,    51,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    59,    51,    52,    53,    54,    55,    -1,    57,
      51,    52,    53,    54,    55,    -1,    57,    -1,    59,    51,
      52,    53,    54,    55,    -1,    57,    -1,    59,    51,    52,
      53,    54,    55,    -1,    57,    -1,    59,    51,    52,    53,
      54,    55,    -1,    57,    -1,    59,    51,    52,    53,    54,
      55,    -1,    57,    -1,    59,    51,    52,    53,    54,    55,
      -1,    57,    -1,    59,    51,    52,    53,    54,    55,    -1,
      57,    -1,    59,    12,    13,    14,    15,    16,    17,    -1,
      19
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,    10,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    52,    58,    62,
      63,    64,    65,    66,    67,    63,    64,    65,    66,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      64,    63,    64,    65,    66,    67,     0,     8,     9,    12,
       8,     9,    12,    13,    14,    15,    16,    17,    18,    19,
      51,    52,    53,    54,    55,    57,    12,    21,    22,    23,
      24,    25,    26,    27,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    51,    12,    13,    14,    15,    16,
      17,    19,    58,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    58,    66,    66,    64,    66,    66,
      66,    66,    64,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    64,    59,    59,    59,    59,    59,
      63,    64,    63,    64,    63,    64,    63,    64,    63,    64,
      64,    64,    64,    64,    64,    64,    66,    64,    64,    64,
      64,    64,    64,    64,    65,    65,    65,    65,    65,    65,
      65,    65,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    58,    67,    67,    67,    67,    67,    67,
      67,    65,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    59,    66,    59,    59,    59,    60,    59,    59,
      59,    59,    60,    60,    59,    59,    60,    60,    60,    60,
      60,    59,    60,    59,    60,    67,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    64,    66,    64,    65,
      64,    64,    64,    64,    64,    66,    59,    59,    59,    59,
      59,    59,    59,    59,    60,    60,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    60,    59,    64,    64,    64,
      59,    59,    59,    60,    66,    59
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
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

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
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


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
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

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


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




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, p); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, parseObj *p)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (p);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, parseObj *p)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, p);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, parseObj *p)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], p);
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
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, parseObj *p)
{
  YY_USE (yyvaluep);
  YY_USE (p);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (parseObj *p)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, p);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
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
  case 3: /* input: logical_exp  */
#line 69 "src/mapparser.y"
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
#line 1390 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 4: /* input: math_exp  */
#line 82 "src/mapparser.y"
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
#line 1409 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 5: /* input: string_exp  */
#line 96 "src/mapparser.y"
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
#line 1428 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 6: /* input: shape_exp  */
#line 110 "src/mapparser.y"
              {
    switch(p->type) {
    case(MS_PARSE_TYPE_SHAPE):
      p->result.shpval = (yyvsp[0].shpval);
      p->result.shpval->scratch = MS_FALSE;
      break;
    }
  }
#line 1441 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 8: /* logical_exp: '(' logical_exp ')'  */
#line 121 "src/mapparser.y"
                        { (yyval.intval) = (yyvsp[-1].intval); }
#line 1447 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 9: /* logical_exp: logical_exp EQ logical_exp  */
#line 122 "src/mapparser.y"
                               {
    (yyval.intval) = MS_FALSE;
    if((yyvsp[-2].intval) == (yyvsp[0].intval)) (yyval.intval) = MS_TRUE;
  }
#line 1456 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 10: /* logical_exp: logical_exp OR logical_exp  */
#line 126 "src/mapparser.y"
                               {
    if((yyvsp[-2].intval) == MS_TRUE)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[0].intval) == MS_TRUE)          
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1469 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 11: /* logical_exp: logical_exp AND logical_exp  */
#line 134 "src/mapparser.y"
                                {
    if((yyvsp[-2].intval) == MS_TRUE) {
      if((yyvsp[0].intval) == MS_TRUE)
        (yyval.intval) = MS_TRUE;
      else
        (yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
#line 1483 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 12: /* logical_exp: logical_exp OR math_exp  */
#line 143 "src/mapparser.y"
                            {
    if((yyvsp[-2].intval) == MS_TRUE)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[0].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1496 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 13: /* logical_exp: logical_exp AND math_exp  */
#line 151 "src/mapparser.y"
                             {
    if((yyvsp[-2].intval) == MS_TRUE) {
      if((yyvsp[0].dblval) != 0)
        (yyval.intval) = MS_TRUE;
      else
        (yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
#line 1510 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 14: /* logical_exp: math_exp OR logical_exp  */
#line 160 "src/mapparser.y"
                            {
    if((yyvsp[-2].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[0].intval) == MS_TRUE)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1523 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 15: /* logical_exp: math_exp AND logical_exp  */
#line 168 "src/mapparser.y"
                             {
    if((yyvsp[-2].dblval) != 0) {
      if((yyvsp[0].intval) == MS_TRUE)
        (yyval.intval) = MS_TRUE;
      else
        (yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
#line 1537 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 16: /* logical_exp: math_exp OR math_exp  */
#line 177 "src/mapparser.y"
                         {
    if((yyvsp[-2].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else if((yyvsp[0].dblval) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1550 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 17: /* logical_exp: math_exp AND math_exp  */
#line 185 "src/mapparser.y"
                               {
    if((yyvsp[-2].dblval) != 0) {
      if((yyvsp[0].dblval) != 0)
        (yyval.intval) = MS_TRUE;
      else
	(yyval.intval) = MS_FALSE;
    } else
      (yyval.intval) = MS_FALSE;
  }
#line 1564 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 18: /* logical_exp: NOT logical_exp  */
#line 194 "src/mapparser.y"
                    { (yyval.intval) = !(yyvsp[0].intval); }
#line 1570 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 19: /* logical_exp: NOT math_exp  */
#line 195 "src/mapparser.y"
                 { (yyval.intval) = !(yyvsp[0].dblval); }
#line 1576 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 20: /* logical_exp: string_exp RE string_exp  */
#line 196 "src/mapparser.y"
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

    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1601 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 21: /* logical_exp: string_exp IRE string_exp  */
#line 216 "src/mapparser.y"
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

    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1626 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 22: /* logical_exp: math_exp EQ math_exp  */
#line 236 "src/mapparser.y"
                         {
    if((yyvsp[-2].dblval) == (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1637 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 23: /* logical_exp: math_exp NE math_exp  */
#line 242 "src/mapparser.y"
                         {
    if((yyvsp[-2].dblval) != (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1648 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 24: /* logical_exp: math_exp GT math_exp  */
#line 248 "src/mapparser.y"
                         {    
    if((yyvsp[-2].dblval) > (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1659 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 25: /* logical_exp: math_exp LT math_exp  */
#line 254 "src/mapparser.y"
                         {
    if((yyvsp[-2].dblval) < (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1670 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 26: /* logical_exp: math_exp GE math_exp  */
#line 260 "src/mapparser.y"
                         {
    if((yyvsp[-2].dblval) >= (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1681 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 27: /* logical_exp: math_exp LE math_exp  */
#line 266 "src/mapparser.y"
                         {
    if((yyvsp[-2].dblval) <= (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1692 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 28: /* logical_exp: string_exp EQ string_exp  */
#line 272 "src/mapparser.y"
                             {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1705 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 29: /* logical_exp: string_exp NE string_exp  */
#line 280 "src/mapparser.y"
                             {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1718 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 30: /* logical_exp: string_exp GT string_exp  */
#line 288 "src/mapparser.y"
                             {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) > 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1731 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 31: /* logical_exp: string_exp LT string_exp  */
#line 296 "src/mapparser.y"
                             {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) < 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1744 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 32: /* logical_exp: string_exp GE string_exp  */
#line 304 "src/mapparser.y"
                             {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) >= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1757 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 33: /* logical_exp: string_exp LE string_exp  */
#line 312 "src/mapparser.y"
                             {
    if(strcmp((yyvsp[-2].strval), (yyvsp[0].strval)) <= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1770 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 34: /* logical_exp: time_exp EQ time_exp  */
#line 320 "src/mapparser.y"
                         {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1781 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 35: /* logical_exp: time_exp NE time_exp  */
#line 326 "src/mapparser.y"
                         {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) != 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1792 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 36: /* logical_exp: time_exp GT time_exp  */
#line 332 "src/mapparser.y"
                         {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) > 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1803 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 37: /* logical_exp: time_exp LT time_exp  */
#line 338 "src/mapparser.y"
                         {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) < 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1814 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 38: /* logical_exp: time_exp GE time_exp  */
#line 344 "src/mapparser.y"
                         {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) >= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1825 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 39: /* logical_exp: time_exp LE time_exp  */
#line 350 "src/mapparser.y"
                         {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) <= 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1836 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 40: /* logical_exp: string_exp IN string_exp  */
#line 356 "src/mapparser.y"
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
    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1862 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 41: /* logical_exp: math_exp IN string_exp  */
#line 377 "src/mapparser.y"
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
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1887 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 42: /* logical_exp: math_exp IEQ math_exp  */
#line 397 "src/mapparser.y"
                          {
    if((yyvsp[-2].dblval) == (yyvsp[0].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1898 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 43: /* logical_exp: string_exp IEQ string_exp  */
#line 403 "src/mapparser.y"
                              {
    if(strcasecmp((yyvsp[-2].strval), (yyvsp[0].strval)) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 1911 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 44: /* logical_exp: time_exp IEQ time_exp  */
#line 411 "src/mapparser.y"
                          {
    if(msTimeCompare(&((yyvsp[-2].tmval)), &((yyvsp[0].tmval))) == 0)
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 1922 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 45: /* logical_exp: shape_exp EQ shape_exp  */
#line 417 "src/mapparser.y"
                           {
    int rval;
    rval = msGEOSEquals((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval) && (yyvsp[-2].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-2].shpval));
      free((yyvsp[-2].shpval));
    }
    if((yyvsp[0].shpval) && (yyvsp[0].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[0].shpval));
      free((yyvsp[0].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Equals (EQ or ==) operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 1944 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 46: /* logical_exp: EQUALS '(' shape_exp ',' shape_exp ')'  */
#line 434 "src/mapparser.y"
                                           {
    int rval;
    rval = msGEOSEquals((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Equals function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 1966 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 47: /* logical_exp: INTERSECTS '(' shape_exp ',' shape_exp ')'  */
#line 451 "src/mapparser.y"
                                               {
    int rval;
    rval = msGEOSIntersects((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Intersects function failed.");
      return(-1);
    } else
    (yyval.intval) = rval;
  }
#line 1988 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 48: /* logical_exp: shape_exp INTERSECTS shape_exp  */
#line 468 "src/mapparser.y"
                                   {
    int rval;
    rval = msGEOSIntersects((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval) && (yyvsp[-2].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-2].shpval));
      free((yyvsp[-2].shpval));
    }
    if((yyvsp[0].shpval) && (yyvsp[0].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[0].shpval));
      free((yyvsp[0].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Intersects operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2010 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 49: /* logical_exp: DISJOINT '(' shape_exp ',' shape_exp ')'  */
#line 485 "src/mapparser.y"
                                             {
    int rval;
    rval = msGEOSDisjoint((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Disjoint function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2032 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 50: /* logical_exp: shape_exp DISJOINT shape_exp  */
#line 502 "src/mapparser.y"
                                 {
    int rval;
    rval = msGEOSDisjoint((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval) && (yyvsp[-2].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-2].shpval));
      free((yyvsp[-2].shpval));
    }
    if((yyvsp[0].shpval) && (yyvsp[0].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[0].shpval));
      free((yyvsp[0].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Disjoint operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2054 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 51: /* logical_exp: TOUCHES '(' shape_exp ',' shape_exp ')'  */
#line 519 "src/mapparser.y"
                                            {
    int rval;
    rval = msGEOSTouches((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Touches function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2076 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 52: /* logical_exp: shape_exp TOUCHES shape_exp  */
#line 536 "src/mapparser.y"
                                {
    int rval;
    rval = msGEOSTouches((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval) && (yyvsp[-2].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-2].shpval));
      free((yyvsp[-2].shpval));
    }
    if((yyvsp[0].shpval) && (yyvsp[0].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[0].shpval));
      free((yyvsp[0].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Touches operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2098 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 53: /* logical_exp: OVERLAPS '(' shape_exp ',' shape_exp ')'  */
#line 553 "src/mapparser.y"
                                             {
    int rval;
    rval = msGEOSOverlaps((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Overlaps function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2120 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 54: /* logical_exp: shape_exp OVERLAPS shape_exp  */
#line 570 "src/mapparser.y"
                                 {
    int rval;
     rval = msGEOSOverlaps((yyvsp[-2].shpval), (yyvsp[0].shpval));
     if((yyvsp[-2].shpval) && (yyvsp[-2].shpval)->scratch == MS_TRUE) {
       msFreeShape((yyvsp[-2].shpval));
       free((yyvsp[-2].shpval));
     }
     if((yyvsp[0].shpval) && (yyvsp[0].shpval)->scratch == MS_TRUE) {
       msFreeShape((yyvsp[0].shpval));
       free((yyvsp[0].shpval));
     }
    if(rval == -1) {
      yyerror(p, "Overlaps operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2142 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 55: /* logical_exp: CROSSES '(' shape_exp ',' shape_exp ')'  */
#line 587 "src/mapparser.y"
                                            {
    int rval;
    rval = msGEOSCrosses((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Crosses function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2164 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 56: /* logical_exp: shape_exp CROSSES shape_exp  */
#line 604 "src/mapparser.y"
                                {
    int rval;
    rval = msGEOSCrosses((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval) && (yyvsp[-2].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-2].shpval));
      free((yyvsp[-2].shpval));
    }
    if((yyvsp[0].shpval) && (yyvsp[0].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[0].shpval));
      free((yyvsp[0].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Crosses operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2186 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 57: /* logical_exp: WITHIN '(' shape_exp ',' shape_exp ')'  */
#line 621 "src/mapparser.y"
                                           {
    int rval;
    rval = msGEOSWithin((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Within function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2208 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 58: /* logical_exp: shape_exp WITHIN shape_exp  */
#line 638 "src/mapparser.y"
                               {
    int rval;
    rval = msGEOSWithin((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval) && (yyvsp[-2].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-2].shpval));
      free((yyvsp[-2].shpval));
    }
    if((yyvsp[0].shpval) && (yyvsp[0].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[0].shpval));
      free((yyvsp[0].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Within operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2230 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 59: /* logical_exp: CONTAINS '(' shape_exp ',' shape_exp ')'  */
#line 655 "src/mapparser.y"
                                             {
    int rval;
    rval = msGEOSContains((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Contains function failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2252 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 60: /* logical_exp: shape_exp CONTAINS shape_exp  */
#line 672 "src/mapparser.y"
                                 {
    int rval;
    rval = msGEOSContains((yyvsp[-2].shpval), (yyvsp[0].shpval));
    if((yyvsp[-2].shpval) && (yyvsp[-2].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-2].shpval));
      free((yyvsp[-2].shpval));
    }
    if((yyvsp[0].shpval) && (yyvsp[0].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[0].shpval));
      free((yyvsp[0].shpval));
    }
    if(rval == -1) {
      yyerror(p, "Contains operator failed.");
      return(-1);
    } else
      (yyval.intval) = rval;
  }
#line 2274 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 61: /* logical_exp: DWITHIN '(' shape_exp ',' shape_exp ',' math_exp ')'  */
#line 689 "src/mapparser.y"
                                                         {
    double d;
    d = msGEOSDistance((yyvsp[-5].shpval), (yyvsp[-3].shpval));
    if((yyvsp[-5].shpval) && (yyvsp[-5].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-5].shpval));
      free((yyvsp[-5].shpval));
    }
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if(d <= (yyvsp[-1].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 2295 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 62: /* logical_exp: BEYOND '(' shape_exp ',' shape_exp ',' math_exp ')'  */
#line 705 "src/mapparser.y"
                                                        {
    double d;
    d = msGEOSDistance((yyvsp[-5].shpval), (yyvsp[-3].shpval));
    if((yyvsp[-5].shpval) && (yyvsp[-5].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-5].shpval));
      free((yyvsp[-5].shpval));
    }
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if(d > (yyvsp[-1].dblval))
      (yyval.intval) = MS_TRUE;
    else
      (yyval.intval) = MS_FALSE;
  }
#line 2316 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 64: /* math_exp: '(' math_exp ')'  */
#line 724 "src/mapparser.y"
                     { (yyval.dblval) = (yyvsp[-1].dblval); }
#line 2322 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 65: /* math_exp: math_exp '+' math_exp  */
#line 725 "src/mapparser.y"
                          { (yyval.dblval) = (yyvsp[-2].dblval) + (yyvsp[0].dblval); }
#line 2328 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 66: /* math_exp: math_exp '-' math_exp  */
#line 726 "src/mapparser.y"
                          { (yyval.dblval) = (yyvsp[-2].dblval) - (yyvsp[0].dblval); }
#line 2334 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 67: /* math_exp: math_exp '*' math_exp  */
#line 727 "src/mapparser.y"
                          { (yyval.dblval) = (yyvsp[-2].dblval) * (yyvsp[0].dblval); }
#line 2340 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 68: /* math_exp: math_exp '%' math_exp  */
#line 728 "src/mapparser.y"
                          { (yyval.dblval) = (int)(yyvsp[-2].dblval) % (int)(yyvsp[0].dblval); }
#line 2346 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 69: /* math_exp: math_exp '/' math_exp  */
#line 729 "src/mapparser.y"
                          {
    if((yyvsp[0].dblval) == 0.0) {
      yyerror(p, "Division by zero.");
      return(-1);
    } else
      (yyval.dblval) = (yyvsp[-2].dblval) / (yyvsp[0].dblval); 
  }
#line 2358 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 70: /* math_exp: '-' math_exp  */
#line 736 "src/mapparser.y"
                           { (yyval.dblval) = (yyvsp[0].dblval); }
#line 2364 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 71: /* math_exp: math_exp '^' math_exp  */
#line 737 "src/mapparser.y"
                          { (yyval.dblval) = pow((yyvsp[-2].dblval), (yyvsp[0].dblval)); }
#line 2370 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 72: /* math_exp: LENGTH '(' string_exp ')'  */
#line 738 "src/mapparser.y"
                              { (yyval.dblval) = strlen((yyvsp[-1].strval)); }
#line 2376 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 73: /* math_exp: AREA '(' shape_exp ')'  */
#line 739 "src/mapparser.y"
                           {
    if((yyvsp[-1].shpval)->type != MS_SHAPE_POLYGON) {
      yyerror(p, "Area can only be computed for polygon shapes.");
      return(-1);
    }
    (yyval.dblval) = msGetPolygonArea((yyvsp[-1].shpval));
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
  }
#line 2392 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 74: /* math_exp: ROUND '(' math_exp ',' math_exp ')'  */
#line 750 "src/mapparser.y"
                                        { (yyval.dblval) = (MS_NINT((yyvsp[-3].dblval)/(yyvsp[-1].dblval)))*(yyvsp[-1].dblval); }
#line 2398 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 75: /* math_exp: ROUND '(' math_exp ')'  */
#line 751 "src/mapparser.y"
                           { (yyval.dblval) = (MS_NINT((yyvsp[-1].dblval))); }
#line 2404 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 77: /* shape_exp: '(' shape_exp ')'  */
#line 755 "src/mapparser.y"
                      { (yyval.shpval) = (yyvsp[-1].shpval); }
#line 2410 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 78: /* shape_exp: YYBUFFER '(' shape_exp ',' math_exp ')'  */
#line 756 "src/mapparser.y"
                                            {
    shapeObj *s;
    s = msGEOSBuffer((yyvsp[-3].shpval), (yyvsp[-1].dblval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if(!s) {
      yyerror(p, "Executing buffer failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2429 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 79: /* shape_exp: INNER '(' shape_exp ')'  */
#line 770 "src/mapparser.y"
                            {
    shapeObj *s;
    s = msRings2Shape((yyvsp[-1].shpval), MS_FALSE);
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(!s) {
      yyerror(p, "Executing inner failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2448 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 80: /* shape_exp: OUTER '(' shape_exp ')'  */
#line 784 "src/mapparser.y"
                            {
    shapeObj *s;
    s = msRings2Shape((yyvsp[-1].shpval), MS_TRUE);
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(!s) {
      yyerror(p, "Executing outer failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2467 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 81: /* shape_exp: CENTERLINE '(' shape_exp ')'  */
#line 798 "src/mapparser.y"
                                 {
    shapeObj *s;
    s = msGEOSCenterline((yyvsp[-1].shpval));
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(!s) {
      yyerror(p, "Executing centerline failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2486 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 82: /* shape_exp: DIFFERENCE '(' shape_exp ',' shape_exp ')'  */
#line 812 "src/mapparser.y"
                                               {
    shapeObj *s;
    s = msGEOSDifference((yyvsp[-3].shpval), (yyvsp[-1].shpval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if(!s) {
      yyerror(p, "Executing difference failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2505 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 83: /* shape_exp: DENSIFY '(' shape_exp ',' math_exp ')'  */
#line 826 "src/mapparser.y"
                                           {
    shapeObj *s;
    s = msDensify((yyvsp[-3].shpval), (yyvsp[-1].dblval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if(!s) {
      yyerror(p, "Executing densify failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2524 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 84: /* shape_exp: SIMPLIFY '(' shape_exp ',' math_exp ')'  */
#line 840 "src/mapparser.y"
                                            {
    shapeObj *s;
    s = msGEOSSimplify((yyvsp[-3].shpval), (yyvsp[-1].dblval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if(!s) {
      yyerror(p, "Executing simplify failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2543 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 85: /* shape_exp: SIMPLIFYPT '(' shape_exp ',' math_exp ')'  */
#line 854 "src/mapparser.y"
                                              {
    shapeObj *s;
    s = msGEOSTopologyPreservingSimplify((yyvsp[-3].shpval), (yyvsp[-1].dblval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if(!s) {
      yyerror(p, "Executing simplifypt failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2562 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 86: /* shape_exp: GENERALIZE '(' shape_exp ',' math_exp ')'  */
#line 868 "src/mapparser.y"
                                              {
    shapeObj *s;
    s = msGeneralize((yyvsp[-3].shpval), (yyvsp[-1].dblval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if(!s) {
      yyerror(p, "Executing generalize failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2581 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 87: /* shape_exp: SMOOTHSIA '(' shape_exp ')'  */
#line 882 "src/mapparser.y"
                                {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[-1].shpval), 3, 1, NULL);
    if((yyvsp[-1].shpval) && (yyvsp[-1].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-1].shpval));
      free((yyvsp[-1].shpval));
    }
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2600 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 88: /* shape_exp: SMOOTHSIA '(' shape_exp ',' math_exp ')'  */
#line 896 "src/mapparser.y"
                                             {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[-3].shpval), (yyvsp[-1].dblval), 1, NULL);
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2619 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 89: /* shape_exp: SMOOTHSIA '(' shape_exp ',' math_exp ',' math_exp ')'  */
#line 910 "src/mapparser.y"
                                                          {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[-5].shpval), (yyvsp[-3].dblval), (yyvsp[-1].dblval), NULL);
    if((yyvsp[-5].shpval) && (yyvsp[-5].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-5].shpval));
      free((yyvsp[-5].shpval));
    }
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2638 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 90: /* shape_exp: SMOOTHSIA '(' shape_exp ',' math_exp ',' math_exp ',' string_exp ')'  */
#line 924 "src/mapparser.y"
                                                                         {
    shapeObj *s;
    s = msSmoothShapeSIA((yyvsp[-7].shpval), (yyvsp[-5].dblval), (yyvsp[-3].dblval), (yyvsp[-1].strval));
    if((yyvsp[-7].shpval) && (yyvsp[-7].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-7].shpval));
      free((yyvsp[-7].shpval));
    }
    msReplaceFreeableStr(&((yyvsp[-1].strval)), NULL);
    if(!s) {
      yyerror(p, "Executing smoothsia failed.");
      return(-1);
    }
    s->scratch = MS_TRUE;
    (yyval.shpval) = s;
  }
#line 2658 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 91: /* shape_exp: JAVASCRIPT '(' shape_exp ',' string_exp ')'  */
#line 939 "src/mapparser.y"
                                                {
#ifdef USE_V8_MAPSCRIPT
    shapeObj *s;
    s = msV8TransformShape((yyvsp[-3].shpval), (yyvsp[-1].strval));
    if((yyvsp[-3].shpval) && (yyvsp[-3].shpval)->scratch == MS_TRUE) {
      msFreeShape((yyvsp[-3].shpval));
      free((yyvsp[-3].shpval));
    }
    msReplaceFreeableStr(&((yyvsp[-1].strval)), NULL);
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
#line 2683 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 93: /* string_exp: '(' string_exp ')'  */
#line 962 "src/mapparser.y"
                       { (yyval.strval) = (yyvsp[-1].strval); }
#line 2689 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 94: /* string_exp: string_exp '+' string_exp  */
#line 963 "src/mapparser.y"
                              { 
    (yyval.strval) = (char *)malloc(strlen((yyvsp[-2].strval)) + strlen((yyvsp[0].strval)) + 1);
    sprintf((yyval.strval), "%s%s", (yyvsp[-2].strval), (yyvsp[0].strval));
    msReplaceFreeableStr(&((yyvsp[-2].strval)), NULL);
    msReplaceFreeableStr(&((yyvsp[0].strval)), NULL);
  }
#line 2700 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 95: /* string_exp: TOSTRING '(' math_exp ',' string_exp ')'  */
#line 969 "src/mapparser.y"
                                             {
    char* ret = msToString((yyvsp[-1].strval), (yyvsp[-3].dblval));
    msReplaceFreeableStr(&((yyvsp[-1].strval)), NULL);
    if(!ret) {
      yyerror(p, "tostring() failed.");
      return(-1);
    }
    (yyval.strval) = ret;
  }
#line 2714 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 96: /* string_exp: COMMIFY '(' string_exp ')'  */
#line 978 "src/mapparser.y"
                               {  
    (yyvsp[-1].strval) = msCommifyString((yyvsp[-1].strval)); 
    (yyval.strval) = (yyvsp[-1].strval); 
  }
#line 2723 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 97: /* string_exp: UPPER '(' string_exp ')'  */
#line 982 "src/mapparser.y"
                             {  
    msStringToUpper((yyvsp[-1].strval)); 
    (yyval.strval) = (yyvsp[-1].strval); 
  }
#line 2732 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 98: /* string_exp: LOWER '(' string_exp ')'  */
#line 986 "src/mapparser.y"
                             {  
    msStringToLower((yyvsp[-1].strval)); 
    (yyval.strval) = (yyvsp[-1].strval); 
  }
#line 2741 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 99: /* string_exp: INITCAP '(' string_exp ')'  */
#line 990 "src/mapparser.y"
                               {  
    msStringInitCap((yyvsp[-1].strval)); 
    (yyval.strval) = (yyvsp[-1].strval); 
  }
#line 2750 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 100: /* string_exp: FIRSTCAP '(' string_exp ')'  */
#line 994 "src/mapparser.y"
                                {  
    msStringFirstCap((yyvsp[-1].strval)); 
    (yyval.strval) = (yyvsp[-1].strval); 
  }
#line 2759 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;

  case 102: /* time_exp: '(' time_exp ')'  */
#line 1001 "src/mapparser.y"
                     { (yyval.tmval) = (yyvsp[-1].tmval); }
#line 2765 "/Users/gfabbian/projects/MapServer/src/mapparser.c"
    break;


#line 2769 "/Users/gfabbian/projects/MapServer/src/mapparser.c"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (p, YY_("syntax error"));
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, p);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (p, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
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
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, p);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1004 "src/mapparser.y"


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
