/*
** Parser for the mapserver
*/

%{

/* C declarations */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "map.h" // for TRUE/FALSE

extern int msyylex(); /* lexer globals */
extern int msyyerror();

int msyyresult;
%}

/* Bison/Yacc declarations */

%union {
  double dblval;
  int intval;  
  char *strval;
}

%token <dblval> NUMBER
%token <strval> STRING
%left OR
%left AND
%left EQ NE LT GT LE GE
%left LENGTH
%left '+' '-'
%left '*' '/'
%left NEG
%right '^'

%type <intval> logical_exp
%type <dblval> math_exp
%type <strval> string_exp

/* Bison/Yacc grammar */

%%

input: /* empty string */
       | logical_exp      { msyyresult = $1; }
;

logical_exp:
       logical_exp OR logical_exp      {
	                                 if($1 == MS_TRUE)
		                           $$ = MS_TRUE;
		                         else
		                           if($3 == MS_TRUE)
			                     $$ = MS_TRUE;
			                   else
			                     $$ = MS_FALSE;
		                       }
       | logical_exp AND logical_exp   {
	                                 if($1 == MS_TRUE)
			                   if($3 == MS_TRUE)
			                     $$ = MS_TRUE;
			                   else
			                     $$ = MS_FALSE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | math_exp EQ math_exp          {
	                                 if($1 == $3)
	 		                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | math_exp NE math_exp          {
	                                 if($1 != $3)
			                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       } 
       | math_exp GT math_exp          {	                                 
	                                 if($1 > $3)
			                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | math_exp LT math_exp          {
	                                 if($1 < $3)
			                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | math_exp GE math_exp          {	                                 
	                                 if($1 >= $3)
			                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | math_exp LE math_exp          {
	                                 if($1 <= $3)
			                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | '(' logical_exp ')'           { $$ = $2; }
       | string_exp EQ string_exp      {
                                         if(strcmp($1, $3) == 0)
					   $$ = MS_TRUE;
					 else
					   $$ = MS_FALSE;
				       }
       | string_exp NE string_exp      {
                                         if(strcmp($1, $3) != 0)
					   $$ = MS_TRUE;
					 else
					   $$ = MS_FALSE;
				       }
;

math_exp: NUMBER
       | math_exp '+' math_exp   { $$ = $1 + $3; }
       | math_exp '-' math_exp   { $$ = $1 - $3; }
       | math_exp '*' math_exp   { $$ = $1 * $3; }
       | math_exp '/' math_exp   {
	                           if($3 == 0.0) {
				     msyyerror("Division by Zero");
				     return(-1);
				   } else
	                             $$ = $1 / $3; 
				 }
       | '-' math_exp %prec NEG  { $$ = $2; }
       | math_exp '^' math_exp   { $$ = pow($1, $3); }
       | '(' math_exp ')'        { $$ = $2; }
       | LENGTH '(' string_exp ')' { $$ = strlen($3) }
;

string_exp: STRING
            | '(' string_exp ')'        { $$ = $2; }
            | string_exp '+' string_exp { sprintf($$, "%s%s", $1, $3); }
;
%%

