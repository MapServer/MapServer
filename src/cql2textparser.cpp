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


/* Substitute the variable and function names.  */
#define yyparse         cql2textparse
#define yylex           cql2textlex
#define yyerror         cql2texterror
#define yydebug         cql2textdebug
#define yynerrs         cql2textnerrs

/* First part of user prologue.  */

/******************************************************************************
 *
 * Project: MapServer
 * Purpose: CQL2 text expression parser grammar.
 *          Requires Bison 3.0.0 or newer to process.  Use "cql2textparser" target.
 * Author: Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (C) 2026 Even Rouault <even dot rouault at spatialys dot com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

// Inspired from GDAL SQL parser: https://github.com/OSGeo/gdal/blob/master/ogr/swq_parser.y
 
#include "cql2text.h"

#define YYSTYPE cql2_expr_node *

/* Defining YYSTYPE_IS_TRIVIAL is needed because the parser is generated as a C++ file. */
/* See http://www.gnu.org/s/bison/manual/html_node/Memory-Management.html that suggests */
/* increase YYINITDEPTH instead, but this will consume memory. */
/* Setting YYSTYPE_IS_TRIVIAL overcomes this limitation, but might be fragile because */
/* it appears to be a non documented feature of Bison */
#define YYSTYPE_IS_TRIVIAL 1


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

#include "cql2textparser.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of string"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_CQL2_TOK_VALUE_START = 3,       /* CQL2_TOK_VALUE_START  */
  YYSYMBOL_CQL2_TOK_INTEGER_NUMBER = 4,    /* "integer number"  */
  YYSYMBOL_CQL2_TOK_DOUBLE_NUMBER = 5,     /* "floating point number"  */
  YYSYMBOL_CQL2_TOK_STRING = 6,            /* "string"  */
  YYSYMBOL_CQL2_TOK_IDENTIFIER = 7,        /* "identifier"  */
  YYSYMBOL_CQL2_TOK_WKT = 8,               /* "wkt geometry"  */
  YYSYMBOL_CQL2_TOK_NOT = 9,               /* "NOT"  */
  YYSYMBOL_CQL2_TOK_OR = 10,               /* "OR"  */
  YYSYMBOL_CQL2_TOK_AND = 11,              /* "AND"  */
  YYSYMBOL_CQL2_TOK_DIV = 12,              /* "DIV"  */
  YYSYMBOL_CQL2_TOK_IS = 13,               /* "IS"  */
  YYSYMBOL_CQL2_TOK_NULL = 14,             /* "NULL"  */
  YYSYMBOL_CQL2_TOK_BOOLEAN_LITERAL = 15,  /* "true/false"  */
  YYSYMBOL_CQL2_TOK_CASEI = 16,            /* "CASEI"  */
  YYSYMBOL_CQL2_TOK_S_INTERSECTS = 17,     /* "S_INTERSECTS"  */
  YYSYMBOL_CQL2_TOK_S_EQUALS = 18,         /* "S_EQUALS"  */
  YYSYMBOL_CQL2_TOK_S_DISJOINT = 19,       /* "S_DISJOINT"  */
  YYSYMBOL_CQL2_TOK_S_TOUCHES = 20,        /* "S_TOUCHES"  */
  YYSYMBOL_CQL2_TOK_S_WITHIN = 21,         /* "S_WITHIN"  */
  YYSYMBOL_CQL2_TOK_S_OVERLAPS = 22,       /* "S_OVERLAPS"  */
  YYSYMBOL_CQL2_TOK_S_CROSSES = 23,        /* "S_CROSSES"  */
  YYSYMBOL_CQL2_TOK_S_CONTAINS = 24,       /* "S_CONTAINS"  */
  YYSYMBOL_25_ = 25,                       /* '='  */
  YYSYMBOL_26_ = 26,                       /* '<'  */
  YYSYMBOL_27_ = 27,                       /* '>'  */
  YYSYMBOL_28_ = 28,                       /* '!'  */
  YYSYMBOL_CQL2_TOK_BETWEEN = 29,          /* CQL2_TOK_BETWEEN  */
  YYSYMBOL_CQL2_TOK_IN = 30,               /* CQL2_TOK_IN  */
  YYSYMBOL_CQL2_TOK_LIKE = 31,             /* CQL2_TOK_LIKE  */
  YYSYMBOL_32_ = 32,                       /* '+'  */
  YYSYMBOL_33_ = 33,                       /* '-'  */
  YYSYMBOL_34_ = 34,                       /* '*'  */
  YYSYMBOL_35_ = 35,                       /* '/'  */
  YYSYMBOL_36_ = 36,                       /* '%'  */
  YYSYMBOL_CQL2_TOK_UMINUS = 37,           /* CQL2_TOK_UMINUS  */
  YYSYMBOL_38_ = 38,                       /* '('  */
  YYSYMBOL_39_ = 39,                       /* ')'  */
  YYSYMBOL_40_ = 40,                       /* ','  */
  YYSYMBOL_YYACCEPT = 41,                  /* $accept  */
  YYSYMBOL_input = 42,                     /* input  */
  YYSYMBOL_boolean_expr = 43,              /* boolean_expr  */
  YYSYMBOL_value_expr_list = 44,           /* value_expr_list  */
  YYSYMBOL_value_expr_or_boolean_literal = 45, /* value_expr_or_boolean_literal  */
  YYSYMBOL_value_expr = 46                 /* value_expr  */
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
typedef yytype_uint8 yy_state_t;

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

