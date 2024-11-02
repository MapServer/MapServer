/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_SRC_MAPPARSER_H_INCLUDED
# define YY_YY_SRC_MAPPARSER_H_INCLUDED
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
    INNER = 296,
    OUTER = 297,
    DIFFERENCE = 298,
    DENSIFY = 299,
    SIMPLIFY = 300,
    SIMPLIFYPT = 301,
    GENERALIZE = 302,
    SMOOTHSIA = 303,
    CENTERLINE = 304,
    JAVASCRIPT = 305,
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
#line 30 "src/mapparser.y"

  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
  shapeObj *shpval;

#line 167 "src/mapparser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (parseObj *p);

#endif /* !YY_YY_SRC_MAPPARSER_H_INCLUDED  */
