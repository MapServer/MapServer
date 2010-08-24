/* A Bison parser, made by GNU Bison 2.0.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

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
     LENGTH = 276,
     NEG = 277
   };
#endif
#define NUMBER 258
#define STRING 259
#define ISTRING 260
#define TIME 261
#define REGEX 262
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
#define LENGTH 276
#define NEG 277




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 24 "mapparser.y"
typedef union YYSTYPE {
  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
} YYSTYPE;
/* Line 1274 of yacc.c.  */
#line 88 "mapparser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE msyylval;