#if 1

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
#endif /* 1 */

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
#define YYFINAL  24
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   399

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  41
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  6
/* YYNRULES -- Number of rules.  */
#define YYNRULES  53
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  162

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   283


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
       2,     2,     2,    28,     2,     2,     2,    36,     2,     2,
      38,    39,    34,    32,    40,    33,     2,    35,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      26,    25,    27,     2,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      29,    30,    31,    37
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    85,    85,    86,    93,   104,   115,   127,   140,   153,
     166,   179,   191,   204,   217,   231,   244,   257,   273,   286,
     302,   317,   334,   348,   365,   377,   392,   405,   418,   431,
     444,   457,   470,   483,   496,   501,   507,   519,   533,   535,
     541,   546,   551,   556,   561,   567,   577,   590,   603,   616,
     629,   642,   655,   685
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of string\"", "error", "\"invalid token\"",
  "CQL2_TOK_VALUE_START", "\"integer number\"",
  "\"floating point number\"", "\"string\"", "\"identifier\"",
  "\"wkt geometry\"", "\"NOT\"", "\"OR\"", "\"AND\"", "\"DIV\"", "\"IS\"",
  "\"NULL\"", "\"true/false\"", "\"CASEI\"", "\"S_INTERSECTS\"",
  "\"S_EQUALS\"", "\"S_DISJOINT\"", "\"S_TOUCHES\"", "\"S_WITHIN\"",
  "\"S_OVERLAPS\"", "\"S_CROSSES\"", "\"S_CONTAINS\"", "'='", "'<'", "'>'",
  "'!'", "CQL2_TOK_BETWEEN", "CQL2_TOK_IN", "CQL2_TOK_LIKE", "'+'", "'-'",
  "'*'", "'/'", "'%'", "CQL2_TOK_UMINUS", "'('", "')'", "','", "$accept",
  "input", "boolean_expr", "value_expr_list",
  "value_expr_or_boolean_literal", "value_expr", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-41)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-40)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      20,    97,    25,   -41,   -41,   -41,   -11,   -41,    97,   -21,
      -6,     1,     5,    34,    35,    37,    38,    42,    60,   138,
      97,    46,    72,   250,   -41,   138,   -41,    36,   138,   138,
     138,   138,   138,   138,   138,   138,   138,   138,   -41,    11,
     151,    97,    97,   118,    78,   138,    -7,    41,   132,   138,
      62,   104,   138,   138,   138,   138,   138,    88,    19,   118,
     239,    59,   160,   176,   185,   194,   203,   212,   221,   255,
     -41,   -41,   117,   -41,   -41,   -41,   363,   138,    91,   126,
     -41,   120,   -41,   138,   118,   363,   138,   363,   347,   138,
     -41,    26,    26,   -41,   -41,   -41,   -41,   138,   -41,   122,
     138,   138,   138,   138,   138,   138,   138,   138,   356,   138,
     -41,   -41,   363,   -41,   363,   138,   102,   -41,   119,   133,
     125,   139,   280,   288,   296,   305,   313,   321,   330,   338,
     138,   115,   363,   -41,   142,   121,   145,   124,   -41,   -41,
     -41,   -41,   -41,   -41,   -41,   -41,   363,   -41,   128,   161,
     130,   163,   167,   135,   169,   140,   150,   -41,   152,   -41,
     -41,   -41
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     0,     0,    40,    41,    42,    44,    43,     0,    35,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     3,     0,    38,     1,     0,     6,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    52,     0,
      38,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    37,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      34,    53,     5,     4,    39,     7,    38,     0,     0,     0,
      50,     0,    24,     0,     0,    12,     0,    13,     0,     0,
      16,    46,    47,    48,    49,    51,    45,     0,    10,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      17,    25,    14,     9,    15,     0,     0,    36,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    22,    20,     0,     0,     0,     0,    26,    27,
      28,    29,    30,    31,    32,    33,    23,    21,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     8,     0,    18,
      19,    11
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -41,   -41,     0,   -20,   -40,   -19
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     2,    21,    57,    22,    23
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      38,    40,    81,    75,   -39,    27,    58,    82,    26,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    98,
      39,    41,    42,     1,    76,    24,    80,    25,    85,    87,
      88,    45,    28,    91,    92,    93,    94,    95,    45,    29,
      76,    72,    73,    30,   113,     3,     4,     5,     6,     7,
      70,    52,    53,    54,    55,    56,    41,    42,   108,    97,
      54,    55,    56,    59,   112,    76,    83,   114,    84,   116,
      58,    45,    31,    32,    19,    33,    34,   117,    58,    37,
      35,   122,   123,   124,   125,   126,   127,   128,   129,   131,
      58,    52,    53,    54,    55,    56,   132,    43,    36,   100,
      89,     3,     4,     5,     6,     7,     8,    77,    78,    79,
      90,   146,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,     3,     4,     5,     6,     7,    96,    42,   109,
      19,   118,   110,    74,   111,    20,     3,     4,     5,     6,
       7,   133,     3,     4,     5,     6,     7,   119,   120,   135,
     134,    19,   136,   121,   147,   137,    37,    86,   148,   149,
      44,   150,   151,    45,    46,    19,   152,   153,   154,   155,
      37,    19,    45,   156,   157,   158,    37,    47,    48,   159,
      49,    50,    51,    52,    53,    54,    55,    56,    45,   160,
      71,   161,    52,    53,    54,    55,    56,    45,     0,     0,
     101,     0,     0,     0,     0,     0,    45,     0,    52,    53,
      54,    55,    56,     0,     0,    45,   102,    52,    53,    54,
      55,    56,     0,     0,    45,   103,    52,    53,    54,    55,
      56,     0,     0,    45,   104,    52,    53,    54,    55,    56,
       0,     0,     0,   105,    52,    53,    54,    55,    56,     0,
       0,    45,   106,    52,    53,    54,    55,    56,     0,    44,
       0,   107,    45,    46,     0,     0,     0,    45,     0,     0,
       0,    52,    53,    54,    55,    56,    47,    48,    99,    49,
      50,    51,    52,    53,    54,    55,    56,    52,    53,    54,
      55,    56,    45,     0,    71,     0,     0,     0,     0,     0,
      45,     0,     0,     0,     0,     0,     0,     0,    45,     0,
       0,     0,    52,    53,    54,    55,    56,    45,     0,   138,
      52,    53,    54,    55,    56,    45,     0,   139,    52,    53,
      54,    55,    56,    45,     0,   140,     0,    52,    53,    54,
      55,    56,    45,     0,   141,    52,    53,    54,    55,    56,
      45,     0,   142,    52,    53,    54,    55,    56,   115,    45,
     143,     0,    52,    53,    54,    55,    56,   130,    45,   144,
      52,    53,    54,    55,    56,    45,     0,   145,     0,    52,
      53,    54,    55,    56,     0,     0,     0,     0,    52,    53,
      54,    55,    56,     0,     0,    52,    53,    54,    55,    56
};

