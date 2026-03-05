%{
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
%}

%define api.pure
%define parse.error verbose
%require "3.0"

%parse-param {cql2text_parse_context *context}
%lex-param {cql2text_parse_context *context}

%token CQL2_TOK_VALUE_START

%token CQL2_TOK_INTEGER_NUMBER      "integer number"
%token CQL2_TOK_DOUBLE_NUMBER       "floating point number"
%token CQL2_TOK_STRING              "string"
%token CQL2_TOK_IDENTIFIER          "identifier"
%token CQL2_TOK_WKT                 "wkt geometry"

%token END 0                        "end of string"

%token CQL2_TOK_NOT                 "NOT"
%token CQL2_TOK_OR                  "OR"
%token CQL2_TOK_AND                 "AND"

%token CQL2_TOK_DIV                 "DIV"

%token CQL2_TOK_IS                  "IS"
%token CQL2_TOK_NULL                "NULL"

%token CQL2_TOK_BOOLEAN_LITERAL     "true/false"

%token CQL2_TOK_CASEI               "CASEI"

%token CQL2_TOK_S_INTERSECTS        "S_INTERSECTS"
%token CQL2_TOK_S_EQUALS            "S_EQUALS"
%token CQL2_TOK_S_DISJOINT          "S_DISJOINT"
%token CQL2_TOK_S_TOUCHES           "S_TOUCHES"
%token CQL2_TOK_S_WITHIN            "S_WITHIN"
%token CQL2_TOK_S_OVERLAPS          "S_OVERLAPS"
%token CQL2_TOK_S_CROSSES           "S_CROSSES"
%token CQL2_TOK_S_CONTAINS          "S_CONTAINS"

%left CQL2_TOK_OR
%left CQL2_TOK_AND
%left CQL2_TOK_NOT

%left '=' '<' '>' '!' CQL2_TOK_BETWEEN CQL2_TOK_IN CQL2_TOK_LIKE CQL2_TOK_IS

%left '+' '-'
%left '*' '/' CQL2_TOK_DIV '%'
%left CQL2_TOK_UMINUS

/* Any grammar rule that does $$ = must be listed afterwards */
/* as well as CQL2_TOK_INTEGER_NUMBER CQL2_TOK_DOUBLE_NUMBER CQL2_TOK_STRING CQL2_TOK_IDENTIFIER CQL2_TOK_BOOLEAN_LITERAL that are allocated by cql2textlex(), and identifier */
%destructor { delete $$; } CQL2_TOK_INTEGER_NUMBER CQL2_TOK_DOUBLE_NUMBER CQL2_TOK_STRING CQL2_TOK_IDENTIFIER CQL2_TOK_WKT CQL2_TOK_BOOLEAN_LITERAL
%destructor { delete $$; } boolean_expr value_expr_or_boolean_literal value_expr value_expr_list

%%

input:
    | CQL2_TOK_VALUE_START boolean_expr
        {
            context->poRoot = $2;
        }

