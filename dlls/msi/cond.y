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
UINT get_property(MSIHANDLE hPackage, const WCHAR* prop, WCHAR* value);

typedef INT (*comp_int)(INT a, INT b);
typedef INT (*comp_str)(LPWSTR a, LPWSTR b, BOOL caseless);
typedef INT (*comp_m1)(LPWSTR a,int b);
typedef INT (*comp_m2)(int a,LPWSTR b);

static INT comp_lt_i(INT a, INT b);
static INT comp_gt_i(INT a, INT b);
static INT comp_le_i(INT a, INT b);
static INT comp_ge_i(INT a, INT b);
static INT comp_eq_i(INT a, INT b);
static INT comp_ne_i(INT a, INT b);
static INT comp_bitand(INT a, INT b);
static INT comp_highcomp(INT a, INT b);
static INT comp_lowcomp(INT a, INT b);

static INT comp_eq_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_ne_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_lt_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_gt_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_le_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_ge_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_substring(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_start(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_end(LPWSTR a, LPWSTR b, BOOL casless);

static INT comp_eq_m1(LPWSTR a, INT b);
static INT comp_ne_m1(LPWSTR a, INT b);
static INT comp_lt_m1(LPWSTR a, INT b);
static INT comp_gt_m1(LPWSTR a, INT b);
static INT comp_le_m1(LPWSTR a, INT b);
static INT comp_ge_m1(LPWSTR a, INT b);

static INT comp_eq_m2(INT a, LPWSTR b);
static INT comp_ne_m2(INT a, LPWSTR b);
static INT comp_lt_m2(INT a, LPWSTR b);
static INT comp_gt_m2(INT a, LPWSTR b);
static INT comp_le_m2(INT a, LPWSTR b);
static INT comp_ge_m2(INT a, LPWSTR b);

%}

%pure-parser

%union
{
    LPWSTR    string;
    INT       value;
    comp_int  fn_comp_int;
    comp_str  fn_comp_str;
    comp_m1   fn_comp_m1;
    comp_m2   fn_comp_m2;
}

%token COND_SPACE COND_EOF COND_SPACE
%token COND_OR COND_AND COND_NOT
%token COND_LT COND_GT COND_EQ 
%token COND_LPAR COND_RPAR COND_DBLQ COND_TILDA
%token COND_PERCENT COND_DOLLARS COND_QUESTION COND_AMPER COND_EXCLAM
%token COND_IDENT COND_NUMBER

%nonassoc COND_EOF COND_ERROR

%type <value> expression boolean_term boolean_factor 
%type <value> term value_i symbol_i integer
%type <string> identifier value_s symbol_s literal
%type <fn_comp_int> comp_op_i
%type <fn_comp_str> comp_op_s 
%type <fn_comp_m1>  comp_op_m1 
%type <fn_comp_m2>  comp_op_m2

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
    | boolean_term COND_AND boolean_factor
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
    value_i
        {
            $$ = $1;
        }
  | value_s
        {
            $$ = atoiW($1);
        }
  | value_i comp_op_i value_i
        {
            $$ = $2( $1, $3 );
        }
  | value_s comp_op_s value_s
        {
            $$ = $2( $1, $3, FALSE );
        }
  | value_s COND_TILDA comp_op_s value_s
        {
            $$ = $3( $1, $4, TRUE );
        }
  | value_s comp_op_m1 value_i
        {
            $$ = $2( $1, $3 );
        }
  | value_i comp_op_m2 value_s
        {
            $$ = $2( $1, $3 );
        }
  | COND_LPAR expression COND_RPAR
        {
            $$ = $2;
        }
    ;

comp_op_i:
    /* common functions */
   COND_EQ
        {
            $$ = comp_eq_i;
        }
  | COND_LT COND_GT
        {
            $$ = comp_ne_i;
        }
  | COND_LT
        {
            $$ = comp_lt_i;
        }
  | COND_GT
        {
            $$ = comp_gt_i;
        }
  | COND_LT COND_EQ
        {
            $$ = comp_le_i;
        }
  | COND_GT COND_EQ
        {
            $$ = comp_ge_i;
        }
  /*Int only*/
  | COND_GT COND_LT
        {
            $$ = comp_bitand;
        }
  | COND_LT COND_LT
        {
            $$ = comp_highcomp;
        }
  | COND_GT COND_GT
        {
            $$ = comp_lowcomp;
        }
    ;

comp_op_s:
    /* common functions */
   COND_EQ
        {
            $$ = comp_eq_s;
        }
  | COND_LT COND_GT
        {
            $$ = comp_ne_s;
        }
  | COND_LT
        {
            $$ = comp_lt_s;
        }
  | COND_GT
        {
            $$ = comp_gt_s;
        }
  | COND_LT COND_EQ
        {
            $$ = comp_le_s;
        }
  | COND_GT COND_EQ
        {
            $$ = comp_ge_s;
        }
  /*string only*/
  | COND_GT COND_LT
        {
            $$ = comp_substring;
        }
  | COND_LT COND_LT
        {
            $$ = comp_start;
        }
  | COND_GT COND_GT
        {
            $$ = comp_end;
        }
    ;

comp_op_m1:
    /* common functions */
   COND_EQ
        {
            $$ = comp_eq_m1;
        }
  | COND_LT COND_GT
        {
            $$ = comp_ne_m1;
        }
  | COND_LT
        {
            $$ = comp_lt_m1;
        }
  | COND_GT
        {
            $$ = comp_gt_m1;
        }
  | COND_LT COND_EQ
        {
            $$ = comp_le_m1;
        }
  | COND_GT COND_EQ
        {
            $$ = comp_ge_m1;
        }
  /*Not valid for mixed compares*/
  | COND_GT COND_LT
        {
            $$ = 0;
        }
  | COND_LT COND_LT
        {
            $$ = 0;
        }
  | COND_GT COND_GT
        {
            $$ = 0;
        }
    ;

comp_op_m2:
    /* common functions */
   COND_EQ
        {
            $$ = comp_eq_m2;
        }
  | COND_LT COND_GT
        {
            $$ = comp_ne_m2;
        }
  | COND_LT
        {
            $$ = comp_lt_m2;
        }
  | COND_GT
        {
            $$ = comp_gt_m2;
        }
  | COND_LT COND_EQ
        {
            $$ = comp_le_m2;
        }
  | COND_GT COND_EQ
        {
            $$ = comp_ge_m2;
        }
  /*Not valid for mixed compares*/
  | COND_GT COND_LT
        {
            $$ = 0;
        }
  | COND_LT COND_LT
        {
            $$ = 0;
        }
  | COND_GT COND_GT
        {
            $$ = 0;
        }
    ;

value_i:
    symbol_i
        {
            $$ = $1;
        }
  | integer
        {
            $$ = $1;
        }
    ;

value_s:
  symbol_s
    {
        $$ = $1;
    } 
  | literal
    {
        $$ = $1;
    }
    ;

literal:
    COND_DBLQ identifier COND_DBLQ
    {
        $$ = $2;
    }
    ;

symbol_i:
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

symbol_s:
    identifier
        {
            COND_input* cond = (COND_input*) info;
            $$ = HeapAlloc( GetProcessHeap(), 0, 0x100*sizeof (WCHAR) );

            /* Lookup the identifier */
            /* This will not really work until we have write access to the table*/
            /* HACK ALERT HACK ALERT... */

            if (get_property(cond->hInstall,$1,$$) != ERROR_SUCCESS)
            {
                $$[0]=0;
            }
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

identifier:
    COND_IDENT
        {
            COND_input* cond = (COND_input*) info;
            $$ = COND_GetString(cond);
            if( !$$ )
                YYABORT;
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


/* the mess of comparison functions */

static INT comp_lt_i(INT a, INT b)
{ return (a < b); }
static INT comp_gt_i(INT a, INT b)
{ return (a > b); }
static INT comp_le_i(INT a, INT b)
{ return (a <= b); }
static INT comp_ge_i(INT a, INT b)
{ return (a >= b); }
static INT comp_eq_i(INT a, INT b)
{ return (a == b); }
static INT comp_ne_i(INT a, INT b)
{ return (a != b); }
static INT comp_bitand(INT a, INT b)
{ return a & b;}
static INT comp_highcomp(INT a, INT b)
{ return HIWORD(a)==b; }
static INT comp_lowcomp(INT a, INT b)
{ return LOWORD(a)==b; }

static INT comp_eq_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return !strcmpiW(a,b); else return !strcmpW(a,b);}
static INT comp_ne_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b); else  return strcmpW(a,b);}
static INT comp_lt_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)<0; else return strcmpW(a,b)<0;}
static INT comp_gt_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)>0; else return strcmpW(a,b)>0;}
static INT comp_le_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)<=0; else return strcmpW(a,b)<=0;}
static INT comp_ge_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)>=0; else return  strcmpW(a,b)>=0;}
static INT comp_substring(LPWSTR a, LPWSTR b, BOOL casless)
/* ERROR NOT WORKING REWRITE */
{ if (casless) return strstrW(a,b)!=NULL; else return strstrW(a,b)!=NULL;}
static INT comp_start(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strncmpiW(a,b,strlenW(b))==0; 
  else return strncmpW(a,b,strlenW(b))==0;}
