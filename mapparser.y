/*
** Parser for the mapserver
*/

%{
/* C declarations */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "mapserver.h" /* for TRUE/FALSE and REGEX includes */
#include "maptime.h" /* for time comparison routines */

extern int msyylex(void); /* lexer globals */
extern int msyyerror(const char *);

int msyyresult;
%}

/* Bison/Yacc declarations */

%union {
  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
}

%token <dblval> NUMBER
%token <strval> STRING
%token <strval> ISTRING
%token <tmval> TIME
%token <strval> REGEX
%token <strval> IREGEX
%left OR
%left AND
%left NOT
%left RE EQ NE LT GT LE GE IN IEQ
%left LENGTH
%left '+' '-'
%left '*' '/' '%'
%left NEG
%right '^'

%type <intval> logical_exp
%type <dblval> math_exp
%type <strval> string_exp
%type <strval> regular_exp
%type <tmval> time_exp

/* Bison/Yacc grammar */

%%

input: /* empty string */
       | logical_exp      { 
			    msyyresult = $1; 
			  }
       | math_exp	  {
			    if($1 != 0)
			      msyyresult = MS_TRUE;
			    else
			      msyyresult = MS_FALSE;			    
			  }
;

regular_exp: REGEX ;

logical_exp:
       logical_exp OR logical_exp      {
	                                 if($1 == MS_TRUE)
		                           $$ = MS_TRUE;
		                         else if($3 == MS_TRUE)
			                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | logical_exp AND logical_exp   {
	                                 if($1 == MS_TRUE) {
			                   if($3 == MS_TRUE)
			                     $$ = MS_TRUE;
			                   else
			                     $$ = MS_FALSE;
			                 } else
			                   $$ = MS_FALSE;
		                       }
       | logical_exp OR math_exp       {
	                                 if($1 == MS_TRUE)
		                           $$ = MS_TRUE;
		                         else if($3 != 0)
			                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | logical_exp AND math_exp      {
	                                 if($1 == MS_TRUE) {
			                   if($3 != 0)
			                     $$ = MS_TRUE;
			                   else
			                     $$ = MS_FALSE;
			                 } else
			                   $$ = MS_FALSE;
		                       }
       | math_exp OR logical_exp       {
	                                 if($1 != 0)
		                           $$ = MS_TRUE;
		                         else if($3 == MS_TRUE)
			                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | math_exp AND logical_exp      {
	                                 if($1 != 0) {
			                   if($3 == MS_TRUE)
			                     $$ = MS_TRUE;
			                   else
			                     $$ = MS_FALSE;
			                 } else
			                   $$ = MS_FALSE;
		                       }
       | math_exp OR math_exp       {
	                                 if($1 != 0)
		                           $$ = MS_TRUE;
		                         else if($3 != 0)
			                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | math_exp AND math_exp      {
	                                 if($1 != 0) {
			                   if($3 != 0)
			                     $$ = MS_TRUE;
			                   else
			                     $$ = MS_FALSE;
			                 } else
			                   $$ = MS_FALSE;
		                       }
       | NOT logical_exp	       { $$ = !$2; }
       | NOT math_exp	       	       { $$ = !$2; }
       | string_exp RE regular_exp     {
                                         ms_regex_t re;

                                         if(ms_regcomp(&re, $3, MS_REG_EXTENDED|MS_REG_NOSUB) != 0) 
                                           $$ = MS_FALSE;

                                         if(ms_regexec(&re, $1, 0, NULL, 0) == 0)
                                  	   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;

                                         ms_regfree(&re);
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
					   free($1);
					   free($3);
				       }
       | string_exp NE string_exp      {
                                         if(strcmp($1, $3) != 0)
					   $$ = MS_TRUE;
					 else
					   $$ = MS_FALSE;
					   free($1);
					   free($3);
				       }
       | string_exp GT string_exp      {
                                         if(strcmp($1, $3) > 0)
					   $$ = MS_TRUE;
					 else
					   $$ = MS_FALSE;
					   /* printf("Not freeing: %s >= %s\n",$1, $3); */
					   free($1);
					   free($3);
                                       }
       | string_exp LT string_exp      {
                                         if(strcmp($1, $3) < 0)
					   $$ = MS_TRUE;
					 else
					   $$ = MS_FALSE;
					   free($1);
					   free($3);
                                       }
       | string_exp GE string_exp      {
                                         if(strcmp($1, $3) >= 0)
					   $$ = MS_TRUE;
					 else
					   $$ = MS_FALSE;
					   free($1);
					   free($3);
                                       }
       | string_exp LE string_exp      {
                                         if(strcmp($1, $3) <= 0)
					   $$ = MS_TRUE;
					 else
					   $$ = MS_FALSE;
					   free($1);
					   free($3);
                                       } 
       | time_exp EQ time_exp      {
                                     if(msTimeCompare(&($1), &($3)) == 0)
				       $$ = MS_TRUE;
				     else
				       $$ = MS_FALSE;
				   }
       | time_exp NE time_exp      {
                                     if(msTimeCompare(&($1), &($3)) != 0)
				       $$ = MS_TRUE;
				     else
				       $$ = MS_FALSE;
				   }
       | time_exp GT time_exp      {
                                     if(msTimeCompare(&($1), &($3)) > 0)
				       $$ = MS_TRUE;
				     else
				       $$ = MS_FALSE;
                                   }
       | time_exp LT time_exp      {
                                     if(msTimeCompare(&($1), &($3)) < 0)
				       $$ = MS_TRUE;
				     else
				       $$ = MS_FALSE;
                                   }
       | time_exp GE time_exp      {
                                     if(msTimeCompare(&($1), &($3)) >= 0)
				       $$ = MS_TRUE;
				     else
				       $$ = MS_FALSE;
                                   }
       | time_exp LE time_exp      {
                                     if(msTimeCompare(&($1), &($3)) <= 0)
				       $$ = MS_TRUE;
				     else
				       $$ = MS_FALSE;
                                   }
       | string_exp IN string_exp      {
					 char *delim,*bufferp;

					 $$ = MS_FALSE;
					 bufferp=$3;

					 while((delim=strchr(bufferp,',')) != NULL) {
					   *delim='\0';
					   if(strcmp($1,bufferp) == 0) {
					     $$ = MS_TRUE;
					     break;
					   } 
					   *delim=',';
					   bufferp=delim+1;
					 }

					 if(strcmp($1,bufferp) == 0) // is this test necessary?
					   $$ = MS_TRUE;
					   free($1);
					   free($3);
				       }
       | math_exp IN string_exp        {
					 char *delim,*bufferp;

					 $$ = MS_FALSE;
					 bufferp=$3;

					 while((delim=strchr(bufferp,',')) != NULL) {
					   *delim='\0';
					   if($1 == atof(bufferp)) {
					     $$ = MS_TRUE;
					     break;
					   } 
					   *delim=',';
					   bufferp=delim+1;
					 }

					 if($1 == atof(bufferp)) // is this test necessary?
					   $$ = MS_TRUE;
					   
					   free($3);
				       }
       | math_exp IEQ math_exp         {
	                                 if($1 == $3)
	 		                   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;
		                       }
       | string_exp IEQ string_exp     {
                                         if(strcasecmp($1, $3) == 0)
					   $$ = MS_TRUE;
					 else
					   $$ = MS_FALSE;
					   free($1);
					   free($3);
				       }
       | time_exp IEQ time_exp     {
                                     if(msTimeCompare(&($1), &($3)) == 0)
				       $$ = MS_TRUE;
				     else
				       $$ = MS_FALSE;
				   }
;

math_exp: NUMBER
       | math_exp '+' math_exp   { $$ = $1 + $3; }
       | math_exp '-' math_exp   { $$ = $1 - $3; }
       | math_exp '*' math_exp   { $$ = $1 * $3; }
       | math_exp '%' math_exp   { $$ = (int)$1 % (int)$3; }
       | math_exp '/' math_exp   {
	         if($3 == 0.0) {
				     msyyerror("division by zero");
				     return(-1);
				   } else
	                             $$ = $1 / $3; 
				 }
       | '-' math_exp %prec NEG  { $$ = $2; }
       | math_exp '^' math_exp   { $$ = pow($1, $3); }
       | '(' math_exp ')'        { $$ = $2; }
       | LENGTH '(' string_exp ')' { $$ = strlen($3); }
;

string_exp: STRING
            | '(' string_exp ')'        { $$ = $2; free($2); }
            | string_exp '+' string_exp { sprintf($$, "%s%s", $1, $3); free($1); free($3); }
;

time_exp: TIME
          | '(' time_exp ')'        { $$ = $2; }
;

%%
