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
     NUMBER = 258,
     STRING = 259,
     ISTRING = 260,
     TIME = 261,
     REGEX = 262,
     IREGEX = 263,
     OR = 264,
     AND = 265,
     NOT = 266,
     IEQ = 267,
     IN = 268,
     GE = 269,
     LE = 270,
     GT = 271,
     LT = 272,
     NE = 273,
     EQ = 274,
     RE = 275,
     LENGTH = 276,
     NEG = 277
   };
#endif
/* Tokens.  */
#define NUMBER 258
#define STRING 259
#define ISTRING 260
#define TIME 261
#define REGEX 262
#define IREGEX 263
#define OR 264
#define AND 265
#define NOT 266
#define IEQ 267
#define IN 268
#define GE 269
#define LE 270
#define GT 271
#define LT 272
#define NE 273
#define EQ 274
#define RE 275
#define LENGTH 276
#define NEG 277




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 24 "mapparser.y"
{
  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
}
/* Line 1489 of yacc.c.  */
#line 100 "mapparser.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE msyylval;