static INT comp_end(LPWSTR a, LPWSTR b, BOOL casless)
{ 
    int i = strlenW(a); 
    int j = strlenW(b); 
    if (j>i)
        return 0;
    if (casless) return (!strcmpiW(&a[i-j-1],b));
    else  return (!strcmpW(&a[i-j-1],b));
}


static INT comp_eq_m1(LPWSTR a, INT b)
{ return atoiW(a)==b; }
static INT comp_ne_m1(LPWSTR a, INT b)
{ return atoiW(a)!=b; }
static INT comp_lt_m1(LPWSTR a, INT b)
{ return atoiW(a)<b; }
static INT comp_gt_m1(LPWSTR a, INT b)
{ return atoiW(a)>b; }
static INT comp_le_m1(LPWSTR a, INT b)
{ return atoiW(a)<=b; }
static INT comp_ge_m1(LPWSTR a, INT b)
{ return atoiW(a)>=b; }

static INT comp_eq_m2(INT a, LPWSTR b)
{ return a == atoiW(b); }
static INT comp_ne_m2(INT a, LPWSTR b)
{ return a != atoiW(b); }
static INT comp_lt_m2(INT a, LPWSTR b)
{ return a < atoiW(b); }
static INT comp_gt_m2(INT a, LPWSTR b)
{ return a > atoiW(b); }
static INT comp_le_m2(INT a, LPWSTR b)
{ return a <= atoiW(b); }
static INT comp_ge_m2(INT a, LPWSTR b)
{ return a >= atoiW(b); }


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
    return( COND_IsAlpha( x ) || COND_IsNumber( x ) || ( x == '_' ) 
            || ( x == '#' ) || (x == '.') );
}

