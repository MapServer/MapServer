/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_HOME_RUNNER_WORK_MAPSERVER_MAPSERVER_SRC_MAPPARSER_H_INCLUDED
# define YY_YY_HOME_RUNNER_WORK_MAPSERVER_MAPSERVER_SRC_MAPPARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    BOOLEAN = 258,                 /* BOOLEAN  */
    NUMBER = 259,                  /* NUMBER  */
    STRING = 260,                  /* STRING  */
    TIME = 261,                    /* TIME  */
    SHAPE = 262,                   /* SHAPE  */
    OR = 263,                      /* OR  */
    AND = 264,                     /* AND  */
    NOT = 265,                     /* NOT  */
    RE = 266,                      /* RE  */
    EQ = 267,                      /* EQ  */
    NE = 268,                      /* NE  */
    LT = 269,                      /* LT  */
    GT = 270,                      /* GT  */
    LE = 271,                      /* LE  */
    GE = 272,                      /* GE  */
    IN = 273,                      /* IN  */
    IEQ = 274,                     /* IEQ  */
    IRE = 275,                     /* IRE  */
    INTERSECTS = 276,              /* INTERSECTS  */
    DISJOINT = 277,                /* DISJOINT  */
    TOUCHES = 278,                 /* TOUCHES  */
    OVERLAPS = 279,                /* OVERLAPS  */
    CROSSES = 280,                 /* CROSSES  */
    WITHIN = 281,                  /* WITHIN  */
    CONTAINS = 282,                /* CONTAINS  */
    EQUALS = 283,                  /* EQUALS  */
    BEYOND = 284,                  /* BEYOND  */
    DWITHIN = 285,                 /* DWITHIN  */
    AREA = 286,                    /* AREA  */
    LENGTH = 287,                  /* LENGTH  */
    COMMIFY = 288,                 /* COMMIFY  */
    ROUND = 289,                   /* ROUND  */
    UPPER = 290,                   /* UPPER  */
    LOWER = 291,                   /* LOWER  */
    INITCAP = 292,                 /* INITCAP  */
    FIRSTCAP = 293,                /* FIRSTCAP  */
    TOSTRING = 294,                /* TOSTRING  */
    YYBUFFER = 295,                /* YYBUFFER  */
    INNER = 296,                   /* INNER  */
    OUTER = 297,                   /* OUTER  */
    DIFFERENCE = 298,              /* DIFFERENCE  */
    DENSIFY = 299,                 /* DENSIFY  */
    SIMPLIFY = 300,                /* SIMPLIFY  */
    SIMPLIFYPT = 301,              /* SIMPLIFYPT  */
    GENERALIZE = 302,              /* GENERALIZE  */
    SMOOTHSIA = 303,               /* SMOOTHSIA  */
    CENTERLINE = 304,              /* CENTERLINE  */
    JAVASCRIPT = 305,              /* JAVASCRIPT  */
    NEG = 306                      /* NEG  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
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
#define INNER 296
#define OUTER 297
#define DIFFERENCE 298
#define DENSIFY 299
#define SIMPLIFY 300
#define SIMPLIFYPT 301
#define GENERALIZE 302
#define SMOOTHSIA 303
#define CENTERLINE 304
#define JAVASCRIPT 305
#define NEG 306

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 29 "src/mapparser.y"

  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
  shapeObj *shpval;

#line 177 "/home/runner/work/MapServer/MapServer/src/mapparser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif




int yyparse (parseObj *p);


#endif /* !YY_YY_HOME_RUNNER_WORK_MAPSERVER_MAPSERVER_SRC_MAPPARSER_H_INCLUDED  */
