%{

/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2003 Mike McCormack for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "msi.h"
#include "msiquery.h"

#define YYLEX_PARAM info
#define YYPARSE_PARAM info

static int COND_error(char *str);

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_yyinput
{
    MSIHANDLE hInstall;
    LPCWSTR str;
    INT    n;
    INT    start;
    MSICONDITION result;
} COND_input;

static LPWSTR COND_GetString( COND_input *info );
static int COND_lex( void *COND_lval, COND_input *info);

typedef INT (*comp_int)(INT a, INT b);

static INT comp_lt(INT a, INT b);
static INT comp_gt(INT a, INT b);
static INT comp_le(INT a, INT b);
static INT comp_ge(INT a, INT b);
static INT comp_eq(INT a, INT b);
static INT comp_ne(INT a, INT b);

%}

%pure-parser

%union
{
    LPWSTR    string;
    INT       value;
    comp_int  fn_comp_int;
}

%token COND_SPACE COND_EOF COND_SPACE
%token COND_OR COND_AND COND_NOT
%token COND_LT COND_GT COND_LE COND_GE COND_EQ COND_NE
%token COND_LPAR COND_RPAR
%token COND_PERCENT COND_DOLLARS COND_QUESTION COND_AMPER COND_EXCLAM
%token COND_IDENT COND_NUMBER

%nonassoc COND_EOF COND_ERROR

%type <value> expression boolean_term boolean_factor term value symbol integer
%type <string> identifier
%type <fn_comp_int> comparison_op

%%

condition:
    expression
        {
            COND_input* cond = (COND_input*) info;
            cond->result = $1;
        }
    ;

expression:
    boolean_term 
        {
            $$ = $1;
        }
  | boolean_term COND_OR expression
        {
            $$ = $1 || $3;
        }
    ;

boolean_term:
    boolean_factor
        {
            $$ = $1;
        }
  | boolean_factor COND_AND term
        {
            $$ = $1 && $3;
        }
    ;

boolean_factor:
    term
        {
            $$ = $1;
        }
  | COND_NOT term
        {
            $$ = ! $2;
        }
    ;

term:
    value
        {
            $$ = $1;
        }
  | value comparison_op value
        {
            $$ = $2( $1, $3 );
        }
  | COND_LPAR expression COND_RPAR
        {
            $$ = $2;
        }
    ;

comparison_op:
    COND_LT
        {
            $$ = comp_lt;
        }
  | COND_GT
        {
            $$ = comp_gt;
        }
  | COND_LE
        {
            $$ = comp_le;
        }
  | COND_GE
        {
            $$ = comp_ge;
        }
  | COND_EQ
        {
            $$ = comp_eq;
        }
  | COND_NE
        {
            $$ = comp_ne;
        }
    ;

value:
    symbol
        {
            $$ = $1;
        }
  | integer
        {
            $$ = $1;
        }
    ;

symbol:
    COND_DOLLARS identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MsiGetComponentStateW(cond->hInstall, $2, &install, &action );
            $$ = action;
        }
  | COND_QUESTION identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MsiGetComponentStateW(cond->hInstall, $2, &install, &action );
            $$ = install;
        }
  | COND_AMPER identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MsiGetFeatureStateW(cond->hInstall, $2, &install, &action );
            $$ = action;
        }
  | COND_EXCLAM identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MsiGetFeatureStateW(cond->hInstall, $2, &install, &action );
            $$ = install;
        }
    ;

identifier:
    COND_IDENT
        {
            COND_input* cond = (COND_input*) info;
            $$ = COND_GetString(cond);
            if( !$$ )
                YYABORT;
        }
  | COND_PERCENT identifier
        {
            UINT len = GetEnvironmentVariableW( $2, NULL, 0 );
            if( len++ )
            {
                $$ = HeapAlloc( GetProcessHeap(), 0, len*sizeof (WCHAR) );
                if( $$ )
                    GetEnvironmentVariableW( $2, $$, len );
            }
            HeapFree( GetProcessHeap(), 0, $2 );
        }
    ;

