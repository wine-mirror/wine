%{

/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002 Mike McCormack for CodeWeavers
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
#include "query.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#define YYLEX_PARAM info
#define YYPARSE_PARAM info

extern int yyerror(const char *str);

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_yyinput
{
    MSIDATABASE *db;
    LPCWSTR command;
    DWORD n, len;
    MSIVIEW **view;  /* view structure for the resulting query */
} yyinput;

struct string_list
{
    LPWSTR string;
    struct string_list *next;
};

static LPWSTR yygetstring( yyinput *info );
static INT yygetint( yyinput *sql );
static int yylex( void *yylval, yyinput *info);

static MSIVIEW *do_one_select( MSIDATABASE *db, MSIVIEW *in, 
                        struct string_list *columns );
static MSIVIEW *do_order_by( MSIDATABASE *db, MSIVIEW *in, 
                               struct string_list *columns );

static struct expr * EXPR_complex( struct expr *l, UINT op, struct expr *r );
static struct expr * EXPR_column( LPWSTR column );
static struct expr * EXPR_ival( INT ival );
static struct expr * EXPR_sval( LPWSTR string );

%}

%pure-parser

%union
{
    LPWSTR string;
    struct string_list *column_list;
    MSIVIEW *table;
    struct expr *expr;
}

%token TK_ABORT TK_AFTER TK_AGG_FUNCTION TK_ALL TK_AND TK_AS TK_ASC
%token TK_BEFORE TK_BEGIN TK_BETWEEN TK_BITAND TK_BITNOT TK_BITOR TK_BY
%token TK_CASCADE TK_CASE TK_CHECK TK_CLUSTER TK_COLLATE TK_COLUMN TK_COMMA
%token TK_COMMENT TK_COMMIT TK_CONCAT TK_CONFLICT 
%token TK_CONSTRAINT TK_COPY TK_CREATE
%token TK_DEFAULT TK_DEFERRABLE TK_DEFERRED TK_DELETE TK_DELIMITERS TK_DESC
%token TK_DISTINCT TK_DOT TK_DROP TK_EACH
%token TK_ELSE TK_END TK_END_OF_FILE TK_EQ TK_EXCEPT TK_EXPLAIN
%token TK_FAIL TK_FLOAT TK_FOR TK_FOREIGN TK_FROM TK_FUNCTION
%token TK_GE TK_GLOB TK_GROUP TK_GT
%token TK_HAVING
%token TK_IGNORE TK_ILLEGAL TK_IMMEDIATE TK_IN TK_INDEX TK_INITIALLY
%token <string> TK_ID 
%token TK_INSERT TK_INSTEAD TK_INTEGER TK_INTERSECT TK_INTO TK_IS TK_ISNULL
%token TK_JOIN TK_JOIN_KW
%token TK_KEY
%token TK_LE TK_LIKE TK_LIMIT TK_LP TK_LSHIFT TK_LT
%token TK_MATCH TK_MINUS
%token TK_NE TK_NOT TK_NOTNULL TK_NULL
%token TK_OF TK_OFFSET TK_ON TK_OR TK_ORACLE_OUTER_JOIN TK_ORDER
%token TK_PLUS TK_PRAGMA TK_PRIMARY
%token TK_RAISE TK_REFERENCES TK_REM TK_REPLACE TK_RESTRICT TK_ROLLBACK
%token TK_ROW TK_RP TK_RSHIFT
%token TK_SELECT TK_SEMI TK_SET TK_SLASH TK_SPACE TK_STAR TK_STATEMENT 
%token <string> TK_STRING
%token TK_TABLE TK_TEMP TK_THEN TK_TRANSACTION TK_TRIGGER
%token TK_UMINUS TK_UNCLOSED_STRING TK_UNION TK_UNIQUE
%token TK_UPDATE TK_UPLUS TK_USING
%token TK_VACUUM TK_VALUES TK_VIEW
%token TK_WHEN TK_WHERE

/*
 * These are extra tokens used by the lexer but never seen by the
 * parser.  We put them in a rule so that the parser generator will
 * add them to the parse.h output file.
 *
 */
%nonassoc END_OF_FILE ILLEGAL SPACE UNCLOSED_STRING COMMENT FUNCTION
          COLUMN AGG_FUNCTION.

%type <query> oneselect
%type <string> column table string_or_id
%type <column_list> selcollist
%type <table> from unorderedsel
%type <expr> expr val column_val