boolean_expr:

    boolean_expr CQL2_TOK_AND boolean_expr
        {
            $$ = cql2_create_and_or_or( CQL2_AND, $1, $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | boolean_expr CQL2_TOK_OR boolean_expr
        {
            $$ = cql2_create_and_or_or( CQL2_OR, $1, $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_NOT boolean_expr
        {
            $$ = new cql2_expr_node( CQL2_NOT );
            $$->PushSubExpression( $2 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr_or_boolean_literal '=' value_expr_or_boolean_literal
        {
            $$ = new cql2_expr_node( CQL2_EQ );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_CASEI '(' value_expr ')' '=' CQL2_TOK_CASEI '(' CQL2_TOK_STRING ')'
        {
            $$ = new cql2_expr_node( CQL2_IEQ);
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $8 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr '<' '>' value_expr_or_boolean_literal
        {
            $$ = new cql2_expr_node( CQL2_NE );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $4 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_BOOLEAN_LITERAL '<' '>' value_expr_or_boolean_literal
        {
            $$ = new cql2_expr_node( CQL2_NE );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $4 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_CASEI '(' value_expr ')' '<' '>' CQL2_TOK_CASEI '(' CQL2_TOK_STRING ')'
        {
            $$ = new cql2_expr_node( CQL2_INE);
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $9 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }
    | value_expr '<' value_expr
        {
            $$ = new cql2_expr_node( CQL2_LT );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr '>' value_expr
        {
            $$ = new cql2_expr_node( CQL2_GT );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr '<' '=' value_expr
        {
            $$ = new cql2_expr_node( CQL2_LE );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $4 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }


    | value_expr '>' '=' value_expr
        {
            $$ = new cql2_expr_node( CQL2_GE );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $4 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr CQL2_TOK_LIKE CQL2_TOK_STRING
        {
            $$ = new cql2_expr_node( CQL2_LIKE);
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr CQL2_TOK_NOT CQL2_TOK_LIKE CQL2_TOK_STRING
        {
            cql2_expr_node *like = new cql2_expr_node( CQL2_LIKE );
            like->PushSubExpression( $1 );
            like->PushSubExpression( $4 );

            $$ = new cql2_expr_node( CQL2_NOT );
            $$->PushSubExpression( like );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_CASEI '(' value_expr ')' CQL2_TOK_LIKE CQL2_TOK_CASEI '(' CQL2_TOK_STRING ')'
        {
            $$ = new cql2_expr_node( CQL2_ILIKE);
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $8 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_CASEI '(' value_expr ')' CQL2_TOK_NOT CQL2_TOK_LIKE CQL2_TOK_CASEI '(' CQL2_TOK_STRING ')'
        {
            cql2_expr_node *like = new cql2_expr_node( CQL2_ILIKE );
            like->PushSubExpression( $3 );
            like->PushSubExpression( $9 );

            $$ = new cql2_expr_node( CQL2_NOT );
            $$->PushSubExpression( like );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr CQL2_TOK_IN '(' value_expr_list ')'
        {
            $$ = $4;
            $$->m_field_type = CQL2_BOOLEAN;
            $$->m_op = CQL2_IN;
            $$->PushSubExpression( $1 );
            $$->ReverseSubExpressions();
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr CQL2_TOK_NOT CQL2_TOK_IN '(' value_expr_list ')'
        {
            cql2_expr_node *in = $5;
            in->m_op = CQL2_IN;
            in->PushSubExpression( $1 );
            in->ReverseSubExpressions();

            $$ = new cql2_expr_node( CQL2_NOT );
            $$->PushSubExpression( in );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr CQL2_TOK_BETWEEN value_expr CQL2_TOK_AND value_expr
        {
            $$ = new cql2_expr_node( CQL2_BETWEEN );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $5 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr CQL2_TOK_NOT CQL2_TOK_BETWEEN value_expr CQL2_TOK_AND value_expr
        {
            cql2_expr_node *between = new cql2_expr_node( CQL2_BETWEEN );
            between->PushSubExpression( $1 );
            between->PushSubExpression( $4 );
            between->PushSubExpression( $6 );

            $$ = new cql2_expr_node( CQL2_NOT );
            $$->PushSubExpression( between );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr CQL2_TOK_IS CQL2_TOK_NULL
        {
            $$ = new cql2_expr_node( CQL2_IS_NULL );
            $$->PushSubExpression( $1 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr CQL2_TOK_IS CQL2_TOK_NOT CQL2_TOK_NULL
        {
            cql2_expr_node *is_null = new cql2_expr_node( CQL2_IS_NULL );
            is_null->PushSubExpression( $1 );

            $$ = new cql2_expr_node( CQL2_NOT );
            $$->PushSubExpression( is_null );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_S_INTERSECTS '(' value_expr ',' value_expr ')'
        {
            $$ = new cql2_expr_node( CQL2_S_INTERSECTS );
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $5 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_S_EQUALS '(' value_expr ',' value_expr ')'
        {
            $$ = new cql2_expr_node( CQL2_S_EQUALS );
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $5 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_S_DISJOINT '(' value_expr ',' value_expr ')'
        {
            $$ = new cql2_expr_node( CQL2_S_DISJOINT );
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $5 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_S_TOUCHES '(' value_expr ',' value_expr ')'
        {
            $$ = new cql2_expr_node( CQL2_S_TOUCHES );
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $5 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_S_WITHIN '(' value_expr ',' value_expr ')'
        {
            $$ = new cql2_expr_node( CQL2_S_WITHIN );
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $5 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_S_OVERLAPS '(' value_expr ',' value_expr ')'
        {
            $$ = new cql2_expr_node( CQL2_S_OVERLAPS );
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $5 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_S_CROSSES '(' value_expr ',' value_expr ')'
        {
            $$ = new cql2_expr_node( CQL2_S_CROSSES );
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $5 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | CQL2_TOK_S_CONTAINS '(' value_expr ',' value_expr ')'
        {
            $$ = new cql2_expr_node( CQL2_S_CONTAINS );
            $$->PushSubExpression( $3 );
            $$->PushSubExpression( $5 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | '(' boolean_expr ')'
        {
            $$ = $2;
        }

    | CQL2_TOK_BOOLEAN_LITERAL
        {
            $$ = $1;
        }

value_expr_list:
    value_expr ',' value_expr_list
        {
            $$ = $3;
            $$->PushSubExpression( $1 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr
        {
            $$ = new cql2_expr_node( CQL2_ARGUMENT_LIST ); /* temporary value */
            $$->PushSubExpression( $1 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

value_expr_or_boolean_literal:

    value_expr

    | CQL2_TOK_BOOLEAN_LITERAL
        {
            $$ = $1;
        }

value_expr:
    CQL2_TOK_INTEGER_NUMBER
        {
            $$ = $1;
        }

    | CQL2_TOK_DOUBLE_NUMBER
        {
            $$ = $1;
        }

    | CQL2_TOK_STRING
        {
            $$ = $1;
        }

    | CQL2_TOK_WKT
        {
            $$ = $1;
        }

    | CQL2_TOK_IDENTIFIER
        {
            $$ = $1;  // validation deferred.
            $$->m_node_type = CQL2_NT_COLUMN;
        }

    | CQL2_TOK_IDENTIFIER '(' value_expr_list ')'
        {
            $$ = $3;
            $$->m_node_type = CQL2_NT_OPERATION;
            $$->m_op = CQL2_CUSTOM_FUNC;
            $$->m_osVal = $1->m_osVal;
            $$->ReverseSubExpressions();
            delete $1;
        }

    | value_expr '+' value_expr
        {
            $$ = new cql2_expr_node( CQL2_ADD );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr '-' value_expr
        {
            $$ = new cql2_expr_node( CQL2_SUBTRACT );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr '*' value_expr
        {
            $$ = new cql2_expr_node( CQL2_MULTIPLY );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr '/' value_expr
        {
            $$ = new cql2_expr_node( CQL2_DIV );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr CQL2_TOK_DIV value_expr
        {
            $$ = new cql2_expr_node( CQL2_DIV );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | value_expr '%' value_expr
        {
            $$ = new cql2_expr_node( CQL2_MOD );
            $$->PushSubExpression( $1 );
            $$->PushSubExpression( $3 );
            if( $$->HasReachedMaxDepth() )
            {
                yyerror (context, "Maximum expression depth reached");
                delete $$;
                YYERROR;
            }
        }

    | '-' value_expr %prec CQL2_TOK_UMINUS
        {
            if ($2->m_node_type == CQL2_NT_CONSTANT )
            {
                if( $2->m_field_type == CQL2_INTEGER &&
                    $2->m_nVal == -2147483648 )
                {
                    $$ = $2;
                }
                else
                {
                    $$ = $2;
                    $$->m_nVal *= -1;
                    $$->m_dfVal *= -1;
                }
            }
            else
            {
                $$ = new cql2_expr_node( CQL2_MULTIPLY );
                $$->PushSubExpression( new cql2_expr_node(-1) );
                $$->PushSubExpression( $2 );
                if( $$->HasReachedMaxDepth() )
                {
                    yyerror (context, "Maximum expression depth reached");
                    delete $$;
                    YYERROR;
                }
            }
        }

    | '(' value_expr ')'
        {
            $$ = $2;
        }
