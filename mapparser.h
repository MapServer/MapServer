/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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
/* Line 1529 of yacc.c.  */
#line 159 "/Users/sdlime/mapserver/sdlime/mapserver/mapparser.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