%%

oneselect:
    unorderedsel TK_ORDER TK_BY selcollist
        {
            yyinput* sql = (yyinput*) info;

            if( !$1 )
                YYABORT;
            if( $4 )
                *sql->view = do_order_by( sql->db, $1, $4 );
            else
                *sql->view = $1;
        }
  | unorderedsel
        {
            yyinput* sql = (yyinput*) info;

            *sql->view = $1;
        }
    ;

unorderedsel:
    TK_SELECT selcollist from 
        {
            yyinput* sql = (yyinput*) info;
            if( !$3 )
                YYABORT;
            if( $2 )
                $$ = do_one_select( sql->db, $3, $2 );
            else
                $$ = $3;
        }
  | TK_SELECT TK_DISTINCT selcollist from 
        {
            yyinput* sql = (yyinput*) info;
            MSIVIEW *view = $4;

            if( !view )
                YYABORT;
            if( $3 )
                view = do_one_select( sql->db, view, $3 );
            DISTINCT_CreateView( sql->db, & $$, view );
        }
    ;

selcollist:
    column 
        { 
            struct string_list *list;

            list = HeapAlloc( GetProcessHeap(), 0, sizeof *list );
            if( !list )
                YYABORT;
            list->string = $1;
            list->next = NULL;

            $$ = list;
            TRACE("Collist %s\n",debugstr_w($$->string));
        }
  | column TK_COMMA selcollist
        { 
            struct string_list *list;

            list = HeapAlloc( GetProcessHeap(), 0, sizeof *list );
            if( !list )
                YYABORT;
            list->string = $1;
            list->next = $3;

            $$ = list;
            TRACE("From table: %s\n",debugstr_w($$->string));
        }
  | TK_STAR
        {
            $$ = NULL;
        }
    ;

from:
    TK_FROM table
        { 
            yyinput* sql = (yyinput*) info;

            $$ = NULL;
            TRACE("From table: %s\n",debugstr_w($2));
            TABLE_CreateView( sql->db, $2, & $$ );
        }
  | TK_FROM table TK_WHERE expr
        { 
            yyinput* sql = (yyinput*) info;
            MSIVIEW *view = NULL;
            UINT r;

            $$ = NULL;
            TRACE("From table: %s\n",debugstr_w($2));
            r = TABLE_CreateView( sql->db, $2, &view );
            if( r != ERROR_SUCCESS )
                YYABORT;
            r = WHERE_CreateView( sql->db, &view, view );
            if( r != ERROR_SUCCESS )
                YYABORT;
            r = WHERE_AddCondition( view, $4 );
            if( r != ERROR_SUCCESS )
                YYABORT;
            $$ = view;
        }
    ;

expr:
    TK_LP expr TK_RP
        {
            $$ = $2;
        }
  | column_val TK_EQ column_val
        {
            $$ = EXPR_complex( $1, OP_EQ, $3 );
        }
  | expr TK_AND expr
        {
            $$ = EXPR_complex( $1, OP_AND, $3 );
        }
  | expr TK_OR expr
        {
            $$ = EXPR_complex( $1, OP_OR, $3 );
        }
  | column_val TK_EQ val
        {
            $$ = EXPR_complex( $1, OP_EQ, $3 );
        }
  | column_val TK_GT val
        {
            $$ = EXPR_complex( $1, OP_GT, $3 );
        }
  | column_val TK_LT val
        {
            $$ = EXPR_complex( $1, OP_LT, $3 );
        }
  | column_val TK_LE val
        {
            $$ = EXPR_complex( $1, OP_LE, $3 );
        }
  | column_val TK_GE val
        {
            $$ = EXPR_complex( $1, OP_GE, $3 );
        }
  | column_val TK_NE val
        {
            $$ = EXPR_complex( $1, OP_NE, $3 );
        }
  | column_val TK_IS TK_NULL
        {
            $$ = EXPR_complex( $1, OP_ISNULL, NULL );
        }
  | column_val TK_IS TK_NOT TK_NULL
        {
            $$ = EXPR_complex( $1, OP_NOTNULL, NULL );
        }
    ;

val:
    column_val
        {
            $$ = $1;
        }
  | TK_INTEGER
        {
            yyinput* sql = (yyinput*) info;
            $$ = EXPR_ival( yygetint(sql) );
        }
  | TK_STRING
        {
            $$ = EXPR_sval( $1 );
        }
    ;