static int COND_lex( void *COND_lval, COND_input *cond )
{
    WCHAR ch;
    /* grammer */
    static const WCHAR NOT[] = {'O','T',' ',0};
    static const WCHAR AND[] = {'N','D',' ',0};
    static const WCHAR OR[] = {'R',' ',0};

    int rc = COND_SPACE;

    while (rc == COND_SPACE)
    {

    cond->start = cond->n;
    ch = cond->str[cond->n];
    if( ch == 0 )
        return 0;
    cond->n++;

    switch( ch )
    {
    case '(': rc = COND_LPAR; break;
    case ')': rc = COND_RPAR; break;
    case '&': rc = COND_AMPER; break;
    case '!': rc = COND_EXCLAM; break;
    case '$': rc = COND_DOLLARS; break;
    case '?': rc = COND_QUESTION; break;
    case '%': rc = COND_PERCENT; break;
    case ' ': rc = COND_SPACE; break;
    case '=': rc = COND_EQ; break;
    case '"': rc = COND_DBLQ; break;
    case '~': rc = COND_TILDA; break;
    case '<': rc = COND_LT; break;
    case '>': rc = COND_GT; break;
    case 'N': 
    case 'n': 
        if (strncmpiW(&cond->str[cond->n],NOT,3)==0)
        {
            cond->n+=3;
            rc = COND_NOT;
        }
        break;
    case 'A': 
    case 'a': 
        if (strncmpiW(&cond->str[cond->n],AND,3)==0)
        {
            cond->n+=3;
            rc = COND_AND;
        }
        break;
    case 'O':
    case 'o':
        if (strncmpiW(&cond->str[cond->n],OR,2)==0)
        {
            cond->n+=2;
            rc = COND_OR;
        }
        break;
    default: 
        if( COND_IsAlpha( ch ) )
        {
            ch = cond->str[cond->n];
            while( COND_IsIdent( ch ) )
                ch = cond->str[cond->n++];
            cond->n--;
            rc = COND_IDENT;
            break;
        }

        if( COND_IsNumber( ch ) )
        {
            ch = cond->str[cond->n];
            while( COND_IsNumber( ch ) )
                ch = cond->str[cond->n++];
            rc = COND_NUMBER;
            break;
        }

        ERR("Got unknown character %c(%x)\n",ch,ch);
        rc = COND_ERROR;
        break;
    }
    }
    
    return rc;
}

static LPWSTR COND_GetString( COND_input *cond )
{
    int len;
    LPWSTR str;

    len = cond->n - cond->start;
    str = HeapAlloc( GetProcessHeap(), 0, (len+1) * sizeof (WCHAR) );
    if( str )
    {
        strncpyW( str, &cond->str[cond->start], len );
        str[len]=0;
    }
    TRACE("Got identifier %s\n",debugstr_w(str));
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
    cond.result = -1;
    
    TRACE("Evaluating %s\n",debugstr_w(szCondition));    

    if( !COND_parse( &cond ) )
        r = cond.result;
    else
        r = MSICONDITION_ERROR;

    TRACE("Evaluates to %i\n",r);
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
