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

#ifndef YY_CQL2TEXT_CQL2TEXTPARSER_HPP_INCLUDED
# define YY_CQL2TEXT_CQL2TEXTPARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int cql2textdebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    END = 0,                       /* "end of string"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    CQL2_TOK_VALUE_START = 258,    /* CQL2_TOK_VALUE_START  */
    CQL2_TOK_INTEGER_NUMBER = 259, /* "integer number"  */
    CQL2_TOK_DOUBLE_NUMBER = 260,  /* "floating point number"  */
    CQL2_TOK_STRING = 261,         /* "string"  */
    CQL2_TOK_IDENTIFIER = 262,     /* "identifier"  */
    CQL2_TOK_WKT = 263,            /* "wkt geometry"  */
    CQL2_TOK_NOT = 264,            /* "NOT"  */
    CQL2_TOK_OR = 265,             /* "OR"  */
    CQL2_TOK_AND = 266,            /* "AND"  */
    CQL2_TOK_DIV = 267,            /* "DIV"  */
    CQL2_TOK_IS = 268,             /* "IS"  */
    CQL2_TOK_NULL = 269,           /* "NULL"  */
    CQL2_TOK_BOOLEAN_LITERAL = 270, /* "true/false"  */
    CQL2_TOK_CASEI = 271,          /* "CASEI"  */
    CQL2_TOK_S_INTERSECTS = 272,   /* "S_INTERSECTS"  */
    CQL2_TOK_S_EQUALS = 273,       /* "S_EQUALS"  */
    CQL2_TOK_S_DISJOINT = 274,     /* "S_DISJOINT"  */
    CQL2_TOK_S_TOUCHES = 275,      /* "S_TOUCHES"  */
    CQL2_TOK_S_WITHIN = 276,       /* "S_WITHIN"  */
    CQL2_TOK_S_OVERLAPS = 277,     /* "S_OVERLAPS"  */
    CQL2_TOK_S_CROSSES = 278,      /* "S_CROSSES"  */
    CQL2_TOK_S_CONTAINS = 279,     /* "S_CONTAINS"  */
    CQL2_TOK_BETWEEN = 280,        /* CQL2_TOK_BETWEEN  */
    CQL2_TOK_IN = 281,             /* CQL2_TOK_IN  */
    CQL2_TOK_LIKE = 282,           /* CQL2_TOK_LIKE  */
    CQL2_TOK_UMINUS = 283          /* CQL2_TOK_UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif




int cql2textparse (cql2text_parse_context *context);


#endif /* !YY_CQL2TEXT_CQL2TEXTPARSER_HPP_INCLUDED  */