column_val:
    column 
        {
            $$ = EXPR_column( $1 );
        }
    ;

column:
    table TK_DOT string_or_id
        {
            $$ = $3;  /* FIXME */
        }
  | string_or_id
        {
            $$ = $1;
        }
    ;

table:
    string_or_id
        {
            $$ = $1;
        }
    ;

string_or_id:
    TK_ID
        {
            yyinput* sql = (yyinput*) info;
            $$ = yygetstring(sql);
        }
  | TK_STRING
        {
            yyinput* sql = (yyinput*) info;
            $$ = yygetstring(sql);
        }
    ;

%%

int yylex( void *yylval, yyinput *sql)
{
    int token;

    do
    {
        sql->n += sql->len;
        if( ! sql->command[sql->n] )
            return 0;  /* end of input */

        TRACE("string : %s\n", debugstr_w(&sql->command[sql->n]));
        sql->len = sqliteGetToken( &sql->command[sql->n], &token );
        if( sql->len==0 )
            break;
    }
    while( token == TK_SPACE );

    TRACE("token : %d (%s)\n", token, debugstr_wn(&sql->command[sql->n], sql->len));
    
    return token;
}

LPWSTR yygetstring( yyinput *sql )
{
    LPCWSTR p = &sql->command[sql->n];
    LPWSTR str;
    UINT len = sql->len;

    /* if there's quotes, remove them */
    if( (p[0]=='`') && (p[len-1]=='`') )
    {
        p++;
        len -= 2;
    }
    str = HeapAlloc( GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
    if(!str )
        return str;
    memcpy(str, p, len*sizeof(WCHAR) );
    str[len]=0;

    return str;
}

INT yygetint( yyinput *sql )
{
    LPCWSTR p = &sql->command[sql->n];

    return atoiW( p );
}

int yyerror(const char *str)
{
    return 0;
}

static MSIVIEW *do_one_select( MSIDATABASE *db, MSIVIEW *in, 
                               struct string_list *columns )
{
    MSIVIEW *view = NULL;

    SELECT_CreateView( db, &view, in );
    if( view )
    {
        struct string_list *x = columns;

        while( x )
        {
            struct string_list *t = x->next;
            SELECT_AddColumn( view, x->string );
            HeapFree( GetProcessHeap(), 0, x->string );
            HeapFree( GetProcessHeap(), 0, x );
            x = t;
        }
    }
    else
        ERR("Error creating select query\n");
    return view;
}

static MSIVIEW *do_order_by( MSIDATABASE *db, MSIVIEW *in, 
                               struct string_list *columns )
{
    MSIVIEW *view = NULL;

    ORDER_CreateView( db, &view, in );
    if( view )
    {
        struct string_list *x = columns;

        while( x )
        {
            struct string_list *t = x->next;
            ORDER_AddColumn( view, x->string );
            HeapFree( GetProcessHeap(), 0, x->string );
            HeapFree( GetProcessHeap(), 0, x );
            x = t;
        }
    }
    else
        ERR("Error creating select query\n");
    return view;
}

static struct expr * EXPR_complex( struct expr *l, UINT op, struct expr *r )
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_COMPLEX;
        e->u.expr.left = l;
        e->u.expr.op = op;
        e->u.expr.right = r;
    }
    return e;
}

static struct expr * EXPR_column( LPWSTR column )
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_COLUMN;
        e->u.column = column;
    }
    return e;
}

static struct expr * EXPR_ival( INT ival )
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_IVAL;
        e->u.ival = ival;
    }
    return e;
}

static struct expr * EXPR_sval( LPWSTR string )
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_SVAL;
        e->u.sval = string;
    }
    return e;
}

UINT MSI_ParseSQL( MSIDATABASE *db, LPCWSTR command, MSIVIEW **phview )
{
    yyinput sql;
    int r;

    *phview = NULL;

    sql.db = db;
    sql.command = command;
    sql.n = 0;
    sql.len = 0;
    sql.view = phview;

    r = yyparse(&sql);

    TRACE("Parse returned %d\n", r);
    if( r )
    {
        if( *sql.view )
            (*sql.view)->ops->delete( *sql.view );
        return ERROR_BAD_QUERY_SYNTAX;
    }

    return ERROR_SUCCESS;
}
