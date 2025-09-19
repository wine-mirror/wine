%{

/* Advanced Query Syntax parser
 *
 * Copyright 2025 Vibhav Pant
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>

#include <wine/debug.h>

#include "aqs.h"

WINE_DEFAULT_DEBUG_CHANNEL( aqs );

#define YYFPRINTF(file, ...) TRACE(__VA_ARGS__)

static const PROPVARIANT propval_empty = { VT_EMPTY };

static int aqs_error( struct aqs_parser *parser, const char *str )
{
    if (TRACE_ON( aqs )) ERR( "%s\n", str );
    return 0;
}
%}

%lex-param { struct aqs_parser *ctx }
%parse-param { struct aqs_parser *ctx }
%define parse.error verbose
%define api.prefix {aqs_}
%define api.pure

%union
{
    struct string str;
    PROPVARIANT propval;
    struct aqs_expr *expr;
}

%token TK_LEFTPAREN TK_RIGHTPAREN TK_NULL
%token TK_INTEGER TK_WHITESPACE TK_ILLEGAL TK_MINUS
%token TK_TRUE TK_FALSE
%token <str> TK_STRING TK_ID

%type <propval> id string number boolean null
%type <propval> propval
%type <expr> expr query

%left TK_AND TK_OR TK_NOT TK_COLON TK_EQUAL TK_NOTEQUAL TK_LT TK_LTE TK_GT TK_GTE TK_TILDE TK_EXCLAM TK_DOLLAR

%destructor { PropVariantClear( &$$ ); } propval
%destructor { free_aqs_expr( $$ ); } expr

%debug

%printer { TRACE( "%s", debugstr_wn( $$.data, $$.len ) ); } TK_STRING TK_ID
%printer { TRACE( "%s", debugstr_propvar( &$$ ) ); } id string
%printer { TRACE( "%s", debugstr_propvar( &$$ ) ); } number
%printer { TRACE( "%s", debugstr_propvar( &$$ ) ); } propval
%printer { TRACE( "%s", debugstr_expr( $$ ) ); } expr

%%

query: expr { ctx->expr = $1; }
        ;
expr:
    TK_LEFTPAREN expr TK_RIGHTPAREN
        {
            $$ = $2;
#if YYBISON >= 30704
            (void)yysymbol_name; /* avoid unused function warning */
#endif
            (void)yynerrs; /* avoid unused variable warning */
        }
    | TK_NOT expr expr
        {
            struct aqs_expr *expr;

            if (FAILED(get_boolean_not_expr( ctx, $2, &expr )))
                YYABORT;
            if (FAILED(join_expr( ctx, expr, $3, &$$ )))
                YYABORT;
        }
    | expr expr TK_OR expr
        {
            struct aqs_expr *expr;

            if (FAILED(join_expr( ctx, $1, $2, &expr )))
                YYABORT;
            if (FAILED(get_boolean_binary_expr( ctx, DEVPROP_OPERATOR_OR_OPEN, expr, $4, &$$ )))
                YYABORT;
        }
    | expr TK_OR expr expr
        {
            struct aqs_expr *expr;

            if (FAILED(get_boolean_binary_expr( ctx, DEVPROP_OPERATOR_OR_OPEN, $1, $3, &expr )))
                YYABORT;
            if (FAILED(join_expr( ctx, expr, $4, &$$ )))
                YYABORT;
        }
    | expr TK_AND expr
        {
            if (FAILED(get_boolean_binary_expr( ctx, DEVPROP_OPERATOR_AND_OPEN, $1, $3, &$$ )))
                YYABORT;
        }
    | expr TK_OR expr
        {
            if (FAILED(get_boolean_binary_expr( ctx, DEVPROP_OPERATOR_OR_OPEN, $1, $3, &$$ )))
                YYABORT;
        }
    | TK_NOT expr
        {
            if (FAILED(get_boolean_not_expr( ctx, $2, &$$ )))
                YYABORT;
        }
    | expr expr
        {
            if (FAILED(join_expr( ctx, $1, $2, &$$ )))
                YYABORT;
        }
    | id TK_COLON propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_EQUALS, &$1, &$3, &$$ )))
                YYABORT;
        }
    | id TK_COLON TK_EQUAL propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_EQUALS, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_COLON TK_NOT propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_NOT_EQUALS, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_COLON TK_NOTEQUAL propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_NOT_EQUALS, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_COLON TK_LT propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_LESS_THAN, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_COLON TK_LTE propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_LESS_THAN_EQUALS, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_COLON TK_GT propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_GREATER_THAN, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_COLON TK_GTE propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_GREATER_THAN_EQUALS, &$1, &$4, &$$ )))
                YYABORT;
        }
    /* String operators */
    | id TK_TILDE TK_LT propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_BEGINS_WITH_IGNORE_CASE, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_TILDE TK_GT propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_ENDS_WITH_IGNORE_CASE, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_TILDE TK_EQUAL propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_CONTAINS_IGNORE_CASE, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_TILDE TK_TILDE propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_CONTAINS_IGNORE_CASE, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_TILDE TK_EXCLAM propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_MODIFIER_NOT | DEVPROP_OPERATOR_CONTAINS_IGNORE_CASE, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_TILDE propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_CONTAINS, &$1, &$3, &$$ )))
                YYABORT;
        }
    | id TK_DOLLAR TK_EQUAL propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_EQUALS, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_DOLLAR TK_DOLLAR propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_EQUALS, &$1, &$4, &$$ )))
                YYABORT;
        }
    | id TK_DOLLAR TK_LT propval
        {
            if (FAILED(get_compare_expr( ctx, DEVPROP_OPERATOR_BEGINS_WITH_IGNORE_CASE, &$1, &$4, &$$ )))
                YYABORT;
        }
propval:
    id | string | number | boolean | null
        ;
id:
    TK_ID
        {
            if (FAILED(get_string( ctx, &$1, TRUE, &$$ )))
                YYABORT;
        }
        ;
string:
    TK_STRING
        {
            if (FAILED(get_string( ctx, &$1, FALSE, &$$ )))
                YYABORT;
        }
        ;
number:
    TK_INTEGER
        {
            if (FAILED(get_integer( ctx, &$$ )))
                YYABORT;
        }
        ;
boolean:
    TK_TRUE
        {
            get_boolean( ctx, TRUE, &$$ );
        }
        ;
    | TK_FALSE
        {
            get_boolean( ctx, FALSE, &$$ );
        }
        ;
null:
    TK_NULL
        {
            $$ = propval_empty;
        }
        ;
%%
