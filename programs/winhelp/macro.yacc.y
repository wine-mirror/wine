%{
/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 */

#include "macro.h"
static int skip = 0;
%}
%union
  {
    BOOL    bool;
    LPCSTR  string;
    LONG    integer;
    VOID    (*void_function_void)(VOID);
    BOOL    (*bool_function_void)(VOID);
    VOID    (*void_function_string)(LPCSTR);
    BOOL    (*bool_function_string)(LPCSTR);

    BOOL    (*bool_funktion_string)(LPCSTR);
    BOOL    (*bool_funktion_void)(VOID);
    VOID    (*void_funktion_2int_3uint_string)(LONG,LONG,LONG,LONG,LONG,LPCSTR);
    VOID    (*void_funktion_2string)(LPCSTR,LPCSTR);
    VOID    (*void_funktion_2string_2uint_2string)(LPCSTR,LPCSTR,LONG,LONG,LPCSTR,LPCSTR);
    VOID    (*void_funktion_2string_uint)(LPCSTR,LPCSTR,LONG);
    VOID    (*void_funktion_2string_uint_string)(LPCSTR,LPCSTR,LONG,LPCSTR);
    VOID    (*void_funktion_2string_wparam_lparam_string)(LPCSTR,LPCSTR,WPARAM,LPARAM,LPCSTR);
    VOID    (*void_funktion_2uint)(LONG,LONG);
    VOID    (*void_funktion_2uint_string)(LONG,LONG,LPCSTR);
    VOID    (*void_funktion_3string)(LPCSTR,LPCSTR,LPCSTR);
    VOID    (*void_funktion_3string_2uint)(LPCSTR,LPCSTR,LPCSTR,LONG,LONG);
    VOID    (*void_funktion_3uint)(LONG,LONG,LONG);
    VOID    (*void_funktion_4string)(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
    VOID    (*void_funktion_4string_2uint)(LPCSTR,LPCSTR,LPCSTR,LPCSTR,LONG,LONG);
    VOID    (*void_funktion_4string_uint)(LPCSTR,LPCSTR,LPCSTR,LPCSTR,LONG);
    VOID    (*void_funktion_string)(LPCSTR);
    VOID    (*void_funktion_string_uint)(LPCSTR,LONG);
    VOID    (*void_funktion_string_uint_2string)(LPCSTR,LONG,LPCSTR,LPCSTR);
    VOID    (*void_funktion_string_uint_string)(LPCSTR,LONG,LPCSTR);
    VOID    (*void_funktion_string_wparam_lparam)(LPCSTR,WPARAM,LPARAM);
    VOID    (*void_funktion_uint)(LONG);
    VOID    (*void_funktion_void)(VOID);
  }
%token							NOT
%token 							IF_THEN
%token 							IF_THEN_ELSE
%token <string>	 					STRING
%token <integer> 					INTEGER
%token <bool_funktion_string>				BOOL_FUNCTION_STRING
%token <bool_funktion_void>				BOOL_FUNCTION_VOID
%token <void_funktion_2int_3uint_string>		VOID_FUNCTION_2INT_3UINT_STRING
%token <void_funktion_2string>				VOID_FUNCTION_2STRING
%token <void_funktion_2string_2uint_2string>		VOID_FUNCTION_2STRING_2UINT_2STRING
%token <void_funktion_2string_uint>			VOID_FUNCTION_2STRING_UINT
%token <void_funktion_2string_uint_string>		VOID_FUNCTION_2STRING_UINT_STRING
%token <void_funktion_2string_wparam_lparam_string>	VOID_FUNCTION_2STRING_WPARAM_LPARAM_STRING
%token <void_funktion_2uint>				VOID_FUNCTION_2UINT
%token <void_funktion_2uint_string>			VOID_FUNCTION_2UINT_STRING
%token <void_funktion_3string>				VOID_FUNCTION_3STRING
%token <void_funktion_3string_2uint>			VOID_FUNCTION_3STRING_2UINT
%token <void_funktion_3uint>				VOID_FUNCTION_3UINT
%token <void_funktion_4string>				VOID_FUNCTION_4STRING
%token <void_funktion_4string_2uint>			VOID_FUNCTION_4STRING_2UINT
%token <void_funktion_4string_uint>			VOID_FUNCTION_4STRING_UINT
%token <void_funktion_string>				VOID_FUNCTION_STRING
%token <void_funktion_string_uint>			VOID_FUNCTION_STRING_UINT
%token <void_funktion_string_uint_2string>		VOID_FUNCTION_STRING_UINT_2STRING
%token <void_funktion_string_uint_string>		VOID_FUNCTION_STRING_UINT_STRING
%token <void_funktion_string_wparam_lparam>		VOID_FUNCTION_STRING_WPARAM_LPARAM
%token <void_funktion_uint>				VOID_FUNCTION_UINT
%token <void_funktion_void>				VOID_FUNCTION_VOID
%token <void_funktion_2string>				VOID_FUNCTION_FILE_WIN
%token <void_funktion_3string>				VOID_FUNCTION_FILE_WIN_STRING
%token <void_funktion_2string_uint>			VOID_FUNCTION_FILE_WIN_UINT
%type  <bool> bool_macro
%type  <string> filename
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
			'(' STRING ')'
			{if (! skip) (*$1)($3);} |
		VOID_FUNCTION_2STRING
			'(' STRING ',' STRING ')'
			{if (! skip) (*$1)($3, $5);} |
		VOID_FUNCTION_2STRING_UINT
			'(' STRING ',' STRING ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_2STRING_UINT_STRING
			'(' STRING ',' STRING ',' INTEGER ',' STRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9);} |
		VOID_FUNCTION_2STRING_2UINT_2STRING
			'(' STRING ',' STRING ',' INTEGER ',' INTEGER ',' STRING ',' STRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11, $13);} |
		VOID_FUNCTION_2STRING_WPARAM_LPARAM_STRING
			'(' STRING ',' STRING ',' INTEGER ',' INTEGER ',' STRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11);} |
		VOID_FUNCTION_3STRING
			'(' STRING ',' STRING ',' STRING ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_3STRING_2UINT
			'(' STRING ',' STRING ',' STRING ',' INTEGER ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11);} |
		VOID_FUNCTION_4STRING
			'(' STRING ',' STRING ',' STRING ',' STRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9);} |
		VOID_FUNCTION_4STRING_UINT
			'(' STRING ',' STRING ',' STRING ',' STRING ',' INTEGER')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11);} |
		VOID_FUNCTION_4STRING_2UINT
			'(' STRING ',' STRING ',' STRING ',' STRING ',' INTEGER ',' INTEGER')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11, $13);} |
		VOID_FUNCTION_STRING_UINT
			'(' STRING ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5);} |
		VOID_FUNCTION_STRING_UINT_STRING
			'(' STRING ',' INTEGER ',' STRING ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_STRING_UINT_2STRING
			'(' STRING ',' INTEGER ',' STRING ',' STRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9);} |
		VOID_FUNCTION_STRING_WPARAM_LPARAM
			'(' STRING ',' INTEGER ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_UINT
			'(' INTEGER ')'
			{if (! skip) (*$1)($3);} |
		VOID_FUNCTION_2UINT
			'(' INTEGER ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5);} |
		VOID_FUNCTION_2UINT_STRING
			'(' INTEGER ',' INTEGER ',' STRING ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_3UINT
			'(' INTEGER ',' INTEGER ',' INTEGER ')'
			{if (! skip) (*$1)($3, $5, $7);} |
		VOID_FUNCTION_2INT_3UINT_STRING
			'(' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ',' STRING ')'
			{if (! skip) (*$1)($3, $5, $7, $9, $11, $13);} |
		VOID_FUNCTION_FILE_WIN
			'(' filename STRING ')'
			{if (! skip) (*$1)($3, $4);} |
		VOID_FUNCTION_FILE_WIN_STRING
			'(' filename STRING ',' STRING ')'
			{if (! skip) (*$1)($3, $4, $6);} |
		VOID_FUNCTION_FILE_WIN_UINT
			'(' filename STRING ',' INTEGER ')'
			{if (! skip) (*$1)($3, $4, $6);} ;

filename:	{MACRO_extra_separator = '>'} STRING {$$ = $2;} ;

bool_macro:     NOT '(' bool_macro ')' {$$ = ! $3;} |
		STRING {$$ = MACRO_IsMark($1);} |
		BOOL_FUNCTION_VOID '(' ')' {$$ = (*$1)();} |
		BOOL_FUNCTION_STRING '(' STRING ')' {$$ = (*$1)($3);} ;