integer:
    COND_NUMBER
        {
            COND_input* cond = (COND_input*) info;
            LPWSTR szNum = COND_GetString(cond);
            if( !szNum )
                YYABORT;
            $$ = atoiW( szNum );
            HeapFree( GetProcessHeap(), 0, szNum );
        }
    ;

%%

static INT comp_lt(INT a, INT b)
{
    return (a < b);
}

static INT comp_gt(INT a, INT b)
{
    return (a > b);
}

static INT comp_le(INT a, INT b)
{
    return (a <= b);
}

static INT comp_ge(INT a, INT b)
{
    return (a >= b);
}

static INT comp_eq(INT a, INT b)
{
    return (a == b);
}

static INT comp_ne(INT a, INT b)
{
    return (a != b);
}


static int COND_IsAlpha( WCHAR x )
{
    return( ( ( x >= 'A' ) && ( x <= 'Z' ) ) ||
            ( ( x >= 'a' ) && ( x <= 'z' ) ) );
}

static int COND_IsNumber( WCHAR x )
{
    return( ( x >= '0' ) && ( x <= '9' ) );
}

static int COND_IsIdent( WCHAR x )
{
    return( COND_IsAlpha( x ) || COND_IsNumber( x ) || ( x == '_' ) );
}

static int COND_lex( void *COND_lval, COND_input *cond )
{
    WCHAR ch;

    cond->start = cond->n;
    ch = cond->str[cond->n];
    if( !ch )
        return COND_EOF;
    cond->n++;

    switch( ch )
    {
    case '(': return COND_LPAR;
    case ')': return COND_RPAR;
    case '&': return COND_AMPER;
    case '!': return COND_EXCLAM;
    case '$': return COND_DOLLARS;
    case '?': return COND_QUESTION;
    case '%': return COND_PERCENT;
    case ' ': return COND_SPACE;
    }

    if( COND_IsAlpha( ch ) )
    {
        ch = cond->str[cond->n];
        while( COND_IsIdent( ch ) )
            ch = cond->str[cond->n++];
        return COND_IDENT;
    }

    if( COND_IsNumber( ch ) )
    {
        ch = cond->str[cond->n];
        while( COND_IsNumber( ch ) )
            ch = cond->str[cond->n++];
        return COND_NUMBER;
    }

    return COND_ERROR;
}

static LPWSTR COND_GetString( COND_input *cond )
{
    int len;
    LPWSTR str;

    len = cond->n - cond->start;
    str = HeapAlloc( GetProcessHeap(), 0, (len+1) * sizeof (WCHAR) );
    if( str )
        strncpyW( str, &cond->str[cond->start], len );
    return str;
}

static int COND_error(char *str)
{
    return 0;
}

MSICONDITION WINAPI MsiEvaluateConditionW( MSIHANDLE hInstall, LPCWSTR szCondition )
{
    COND_input cond;
    MSICONDITION r;

    cond.hInstall = hInstall;
    cond.str   = szCondition;
    cond.n     = 0;
    cond.start = 0;
    cond.result = MSICONDITION_ERROR;
    
    if( !COND_parse( &cond ) )
        r = cond.result;
    else
        r = MSICONDITION_ERROR;

    return r;
}

MSICONDITION WINAPI MsiEvaluateConditionA( MSIHANDLE hInstall, LPCSTR szCondition )
{
    LPWSTR szwCond = NULL;
    MSICONDITION r;

    if( szCondition )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szCondition, -1, NULL, 0 );
        szwCond = HeapAlloc( GetProcessHeap(), 0, len * sizeof (WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, szCondition, -1, szwCond, len );
    }

    r = MsiEvaluateConditionW( hInstall, szwCond );

    if( szwCond )
        HeapFree( GetProcessHeap(), 0, szwCond );

    return r;
}