static const yytype_int16 yycheck[] =
{
      19,    20,     9,    43,    25,    26,    25,    14,     8,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    59,
      20,    10,    11,     3,    43,     0,    45,    38,    47,    48,
      49,    12,    38,    52,    53,    54,    55,    56,    12,    38,
      59,    41,    42,    38,    84,     4,     5,     6,     7,     8,
      39,    32,    33,    34,    35,    36,    10,    11,    77,    40,
      34,    35,    36,    27,    83,    84,    25,    86,    27,    89,
      89,    12,    38,    38,    33,    38,    38,    97,    97,    38,
      38,   100,   101,   102,   103,   104,   105,   106,   107,   109,
     109,    32,    33,    34,    35,    36,   115,    25,    38,    40,
      38,     4,     5,     6,     7,     8,     9,    29,    30,    31,
       6,   130,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,     4,     5,     6,     7,     8,    39,    11,    38,
      33,     9,     6,    15,    14,    38,     4,     5,     6,     7,
       8,    39,     4,     5,     6,     7,     8,    25,    26,    16,
      31,    33,    27,    31,    39,    16,    38,    25,    16,    38,
       9,    16,    38,    12,    13,    33,    38,     6,    38,     6,
      38,    33,    12,     6,    39,     6,    38,    26,    27,    39,
      29,    30,    31,    32,    33,    34,    35,    36,    12,    39,
      39,    39,    32,    33,    34,    35,    36,    12,    -1,    -1,
      40,    -1,    -1,    -1,    -1,    -1,    12,    -1,    32,    33,
      34,    35,    36,    -1,    -1,    12,    40,    32,    33,    34,
      35,    36,    -1,    -1,    12,    40,    32,    33,    34,    35,
      36,    -1,    -1,    12,    40,    32,    33,    34,    35,    36,
      -1,    -1,    -1,    40,    32,    33,    34,    35,    36,    -1,
      -1,    12,    40,    32,    33,    34,    35,    36,    -1,     9,
      -1,    40,    12,    13,    -1,    -1,    -1,    12,    -1,    -1,
      -1,    32,    33,    34,    35,    36,    26,    27,    39,    29,
      30,    31,    32,    33,    34,    35,    36,    32,    33,    34,
      35,    36,    12,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    12,    -1,
      -1,    -1,    32,    33,    34,    35,    36,    12,    -1,    39,
      32,    33,    34,    35,    36,    12,    -1,    39,    32,    33,
      34,    35,    36,    12,    -1,    39,    -1,    32,    33,    34,
      35,    36,    12,    -1,    39,    32,    33,    34,    35,    36,
      12,    -1,    39,    32,    33,    34,    35,    36,    11,    12,
      39,    -1,    32,    33,    34,    35,    36,    11,    12,    39,
      32,    33,    34,    35,    36,    12,    -1,    39,    -1,    32,
      33,    34,    35,    36,    -1,    -1,    -1,    -1,    32,    33,
      34,    35,    36,    -1,    -1,    32,    33,    34,    35,    36
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,    42,     4,     5,     6,     7,     8,     9,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    33,
      38,    43,    45,    46,     0,    38,    43,    26,    38,    38,
      38,    38,    38,    38,    38,    38,    38,    38,    46,    43,
      46,    10,    11,    25,     9,    12,    13,    26,    27,    29,
      30,    31,    32,    33,    34,    35,    36,    44,    46,    27,
      46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
      39,    39,    43,    43,    15,    45,    46,    29,    30,    31,
      46,     9,    14,    25,    27,    46,    25,    46,    46,    38,
       6,    46,    46,    46,    46,    46,    39,    40,    45,    39,
      40,    40,    40,    40,    40,    40,    40,    40,    46,    38,
       6,    14,    46,    45,    46,    11,    44,    44,     9,    25,
      26,    31,    46,    46,    46,    46,    46,    46,    46,    46,
      11,    44,    46,    39,    31,    16,    27,    16,    39,    39,
      39,    39,    39,    39,    39,    39,    46,    39,    16,    38,
      16,    38,    38,     6,    38,     6,     6,    39,     6,    39,
      39,    39
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    41,    42,    42,    43,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    43,    44,    44,    45,    45,
      46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
      46,    46,    46,    46
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     3,     3,     2,     3,     9,     4,
       4,    10,     3,     3,     4,     4,     3,     4,     9,    10,
       5,     6,     5,     6,     3,     4,     6,     6,     6,     6,
       6,     6,     6,     6,     3,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     4,     3,     3,     3,     3,
       3,     3,     2,     3
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
        yyerror (context, YY_("syntax error: cannot back up")); \
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
                  Kind, Value, context); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, cql2text_parse_context *context)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (context);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, cql2text_parse_context *context)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, context);
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
                 int yyrule, cql2text_parse_context *context)
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
                       &yyvsp[(yyi + 1) - (yynrhs)], context);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, context); \
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


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
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
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
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
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
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
            else
              goto append;

          append:
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

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
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
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
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
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, cql2text_parse_context *context)
{
  YY_USE (yyvaluep);
  YY_USE (context);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_CQL2_TOK_INTEGER_NUMBER: /* "integer number"  */
            { delete (*yyvaluep); }
        break;

    case YYSYMBOL_CQL2_TOK_DOUBLE_NUMBER: /* "floating point number"  */
            { delete (*yyvaluep); }
        break;

    case YYSYMBOL_CQL2_TOK_STRING: /* "string"  */
            { delete (*yyvaluep); }
        break;

    case YYSYMBOL_CQL2_TOK_IDENTIFIER: /* "identifier"  */
            { delete (*yyvaluep); }
        break;

    case YYSYMBOL_CQL2_TOK_WKT: /* "wkt geometry"  */
            { delete (*yyvaluep); }
        break;

    case YYSYMBOL_CQL2_TOK_BOOLEAN_LITERAL: /* "true/false"  */
            { delete (*yyvaluep); }
        break;

    case YYSYMBOL_boolean_expr: /* boolean_expr  */
            { delete (*yyvaluep); }
        break;

    case YYSYMBOL_value_expr_list: /* value_expr_list  */
            { delete (*yyvaluep); }
        break;

    case YYSYMBOL_value_expr_or_boolean_literal: /* value_expr_or_boolean_literal  */
            { delete (*yyvaluep); }
        break;

    case YYSYMBOL_value_expr: /* value_expr  */
            { delete (*yyvaluep); }
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (cql2text_parse_context *context)
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

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

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
      yychar = yylex (&yylval, context);
    }

  if (yychar <= END)
    {
      yychar = END;
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
  case 3: /* input: CQL2_TOK_VALUE_START boolean_expr  */
        {
            context->poRoot = yyvsp[0];
        }
    break;

  case 4: /* boolean_expr: boolean_expr "AND" boolean_expr  */
        {
            yyval = cql2_create_and_or_or( CQL2_AND, yyvsp[-2], yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 5: /* boolean_expr: boolean_expr "OR" boolean_expr  */
        {
            yyval = cql2_create_and_or_or( CQL2_OR, yyvsp[-2], yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 6: /* boolean_expr: "NOT" boolean_expr  */
        {
            yyval = new cql2_expr_node( CQL2_NOT );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 7: /* boolean_expr: value_expr_or_boolean_literal '=' value_expr_or_boolean_literal  */
        {
            yyval = new cql2_expr_node( CQL2_EQ );
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 8: /* boolean_expr: "CASEI" '(' value_expr ')' '=' "CASEI" '(' "string" ')'  */
        {
            yyval = new cql2_expr_node( CQL2_IEQ);
            yyval->PushSubExpression( yyvsp[-6] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 9: /* boolean_expr: value_expr '<' '>' value_expr_or_boolean_literal  */
        {
            yyval = new cql2_expr_node( CQL2_NE );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 10: /* boolean_expr: "true/false" '<' '>' value_expr_or_boolean_literal  */
        {
            yyval = new cql2_expr_node( CQL2_NE );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 11: /* boolean_expr: "CASEI" '(' value_expr ')' '<' '>' "CASEI" '(' "string" ')'  */
        {
            yyval = new cql2_expr_node( CQL2_INE);
            yyval->PushSubExpression( yyvsp[-7] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 12: /* boolean_expr: value_expr '<' value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_LT );
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 13: /* boolean_expr: value_expr '>' value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_GT );
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 14: /* boolean_expr: value_expr '<' '=' value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_LE );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 15: /* boolean_expr: value_expr '>' '=' value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_GE );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 16: /* boolean_expr: value_expr CQL2_TOK_LIKE "string"  */
        {
            yyval = new cql2_expr_node( CQL2_LIKE);
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 17: /* boolean_expr: value_expr "NOT" CQL2_TOK_LIKE "string"  */
        {
            cql2_expr_node *like = new cql2_expr_node( CQL2_LIKE );
            like->PushSubExpression( yyvsp[-3] );
            like->PushSubExpression( yyvsp[0] );

            yyval = new cql2_expr_node( CQL2_NOT );
            yyval->PushSubExpression( like );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 18: /* boolean_expr: "CASEI" '(' value_expr ')' CQL2_TOK_LIKE "CASEI" '(' "string" ')'  */
        {
            yyval = new cql2_expr_node( CQL2_ILIKE);
            yyval->PushSubExpression( yyvsp[-6] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 19: /* boolean_expr: "CASEI" '(' value_expr ')' "NOT" CQL2_TOK_LIKE "CASEI" '(' "string" ')'  */
        {
            cql2_expr_node *like = new cql2_expr_node( CQL2_ILIKE );
            like->PushSubExpression( yyvsp[-7] );
            like->PushSubExpression( yyvsp[-1] );

            yyval = new cql2_expr_node( CQL2_NOT );
            yyval->PushSubExpression( like );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 20: /* boolean_expr: value_expr CQL2_TOK_IN '(' value_expr_list ')'  */
        {
            yyval = yyvsp[-1];
            yyval->m_field_type = CQL2_BOOLEAN;
            yyval->m_op = CQL2_IN;
            yyval->PushSubExpression( yyvsp[-4] );
            yyval->ReverseSubExpressions();
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 21: /* boolean_expr: value_expr "NOT" CQL2_TOK_IN '(' value_expr_list ')'  */
        {
            cql2_expr_node *in = yyvsp[-1];
            in->m_op = CQL2_IN;
            in->PushSubExpression( yyvsp[-5] );
            in->ReverseSubExpressions();

            yyval = new cql2_expr_node( CQL2_NOT );
            yyval->PushSubExpression( in );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 22: /* boolean_expr: value_expr CQL2_TOK_BETWEEN value_expr "AND" value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_BETWEEN );
            yyval->PushSubExpression( yyvsp[-4] );
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 23: /* boolean_expr: value_expr "NOT" CQL2_TOK_BETWEEN value_expr "AND" value_expr  */
        {
            cql2_expr_node *between = new cql2_expr_node( CQL2_BETWEEN );
            between->PushSubExpression( yyvsp[-5] );
            between->PushSubExpression( yyvsp[-2] );
            between->PushSubExpression( yyvsp[0] );

            yyval = new cql2_expr_node( CQL2_NOT );
            yyval->PushSubExpression( between );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 24: /* boolean_expr: value_expr "IS" "NULL"  */
        {
            yyval = new cql2_expr_node( CQL2_IS_NULL );
            yyval->PushSubExpression( yyvsp[-2] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 25: /* boolean_expr: value_expr "IS" "NOT" "NULL"  */
        {
            cql2_expr_node *is_null = new cql2_expr_node( CQL2_IS_NULL );
            is_null->PushSubExpression( yyvsp[-3] );

            yyval = new cql2_expr_node( CQL2_NOT );
            yyval->PushSubExpression( is_null );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 26: /* boolean_expr: "S_INTERSECTS" '(' value_expr ',' value_expr ')'  */
        {
            yyval = new cql2_expr_node( CQL2_S_INTERSECTS );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 27: /* boolean_expr: "S_EQUALS" '(' value_expr ',' value_expr ')'  */
        {
            yyval = new cql2_expr_node( CQL2_S_EQUALS );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 28: /* boolean_expr: "S_DISJOINT" '(' value_expr ',' value_expr ')'  */
        {
            yyval = new cql2_expr_node( CQL2_S_DISJOINT );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 29: /* boolean_expr: "S_TOUCHES" '(' value_expr ',' value_expr ')'  */
        {
            yyval = new cql2_expr_node( CQL2_S_TOUCHES );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 30: /* boolean_expr: "S_WITHIN" '(' value_expr ',' value_expr ')'  */
        {
            yyval = new cql2_expr_node( CQL2_S_WITHIN );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 31: /* boolean_expr: "S_OVERLAPS" '(' value_expr ',' value_expr ')'  */
        {
            yyval = new cql2_expr_node( CQL2_S_OVERLAPS );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 32: /* boolean_expr: "S_CROSSES" '(' value_expr ',' value_expr ')'  */
        {
            yyval = new cql2_expr_node( CQL2_S_CROSSES );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 33: /* boolean_expr: "S_CONTAINS" '(' value_expr ',' value_expr ')'  */
        {
            yyval = new cql2_expr_node( CQL2_S_CONTAINS );
            yyval->PushSubExpression( yyvsp[-3] );
            yyval->PushSubExpression( yyvsp[-1] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 34: /* boolean_expr: '(' boolean_expr ')'  */
        {
            yyval = yyvsp[-1];
        }
    break;

  case 35: /* boolean_expr: "true/false"  */
        {
            yyval = yyvsp[0];
        }
    break;

  case 36: /* value_expr_list: value_expr ',' value_expr_list  */
        {
            yyval = yyvsp[0];
            yyval->PushSubExpression( yyvsp[-2] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 37: /* value_expr_list: value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_ARGUMENT_LIST ); /* temporary value */
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 39: /* value_expr_or_boolean_literal: "true/false"  */
        {
            yyval = yyvsp[0];
        }
    break;

  case 40: /* value_expr: "integer number"  */
        {
            yyval = yyvsp[0];
        }
    break;

  case 41: /* value_expr: "floating point number"  */
        {
            yyval = yyvsp[0];
        }
    break;

  case 42: /* value_expr: "string"  */
        {
            yyval = yyvsp[0];
        }
    break;

  case 43: /* value_expr: "wkt geometry"  */
        {
            yyval = yyvsp[0];
        }
    break;

  case 44: /* value_expr: "identifier"  */
        {
            yyval = yyvsp[0];  // validation deferred.
            yyval->m_node_type = CQL2_NT_COLUMN;
        }
    break;

  case 45: /* value_expr: "identifier" '(' value_expr_list ')'  */
        {
            yyval = yyvsp[-1];
            yyval->m_node_type = CQL2_NT_OPERATION;
            yyval->m_op = CQL2_CUSTOM_FUNC;
            yyval->m_osVal = yyvsp[-3]->m_osVal;
            yyval->ReverseSubExpressions();
            delete yyvsp[-3];
        }
    break;

  case 46: /* value_expr: value_expr '+' value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_ADD );
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 47: /* value_expr: value_expr '-' value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_SUBTRACT );
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 48: /* value_expr: value_expr '*' value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_MULTIPLY );
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 49: /* value_expr: value_expr '/' value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_DIV );
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 50: /* value_expr: value_expr "DIV" value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_DIV );
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 51: /* value_expr: value_expr '%' value_expr  */
        {
            yyval = new cql2_expr_node( CQL2_MOD );
            yyval->PushSubExpression( yyvsp[-2] );
            yyval->PushSubExpression( yyvsp[0] );
            if( yyval->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete yyval;
                YYERROR;
            }
        }
    break;

  case 52: /* value_expr: '-' value_expr  */
        {
            if (yyvsp[0]->m_node_type == CQL2_NT_CONSTANT )
            {
                if( yyvsp[0]->m_field_type == CQL2_INTEGER &&
                    yyvsp[0]->m_nVal == -2147483648 )
                {
                    yyval = yyvsp[0];
                }
                else
                {
                    yyval = yyvsp[0];
                    yyval->m_nVal *= -1;
                    yyval->m_dfVal *= -1;
                }
            }
            else
            {
                yyval = new cql2_expr_node( CQL2_MULTIPLY );
                yyval->PushSubExpression( new cql2_expr_node(-1) );
                yyval->PushSubExpression( yyvsp[0] );
                if( yyval->HasReachedMaxDepth() )
                {
                    yyerror (context, "Maximum expression depth reached");
                    delete yyval;
                    YYERROR;
                }
            }
        }
    break;

  case 53: /* value_expr: '(' value_expr ')'  */
        {
            yyval = yyvsp[-1];
        }
    break;



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
      {
        yypcontext_t yyctx
          = {yyssp, yytoken};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (context, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= END)
        {
          /* Return failure if at end of input.  */
          if (yychar == END)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, context);
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, context);
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
  yyerror (context, YY_("memory exhausted"));
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
                  yytoken, &yylval, context);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, context);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

