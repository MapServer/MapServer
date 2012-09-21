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
#include "mapprimitive.h" /* for shapeObj */

#include "mapparser.h" /* for YYSTYPE in the function prototype for yylex() */

int yylex(YYSTYPE *, parseObj *); /* prototype functions, defined after the grammar */
int yyerror(parseObj *, const char *);
%}

/* Bison/Yacc declarations */

/* %define api.pure */
%pure_parser
%parse-param {parseObj *p}
%lex-param {parseObj *p}

%union {
  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
  shapeObj *shpval;
}

%token <dblval> NUMBER
%token <strval> STRING
%token <tmval> TIME
%token <shpval> SHAPE
%left OR
%left AND
%left NOT
%left RE EQ NE LT GT LE GE IN IEQ IRE
%left INTERSECTS DISJOINT TOUCHES OVERLAPS CROSSES WITHIN CONTAINS BEYOND DWITHIN
%left AREA LENGTH COMMIFY ROUND
%left TOSTRING
%left YYBUFFER DIFFERENCE
%left '+' '-'
%left '*' '/' '%'
%left NEG
%right '^'

%type <intval> logical_exp
%type <dblval> math_exp
%type <strval> string_exp
%type <tmval> time_exp
%type <shpval> shape_exp

/* Bison/Yacc grammar */

%%

input: /* empty string */
  | logical_exp {
      switch(p->type) {
      case(MS_PARSE_TYPE_BOOLEAN):
        p->result.intval = $1; 
        break;
      case(MS_PARSE_TYPE_STRING):
        if($1) 
          p->result.strval = strdup("true");
        else
          p->result.strval = strdup("false");
        break;
      }
    }
  | math_exp {
      switch(p->type) {
      case(MS_PARSE_TYPE_BOOLEAN):
        if($1 != 0)
          p->result.intval = MS_TRUE;
        else
          p->result.intval = MS_FALSE;			    
        break;
      case(MS_PARSE_TYPE_STRING):
        p->result.strval = (char *)malloc(64); /* large enough for a double */
        snprintf(p->result.strval, 64, "%g", $1);
        break;
      }
    }
  | string_exp {
      switch(p->type) {
      case(MS_PARSE_TYPE_BOOLEAN):
        if($1) /* string is not NULL */
          p->result.intval = MS_TRUE;
        else
          p->result.intval = MS_FALSE;
        break;
      case(MS_PARSE_TYPE_STRING):
        p->result.strval = $1; // strdup($1);
        break;
      }
    }
  | shape_exp {
    switch(p->type) {
    case(MS_PARSE_TYPE_SHAPE):
      p->result.shpval = $1;
      p->result.shpval->scratch = MS_FALSE;
      break;
    }
  }
