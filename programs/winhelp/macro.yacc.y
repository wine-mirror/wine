%{
/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 */

#include <stdlib.h>
#include "macro.h"

static int   skip = 0;
static LPSTR filename;
static LPSTR windowname;

%}
%union
  {
    BOOL    bool;
    LONG    integer;
    LPSTR   string;
    BOOL    (*bool_function_void)(VOID);
    BOOL    (*bool_function_string)(LPCSTR);
    VOID    (*void_function_void)(VOID);
    VOID    (*void_function_uint)(LONG);
    VOID    (*void_function_string)(LPCSTR);
    VOID    (*void_function_2int_3uint_string)(LONG,LONG,LONG,LONG,LONG,LPCSTR);
    VOID    (*void_function_2string)(LPCSTR,LPCSTR);
    VOID    (*void_function_2string_2uint_2string)(LPCSTR,LPCSTR,LONG,LONG,LPCSTR,LPCSTR);
    VOID    (*void_function_2string_uint)(LPCSTR,LPCSTR,LONG);
    VOID    (*void_function_2string_uint_string)(LPCSTR,LPCSTR,LONG,LPCSTR);
    VOID    (*void_function_2string_wparam_lparam_string)(LPCSTR,LPCSTR,WPARAM,LPARAM,LPCSTR);
    VOID    (*void_function_2uint)(LONG,LONG);
    VOID    (*void_function_2uint_string)(LONG,LONG,LPCSTR);
    VOID    (*void_function_3string)(LPCSTR,LPCSTR,LPCSTR);
    VOID    (*void_function_3string_2uint)(LPCSTR,LPCSTR,LPCSTR,LONG,LONG);
    VOID    (*void_function_3uint)(LONG,LONG,LONG);
    VOID    (*void_function_4string)(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
    VOID    (*void_function_4string_2uint)(LPCSTR,LPCSTR,LPCSTR,LPCSTR,LONG,LONG);
    VOID    (*void_function_4string_uint)(LPCSTR,LPCSTR,LPCSTR,LPCSTR,LONG);
    VOID    (*void_function_string_uint)(LPCSTR,LONG);
    VOID    (*void_function_string_uint_2string)(LPCSTR,LONG,LPCSTR,LPCSTR);
    VOID    (*void_function_string_uint_string)(LPCSTR,LONG,LPCSTR);
    VOID    (*void_function_string_wparam_lparam)(LPCSTR,WPARAM,LPARAM);
  }
%token							NOT
%token 							IF_THEN
%token 							IF_THEN_ELSE
%token <string>	 					tSTRING
%token <integer> 					INTEGER
%token <bool_function_string>				BOOL_FUNCTION_STRING
%token <bool_function_void>				BOOL_FUNCTION_VOID
%token <void_function_2int_3uint_string>		VOID_FUNCTION_2INT_3UINT_STRING
%token <void_function_2string>				VOID_FUNCTION_2STRING
%token <void_function_2string_2uint_2string>		VOID_FUNCTION_2STRING_2UINT_2STRING
%token <void_function_2string_uint>			VOID_FUNCTION_2STRING_UINT
%token <void_function_2string_uint_string>		VOID_FUNCTION_2STRING_UINT_STRING
%token <void_function_2string_wparam_lparam_string>	VOID_FUNCTION_2STRING_WPARAM_LPARAM_STRING
%token <void_function_2uint>				VOID_FUNCTION_2UINT
%token <void_function_2uint_string>			VOID_FUNCTION_2UINT_STRING
%token <void_function_3string>				VOID_FUNCTION_3STRING
%token <void_function_3string_2uint>			VOID_FUNCTION_3STRING_2UINT
%token <void_function_3uint>				VOID_FUNCTION_3UINT
%token <void_function_4string>				VOID_FUNCTION_4STRING
%token <void_function_4string_2uint>			VOID_FUNCTION_4STRING_2UINT
%token <void_function_4string_uint>			VOID_FUNCTION_4STRING_UINT
%token <void_function_string>				VOID_FUNCTION_STRING
%token <void_function_string_uint>			VOID_FUNCTION_STRING_UINT
%token <void_function_string_uint_2string>		VOID_FUNCTION_STRING_UINT_2STRING
%token <void_function_string_uint_string>		VOID_FUNCTION_STRING_UINT_STRING
%token <void_function_string_wparam_lparam>		VOID_FUNCTION_STRING_WPARAM_LPARAM
%token <void_function_uint>				VOID_FUNCTION_UINT
%token <void_function_void>				VOID_FUNCTION_VOID
%token <void_function_2string>				VOID_FUNCTION_FILE_WIN
%token <void_function_3string>				VOID_FUNCTION_FILE_WIN_STRING
%token <void_function_2string_uint>			VOID_FUNCTION_FILE_WIN_UINT
%type  <bool> bool_macro
%%

macrostring:	macro |
		macro macrosep macrostring ;

macrosep:	';' |
		':' ;

macro:		/* Empty */ |
		IF_THEN      '(' bool_macro ','  {if (! $3) skip++;}
                                 macrostring ')' {if (! $3) skip--;} |
		IF_THEN_ELSE '(' bool_macro ','  {if (! $3) skip++;}
                                 macrostring ',' {if (! $3) skip--; else skip++;}
                                 macrostring ')' {if (  $3) skip--;} |
		VOID_FUNCTION_VOID
			'(' ')'
			{if (! skip) (*$1)();} |
		VOID_FUNCTION_STRING
			'(' tSTRING ')'
			{if (! skip) (*$1)($3);} |
		VOID_FUNCTION_2STRING
			'(' tSTRING ',' tSTRING ')'
			{if (! skip) (*$1)($3, $5);} |
		VOID_FUNCTION_2STRING_UINT
			'(' tSTRING ',' tSTRING ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_2STRING_UINT_STRING
			'(' tSTRING ',' tSTRING ',' INTEGER ',' tSTRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9);} |
		VOID_FUNCTION_2STRING_2UINT_2STRING
			'(' tSTRING ',' tSTRING ',' INTEGER ',' INTEGER ',' tSTRING ',' tSTRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11, $13);} |
		VOID_FUNCTION_2STRING_WPARAM_LPARAM_STRING
			'(' tSTRING ',' tSTRING ',' INTEGER ',' INTEGER ',' tSTRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11);} |
		VOID_FUNCTION_3STRING
			'(' tSTRING ',' tSTRING ',' tSTRING ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_3STRING_2UINT
			'(' tSTRING ',' tSTRING ',' tSTRING ',' INTEGER ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11);} |
		VOID_FUNCTION_4STRING
			'(' tSTRING ',' tSTRING ',' tSTRING ',' tSTRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9);} |
		VOID_FUNCTION_4STRING_UINT
			'(' tSTRING ',' tSTRING ',' tSTRING ',' tSTRING ',' INTEGER')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11);} |
		VOID_FUNCTION_4STRING_2UINT
			'(' tSTRING ',' tSTRING ',' tSTRING ',' tSTRING ',' INTEGER ',' INTEGER')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11, $13);} |
		VOID_FUNCTION_STRING_UINT
			'(' tSTRING ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5);} |
		VOID_FUNCTION_STRING_UINT_STRING
			'(' tSTRING ',' INTEGER ',' tSTRING ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_STRING_UINT_2STRING
			'(' tSTRING ',' INTEGER ',' tSTRING ',' tSTRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9);} |
		VOID_FUNCTION_STRING_WPARAM_LPARAM
			'(' tSTRING ',' INTEGER ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_UINT
			'(' INTEGER ')'
			{if (! skip) (*$1)($3);} |
		VOID_FUNCTION_2UINT
			'(' INTEGER ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5);} |
		VOID_FUNCTION_2UINT_STRING
			'(' INTEGER ',' INTEGER ',' tSTRING ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_3UINT
			'(' INTEGER ',' INTEGER ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_2INT_3UINT_STRING
			'(' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ',' tSTRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11, $13);} |
		VOID_FUNCTION_FILE_WIN
			'(' file_win ')'
			{if (! skip) (*$1)(filename, windowname);} |
		VOID_FUNCTION_FILE_WIN_STRING
			'(' file_win ',' tSTRING ')'
			{if (! skip) (*$1)(filename, windowname, $5);} |
		VOID_FUNCTION_FILE_WIN_UINT
			'(' file_win ',' INTEGER ')'
			{if (! skip) (*$1)(filename, windowname, $5);} ;

file_win:	tSTRING
                {
		  filename = windowname = $1;
		  while (*windowname && *windowname != '>') windowname++;
		  if (*windowname) *windowname++ = 0;
		} ;

bool_macro:     NOT '(' bool_macro ')' {$$ = ! $3;} |
		tSTRING {$$ = MACRO_IsMark($1);} |
		BOOL_FUNCTION_VOID '(' ')' {$$ = (*$1)();} |
		BOOL_FUNCTION_STRING '(' tSTRING ')' {$$ = (*$1)($3);} ;