;

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
       | string_exp RE string_exp     {
                                         ms_regex_t re;

                                         if(ms_regcomp(&re, $3, MS_REG_EXTENDED|MS_REG_NOSUB) != 0) 
                                           $$ = MS_FALSE;

                                         if(ms_regexec(&re, $1, 0, NULL, 0) == 0)
                                  	   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;

                                         ms_regfree(&re);
                                         free($1);
                                         free($3);
                                       }
       | string_exp IRE string_exp     {
                                         ms_regex_t re;

                                         if(ms_regcomp(&re, $3, MS_REG_EXTENDED|MS_REG_NOSUB|MS_REG_ICASE) != 0) 
                                           $$ = MS_FALSE;

                                         if(ms_regexec(&re, $1, 0, NULL, 0) == 0)
                                  	   $$ = MS_TRUE;
			                 else
			                   $$ = MS_FALSE;

                                         ms_regfree(&re);
                                         free($1);
                                         free($3);
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

  | shape_exp EQ shape_exp {
      int rval;
      rval = msGEOSEquals($1, $3);
      if($1->scratch == MS_TRUE) msFreeShape($1);
      if($3->scratch == MS_TRUE) msFreeShape($3);
      if(rval == -1) {
        yyerror(p, "Equals (EQ or ==) operator failed.");
        return(-1);
      } else
        $$ = rval;
    }
  | shape_exp INTERSECTS shape_exp {
      int rval;
      rval = msGEOSIntersects($1, $3);      
      if($1->scratch == MS_TRUE) msFreeShape($1);
      if($3->scratch == MS_TRUE) msFreeShape($3); 
      if(rval == -1) {
        yyerror(p, "Intersects operator failed.");
        return(-1);
      } else
        $$ = rval;
    }
  | shape_exp DISJOINT shape_exp {
      int rval;
      rval = msGEOSDisjoint($1, $3);
      if($1->scratch == MS_TRUE) msFreeShape($1);
      if($3->scratch == MS_TRUE) msFreeShape($3);
      if(rval == -1) {
        yyerror(p, "Disjoint operator failed.");
        return(-1);
      } else
        $$ = rval;
    }
  | shape_exp TOUCHES shape_exp {
      int rval;
      rval = msGEOSTouches($1, $3);
      if($1->scratch == MS_TRUE) msFreeShape($1);
      if($3->scratch == MS_TRUE) msFreeShape($3);
      if(rval == -1) {
        yyerror(p, "Touches operator failed.");
        return(-1);
      } else
        $$ = rval;
    }
  | shape_exp OVERLAPS shape_exp {
      int rval;
      rval = msGEOSOverlaps($1, $3);
      if($1->scratch == MS_TRUE) msFreeShape($1);
      if($3->scratch == MS_TRUE) msFreeShape($3);
      if(rval == -1) {
        yyerror(p, "Overlaps operator failed.");
        return(-1);
      } else
        $$ = rval;
    }
  | shape_exp CROSSES shape_exp {
      int rval;
      rval = msGEOSCrosses($1, $3);
      if($1->scratch == MS_TRUE) msFreeShape($1);
      if($3->scratch == MS_TRUE) msFreeShape($3);
      if(rval == -1) {
        yyerror(p, "Crosses operator failed.");
        return(-1);
      } else
        $$ = rval;
    }
  | shape_exp WITHIN shape_exp {
      int rval;
      rval = msGEOSWithin($1, $3);
      if($1->scratch == MS_TRUE) msFreeShape($1);
      if($3->scratch == MS_TRUE) msFreeShape($3);
      if(rval == -1) {
        yyerror(p, "Within operator failed.");
        return(-1);
      } else
        $$ = rval;
    }
  | shape_exp CONTAINS shape_exp {
      int rval;
      rval = msGEOSContains($1, $3);
      if($1->scratch == MS_TRUE) msFreeShape($1);
      if($3->scratch == MS_TRUE) msFreeShape($3);
      if(rval == -1) {
        yyerror(p, "Contains operator failed.");
        return(-1);
      } else
        $$ = rval;
    }
  | shape_exp DWITHIN shape_exp {
      double d;
      d = msGEOSDistance($1, $3);
      if($1->scratch == MS_TRUE) msFreeShape($1);
      if($3->scratch == MS_TRUE) msFreeShape($3);
      if(d == 0.0) 
        $$ = MS_TRUE;
      else
        $$ = MS_FALSE;
    }
  | shape_exp BEYOND shape_exp {
      double d;
      d = msGEOSDistance($1, $3);
      if($1->scratch == MS_TRUE) msFreeShape($1);
      if($3->scratch == MS_TRUE) msFreeShape($3);
      if(d > 0.0) 
        $$ = MS_TRUE;
      else
        $$ = MS_FALSE;
    }
;

math_exp: NUMBER
  | '(' math_exp ')' { $$ = $2; }
  | math_exp '+' math_exp { $$ = $1 + $3; }
  | math_exp '-' math_exp { $$ = $1 - $3; }
  | math_exp '*' math_exp { $$ = $1 * $3; }
  | math_exp '%' math_exp { $$ = (int)$1 % (int)$3; }
  | math_exp '/' math_exp {
      if($3 == 0.0) {
        yyerror(p, "Division by zero.");
        return(-1);
      } else
        $$ = $1 / $3; 
    }
  | '-' math_exp %prec NEG { $$ = $2; }
  | math_exp '^' math_exp { $$ = pow($1, $3); }
  | LENGTH '(' string_exp ')' { $$ = strlen($3); }
  | AREA '(' shape_exp ')' {
      if($3->type != MS_SHAPE_POLYGON) {
        yyerror(p, "Area can only be computed for polygon shapes.");
        return(-1);
      }
      $$ = msGetPolygonArea($3);
      if($3->scratch == MS_TRUE) msFreeShape($3);
    }
  | ROUND '(' math_exp ',' math_exp ')' { $$ = (MS_NINT($3/$5))*$5; }
;

shape_exp: SHAPE 
  | '(' shape_exp ')' { $$ = $2; }
  | YYBUFFER '(' shape_exp ',' math_exp ')' {
      shapeObj *s;
      s = msGEOSBuffer($3, $5);
      if(!s) {
        yyerror(p, "Executing buffer failed.");
        return(-1);
      }
      s->scratch = MS_TRUE;
      $$ = s;
    }
  | DIFFERENCE '(' shape_exp ',' shape_exp ')' {
      shapeObj *s;
      s = msGEOSDifference($3, $5);
      if(!s) {
        yyerror(p, "Executing difference failed.");
        return(-1);
      }
      s->scratch = MS_TRUE;
      $$ = s;
    }
;

string_exp: STRING
  | '(' string_exp ')' { $$ = $2; }
  | string_exp '+' string_exp { 
      $$ = (char *)malloc(strlen($1) + strlen($3) + 1);
      sprintf($$, "%s%s", $1, $3); free($1); free($3); 
    }
  | TOSTRING '(' math_exp ',' string_exp ')' {
      $$ = (char *) malloc(strlen($5) + 64); /* Plenty big? Should use snprintf below... */
      sprintf($$, $5, $3);
    }
  | COMMIFY '(' string_exp ')' {  
      $3 = msCommifyString($3); 
      $$ = $3; 
    }
;

time_exp: TIME
  | '(' time_exp ')' { $$ = $2; }
;

%%

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
    token = STRING;
    (*lvalp).strval = strdup(p->expr->curtoken->tokenval.strval);    
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

  case MS_TOKEN_COMPARISON_INTERSECTS: token = INTERSECTS; break;
  case MS_TOKEN_COMPARISON_DISJOINT: token = DISJOINT; break;
  case MS_TOKEN_COMPARISON_TOUCHES: token = TOUCHES; break;
  case MS_TOKEN_COMPARISON_OVERLAPS: token = OVERLAPS; break; 
  case MS_TOKEN_COMPARISON_CROSSES: token = CROSSES; break;
  case MS_TOKEN_COMPARISON_WITHIN: token = WITHIN; break; 
  case MS_TOKEN_COMPARISON_CONTAINS: token = CONTAINS; break;
  case MS_TOKEN_COMPARISON_BEYOND: token = BEYOND; break;
  case MS_TOKEN_COMPARISON_DWITHIN: token = DWITHIN; break;

  case MS_TOKEN_LOGICAL_AND: token = AND; break;
  case MS_TOKEN_LOGICAL_OR: token = OR; break;
  case MS_TOKEN_LOGICAL_NOT: token = NOT; break;

  case MS_TOKEN_BINDING_DOUBLE:
  case MS_TOKEN_BINDING_INTEGER:
    token = NUMBER;
    (*lvalp).dblval = atof(p->shape->values[p->expr->curtoken->tokenval.bindval.index]);
    break;
  case MS_TOKEN_BINDING_STRING:
    token = STRING;
    (*lvalp).strval = strdup(p->shape->values[p->expr->curtoken->tokenval.bindval.index]);
    break;
  case MS_TOKEN_BINDING_SHAPE:
    token = SHAPE;
    // fprintf(stderr, "token value = %s\n", msShapeToWKT(p->shape));
    (*lvalp).shpval = p->shape;
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
  case MS_TOKEN_FUNCTION_ROUND: token = ROUND; break;

  case MS_TOKEN_FUNCTION_BUFFER: token = YYBUFFER; break;
  case MS_TOKEN_FUNCTION_DIFFERENCE: token = DIFFERENCE; break;

  default:
    break;
  }

  p->expr->curtoken = p->expr->curtoken->next; /* re-position */ 
  return(token);
}

int yyerror(parseObj *p, const char *s) {
  msSetError(MS_PARSEERR, s, "yyparse()");
  return(0);
}
