%{
/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"

#if defined(YYBYACC)
	/* Berkeley yacc (byacc) doesn't seem to know about these */
	/* Some *BSD supplied versions do define these though */
# ifndef YYEMPTY
#  define YYEMPTY	(-1)	/* Empty lookahead value of yychar */
# endif
# ifndef YYLEX
#  define YYLEX		yylex()
# endif

#elif defined(YYBISON)
	/* Bison was used for original development */
	/* #define YYEMPTY -2 */
	/* #define YYLEX   yylex() */

#else
	/* No yacc we know yet */
# if !defined(YYEMPTY) || !defined(YYLEX)
#  error Yacc version/type unknown. This version needs to be verified for settings of YYEMPTY and YYLEX.
# elif defined(__GNUC__)	/* gcc defines the #warning directive */
#  warning Yacc version/type unknown. It defines YYEMPTY and YYLEX, but is not tested
  /* #else we just take a chance that it works... */
# endif
#endif

static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, DWORD val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_t *make_expr(enum expr_type type);
static expr_t *make_exprl(enum expr_type type, long val);
static expr_t *make_exprs(enum expr_type type, char *val);
static expr_t *make_expr1(enum expr_type type, expr_t *expr);
static expr_t *make_expr2(enum expr_type type, expr_t *exp1, expr_t *exp2);
static type_t *make_type(BYTE type, type_t *ref);
static typeref_t *make_tref(char *name, type_t *ref);
static typeref_t *uniq_tref(typeref_t *ref);
static type_t *type_ref(typeref_t *ref);
static void set_type(var_t *v, typeref_t *ref, expr_t *arr);
static var_t *make_var(char *name);
static func_t *make_func(var_t *def, var_t *args);

static type_t *reg_type(type_t *type, char *name, int t);
static type_t *reg_types(type_t *type, var_t *names, int t);
static type_t *find_type(char *name, int t);
static type_t *find_type2(char *name, int t);
static type_t *get_type(BYTE type, char *name, int t);
static type_t *get_typev(BYTE type, var_t *name, int t);

#define tsENUM   1
#define tsSTRUCT 2
#define tsUNION  3

static type_t std_bool = { "boolean" };
static type_t std_int = { "int" };

%}
%union {
	attr_t *attr;
	expr_t *expr;
	type_t *type;
	typeref_t *tref;
	var_t *var;
	func_t *func;
	char *str;
	uuid_t *uuid;
	int num;
}

%token <str> aIDENTIFIER
%token <str> aKNOWNTYPE
%token <num> aNUM
%token <str> aSTRING
%token <uuid> aUUID
%token aEOF
%token SHL SHR
%token tAGGREGATABLE tALLOCATE tAPPOBJECT tARRAYS tASYNC tASYNCUUID
%token tAUTOHANDLE tBINDABLE tBOOLEAN tBROADCAST tBYTE tBYTECOUNT
%token tCALLAS tCALLBACK tCASE tCDECL tCHAR tCOCLASS tCODE tCOMMSTATUS
%token tCONST tCONTEXTHANDLE tCONTEXTHANDLENOSERIALIZE
%token tCONTEXTHANDLESERIALIZE tCONTROL tCPPQUOTE
%token tDEFAULT
%token tDOUBLE
%token tENUM
%token tEXTERN
%token tFLOAT
%token tHYPER
%token tIIDIS
%token tIMPORT tIMPORTLIB
%token tIN tINCLUDE tINLINE
%token tINT tINT64
%token tINTERFACE
%token tLENGTHIS
%token tLOCAL
%token tLONG
%token tOBJECT tODL tOLEAUTOMATION
%token tOUT
%token tPOINTERDEFAULT
%token tREF
%token tSHORT
%token tSIGNED
%token tSIZEIS tSIZEOF
%token tSTDCALL
%token tSTRING tSTRUCT
%token tSWITCH tSWITCHIS tSWITCHTYPE
%token tTYPEDEF
%token tUNION
%token tUNIQUE
%token tUNSIGNED
%token tUUID
%token tV1ENUM
%token tVERSION
%token tVOID
%token tWCHAR tWIREMARSHAL

/* used in attr_t */
%token tPOINTERTYPE

%type <attr> m_attributes attributes attrib_list attribute
%type <expr> aexprs aexpr_list aexpr array
%type <type> inherit interface interfacehdr interfacedef lib_statements
%type <type> base_type int_std
%type <type> enumdef structdef typedef uniondef
%type <tref> type
%type <var> m_args no_args args arg
%type <var> fields field s_field cases case enums enum_list enum constdef
%type <var> m_ident t_ident ident p_ident pident pident_list
%type <func> funcdef int_statements
%type <num> expr pointer_type

%left ','
%left '|'
%left '&'
%left '-' '+'
%left '*' '/'
%left SHL SHR
%right CAST
%right PPTR
%right NEG

%%

input:	  lib_statements			{ /* FIXME */ }
	;

lib_statements:					{ $$ = NULL; }
	| lib_statements interface ';'		{ if (!parse_only) write_forward($2); }
	| lib_statements interfacedef		{ LINK($2, $1); $$ = $2; }
/*	| lib_statements librarydef (when implemented) */
	| lib_statements statement
	;

int_statements:					{ $$ = NULL; }
	| int_statements funcdef ';'		{ LINK($2, $1); $$ = $2; }
	| int_statements statement
	;

statement: ';'					{}
	| constdef				{}
	| cppquote				{}
	| enumdef ';'				{ if (!parse_only) { write_type(header, $1, NULL, NULL); fprintf(header, ";\n\n"); } }
	| externdef ';'				{}
	| import				{}
/*	| interface ';'				{} */
/*	| interfacedef				{} */
	| structdef ';'				{ if (!parse_only) { write_type(header, $1, NULL, NULL); fprintf(header, ";\n\n"); } }
	| typedef ';'				{}
	| uniondef ';'				{ if (!parse_only) { write_type(header, $1, NULL, NULL); fprintf(header, ";\n\n"); } }
	;

cppquote:	tCPPQUOTE '(' aSTRING ')'	{ if (!parse_only) fprintf(header, "%s\n", $3); }
	;
import_start:	tIMPORT aSTRING ';'		{ do_import($2); }
	;
import:		import_start input aEOF		{}
	;

m_args:						{ $$ = NULL; }
	| args
	;

no_args:  tVOID					{ $$ = NULL; }
	;

args:	  arg
	| args ',' arg				{ LINK($3, $1); $$ = $3; }
	| no_args
	;

/* split into two rules to get bison to resolve a tVOID conflict */
arg:	  attributes type pident array		{ $$ = $3;
						  set_type($$, $2, $4);
						  $$->attrs = $1;
						}
	| type pident array			{ $$ = $2;
						  set_type($$, $1, $3);
						}
	;

aexprs:						{ $$ = make_expr(EXPR_VOID); }
	| aexpr_list
	;

aexpr_list: aexpr
	| aexpr_list ',' aexpr			{ LINK($3, $1); $$ = $3; }
	;

aexpr:	  aNUM					{ $$ = make_exprl(EXPR_NUM, $1); }
	| aIDENTIFIER				{ $$ = make_exprs(EXPR_IDENTIFIER, $1); }
	| aexpr '|' aexpr			{ $$ = make_expr2(EXPR_OR , $1, $3); }
	| aexpr '&' aexpr			{ $$ = make_expr2(EXPR_AND, $1, $3); }
	| aexpr '+' aexpr			{ $$ = make_expr2(EXPR_ADD, $1, $3); }
	| aexpr '-' aexpr			{ $$ = make_expr2(EXPR_SUB, $1, $3); }
	| aexpr '*' aexpr			{ $$ = make_expr2(EXPR_MUL, $1, $3); }
	| aexpr '/' aexpr			{ $$ = make_expr2(EXPR_DIV, $1, $3); }
	| aexpr SHL aexpr			{ $$ = make_expr2(EXPR_SHL, $1, $3); }
	| aexpr SHR aexpr			{ $$ = make_expr2(EXPR_SHR, $1, $3); }
	| '-' aexpr %prec NEG			{ $$ = make_expr1(EXPR_NEG, $2); }
	| '*' aexpr %prec PPTR			{ $$ = make_expr1(EXPR_PPTR, $2); }
	| '(' type ')' aexpr %prec CAST		{ $$ = $4; /* FIXME */ free($2); }
	| '(' aexpr ')'				{ $$ = $2; }
	| tSIZEOF '(' type ')'			{ $$ = make_exprl(EXPR_NUM, 0); /* FIXME */ free($3); warning("can't do sizeof() yet\n"); }
	;

array:						{ $$ = NULL; }
	| '[' aexprs ']'			{ $$ = $2; }
	| '[' '*' ']'				{ $$ = make_expr(EXPR_VOID); }
	;

m_attributes:					{ $$ = NULL; }
	| attributes
	;

attributes:
	  m_attributes '[' attrib_list ']'	{ LINK_LAST($3, $1); $$ = $3; }
	;

attrib_list: attribute
	| attrib_list ',' attribute		{ LINK_SAFE($3, $1); $$ = $3; /* FIXME: don't use SAFE */ }
	;

attribute:
	  tASYNC				{ $$ = make_attr(ATTR_ASYNC); }
	| tCALLAS '(' ident ')'			{ $$ = make_attrp(ATTR_CALLAS, $3); }
	| tCASE '(' expr_list ')'		{ $$ = NULL; }
	| tCONTEXTHANDLE			{ $$ = NULL; }
	| tCONTEXTHANDLENOSERIALIZE		{ $$ = NULL; }
	| tCONTEXTHANDLESERIALIZE		{ $$ = NULL; }
	| tDEFAULT				{ $$ = make_attr(ATTR_DEFAULT); }
	| tIIDIS '(' ident ')'			{ $$ = make_attrp(ATTR_IIDIS, $3); }
	| tIN					{ $$ = make_attr(ATTR_IN); }
	| tLENGTHIS '(' aexprs ')'		{ $$ = NULL; }
	| tLOCAL				{ $$ = make_attr(ATTR_LOCAL); }
	| tOBJECT				{ $$ = make_attr(ATTR_OBJECT); }
	| tOLEAUTOMATION			{ $$ = make_attr(ATTR_OLEAUTOMATION); }
	| tOUT					{ $$ = make_attr(ATTR_OUT); }
	| tPOINTERDEFAULT '(' pointer_type ')'	{ $$ = make_attrv(ATTR_POINTERDEFAULT, $3); }
	| tSIZEIS '(' aexprs ')'		{ $$ = NULL; }
	| tSTRING				{ $$ = make_attr(ATTR_STRING); }
	| tSWITCHIS '(' aexpr ')'		{ $$ = NULL; }
	| tSWITCHTYPE '(' type ')'		{ $$ = NULL; }
	| tUUID '(' aUUID ')'			{ $$ = make_attrp(ATTR_UUID, $3); }
	| tV1ENUM				{ $$ = make_attr(ATTR_V1ENUM); }
	| tVERSION '(' version ')'		{ $$ = NULL; }
	| tWIREMARSHAL '(' type ')'		{ $$ = make_attrp(ATTR_WIREMARSHAL, type_ref($3)); }
	| pointer_type				{ $$ = make_attrv(ATTR_POINTERTYPE, $1); }
	;

callconv:
	| tSTDCALL
	;

cases:						{ $$ = NULL; }
	| cases case				{ LINK($2, $1); $$ = $2; }
	;

case:	  tCASE expr ':' field			{ $$ = $4; }
	| tDEFAULT ':' field			{ $$ = $3; }
	;

constdef: tCONST type ident '=' expr		{ $$ = $3;
						  set_type($$, $2, NULL);
						  $$->has_val = TRUE;
						  $$->lval = $5;
						}
	;

enums:						{ $$ = NULL; }
	| enum_list ','				{ $$ = $1; }
	| enum_list
	;

enum_list: enum
	| enum_list ',' enum			{ LINK($3, $1); $$ = $3; }
	;

enum:	  ident '=' expr			{ $$ = $1;
						  $$->has_val = TRUE;
						  $$->lval = $3;
						}
	| ident					{ $$ = $1; }
	;

enumdef: tENUM t_ident '{' enums '}'		{ $$ = get_typev(RPC_FC_ENUM16, $2, tsENUM);
						  $$->fields = $4;
						  $$->defined = TRUE;
						}
	;

expr_list: expr					{}
	| expr_list ',' expr			{}
	;

expr:	  aNUM
	| aIDENTIFIER				{ $$ = 0; /* FIXME */ }
	| expr '|' expr				{ $$ = $1 | $3; }
	| expr '&' expr				{ $$ = $1 & $3; }
	| expr SHL expr				{ $$ = $1 << $3; }
	| expr SHR expr				{ $$ = $1 >> $3; }
	| '-' expr %prec NEG			{ $$ = -$2; }
	;

externdef: tEXTERN tCONST type ident
	;

fields:						{ $$ = NULL; }
	| fields field				{ LINK($2, $1); $$ = $2; }
	;

field:	  s_field ';'				{ $$ = $1; }
	| m_attributes uniondef ';'		{ $$ = make_var(NULL); $$->type = $2; $$->attrs = $1; }
	| attributes ';'			{ $$ = make_var(NULL); $$->attrs = $1; }
	| ';'					{ $$ = NULL; }
	;

s_field:  m_attributes type pident array	{ $$ = $3; set_type($$, $2, $4); $$->attrs = $1; }
	;

funcdef:
	  m_attributes type callconv pident
	  '(' m_args ')'			{ set_type($4, $2, NULL);
						  $4->attrs = $1;
						  $$ = make_func($4, $6);
						}
	;

m_ident:					{ $$ = NULL; }
	| ident
	;

t_ident:					{ $$ = NULL; }
	| aIDENTIFIER				{ $$ = make_var($1); }
	| aKNOWNTYPE				{ $$ = make_var($1); }
	;

ident:	  aIDENTIFIER				{ $$ = make_var($1); }
	;

base_type: tBYTE				{ $$ = make_type(RPC_FC_BYTE, NULL); }
	| tWCHAR				{ $$ = make_type(RPC_FC_WCHAR, NULL); }
	| int_std
	| tSIGNED int_std			{ $$ = $2; $$->sign = 1; }
	| tUNSIGNED int_std			{ $$ = $2; $$->sign = -1;
						  switch ($$->type) {
						  case RPC_FC_SMALL: $$->type = RPC_FC_USMALL; break;
						  case RPC_FC_SHORT: $$->type = RPC_FC_USHORT; break;
						  case RPC_FC_LONG:  $$->type = RPC_FC_ULONG;  break;
						  default: break;
						  }
						}
	| tFLOAT				{ $$ = make_type(RPC_FC_FLOAT, NULL); }
	| tDOUBLE				{ $$ = make_type(RPC_FC_DOUBLE, NULL); }
	| tBOOLEAN				{ $$ = make_type(RPC_FC_BYTE, &std_bool); /* ? */ }
	;

m_int:
	| tINT
	;

int_std:  tINT					{ $$ = make_type(RPC_FC_LONG, &std_int); } /* win32 only */
	| tSHORT m_int				{ $$ = make_type(RPC_FC_SHORT, NULL); }
	| tLONG m_int				{ $$ = make_type(RPC_FC_LONG, NULL); }
	| tHYPER m_int				{ $$ = make_type(RPC_FC_HYPER, NULL); }
	| tINT64				{ $$ = make_type(RPC_FC_HYPER, NULL); }
	| tCHAR					{ $$ = make_type(RPC_FC_CHAR, NULL); }
	;

inherit:					{ $$ = NULL; }
	| ':' aKNOWNTYPE			{ $$ = find_type2($2, 0); }
	;

interface: tINTERFACE aIDENTIFIER		{ $$ = get_type(RPC_FC_IP, $2, 0); }
	|  tINTERFACE aKNOWNTYPE		{ $$ = get_type(RPC_FC_IP, $2, 0); }
	;

interfacehdr: attributes interface		{ $$ = $2;
						  if ($$->defined) yyerror("multiple definition error\n");
						  $$->attrs = $1;
						  $$->defined = TRUE;
						  if (!parse_only) write_forward($$);
						}
	;

interfacedef: interfacehdr inherit
	  '{' int_statements '}'		{ $$ = $1;
						  $$->ref = $2;
						  $$->funcs = $4;
						  if (!parse_only) write_interface($$);
						}
/* MIDL is able to import the definition of a base class from inside the
 * definition of a derived class, I'll try to support it with this rule */
	| interfacehdr ':' aIDENTIFIER
	  '{' import int_statements '}'		{ $$ = $1;
						  $$->ref = find_type2($3, 0);
						  if (!$$->ref) yyerror("base class %s not found in import\n", $3);
						  $$->funcs = $6;
						  if (!parse_only) write_interface($$);
						}
	;

p_ident:  '*' pident %prec PPTR			{ $$ = $2; $$->ptr_level++; }
	| tCONST p_ident			{ $$ = $2; /* FIXME */ }
	;

pident:	  ident
	| p_ident
	| '(' pident ')'			{ $$ = $2; }
	;

pident_list:
	  pident
	| pident_list ',' pident		{ LINK($3, $1); $$ = $3; }
	;

pointer_type:
	  tREF					{ $$ = RPC_FC_RP; }
	| tUNIQUE				{ $$ = RPC_FC_UP; }
	;

structdef: tSTRUCT t_ident '{' fields '}'	{ $$ = get_typev(RPC_FC_STRUCT, $2, tsSTRUCT);
						  $$->fields = $4;
						  $$->defined = TRUE;
						}
	;

type:	  tVOID					{ $$ = make_tref(NULL, make_type(0, NULL)); }
	| aKNOWNTYPE				{ $$ = make_tref($1, find_type($1, 0)); }
	| base_type				{ $$ = make_tref(NULL, $1); }
	| tCONST type				{ $$ = uniq_tref($2); $$->ref->is_const = TRUE; }
	| enumdef				{ $$ = make_tref(NULL, $1); }
	| tENUM aIDENTIFIER			{ $$ = make_tref(NULL, find_type2($2, tsENUM)); }
	| structdef				{ $$ = make_tref(NULL, $1); }
	| tSTRUCT aIDENTIFIER			{ $$ = make_tref(NULL, get_type(RPC_FC_STRUCT, $2, tsSTRUCT)); }
	| uniondef				{ $$ = make_tref(NULL, $1); }
	| tUNION aIDENTIFIER			{ $$ = make_tref(NULL, find_type2($2, tsUNION)); }
	;

typedef: tTYPEDEF m_attributes type pident_list	{ typeref_t *tref = uniq_tref($3);
						  $4->tname = tref->name;
						  tref->name = NULL;
						  $$ = type_ref(tref);
						  $$->attrs = $2;
						  if (!parse_only) write_typedef($$, $4);
						  reg_types($$, $4, 0);
						}
	;

uniondef: tUNION t_ident '{' fields '}'		{ $$ = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, $2, tsUNION);
						  $$->fields = $4;
						  $$->defined = TRUE;
						}
	| tUNION t_ident
	  tSWITCH '(' s_field ')'
	  m_ident '{' cases '}'			{ $$ = get_typev(RPC_FC_ENCAPSULATED_UNION, $2, tsUNION);
						  LINK($5, $9); $$->fields = $5;
						  $$->defined = TRUE;
						}
	;

version:
	  aNUM					{}
	| aNUM '.' aNUM				{}
	;

%%

static attr_t *make_attr(enum attr_type type)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = 0;
  INIT_LINK(a);
  return a;
}

static attr_t *make_attrv(enum attr_type type, DWORD val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = val;
  INIT_LINK(a);
  return a;
}

static attr_t *make_attrp(enum attr_type type, void *val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.pval = val;
  INIT_LINK(a);
  return a;
}

static expr_t *make_expr(enum expr_type type)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = 0;
  INIT_LINK(e);
  return e;
}

static expr_t *make_exprl(enum expr_type type, long val)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = val;
  INIT_LINK(e);
  return e;
}

static expr_t *make_exprs(enum expr_type type, char *val)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  /* FIXME: if type is EXPR_IDENTIFIER, we could check for match against const
   * declaration, and change to appropriate type and value if so */
  e->type = type;
  e->ref = NULL;
  e->u.sval = val;
  INIT_LINK(e);
  return e;
}

static expr_t *make_expr1(enum expr_type type, expr_t *expr)
{
  expr_t *e;
  /* check for compile-time optimization */
  if (expr->type == EXPR_NUM) {
    switch (type) {
    case EXPR_NEG:
      expr->u.lval = -expr->u.lval;
      return expr;
    default:
    }
  }
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr;
  e->u.lval = 0;
  INIT_LINK(e);
  return e;
}

static expr_t *make_expr2(enum expr_type type, expr_t *expr1, expr_t *expr2)
{
  expr_t *e;
  /* check for compile-time optimization */
  if (expr1->type == EXPR_NUM && expr2->type == EXPR_NUM) {
    switch (type) {
    case EXPR_ADD:
      expr1->u.lval += expr2->u.lval;
      free(expr2);
      return expr1;
    case EXPR_SUB:
      expr1->u.lval -= expr2->u.lval;
      free(expr2);
      return expr1;
    case EXPR_MUL:
      expr1->u.lval *= expr2->u.lval;
      free(expr2);
      return expr1;
    case EXPR_DIV:
      expr1->u.lval /= expr2->u.lval;
      free(expr2);
      return expr1;
    case EXPR_OR:
      expr1->u.lval |= expr2->u.lval;
      free(expr2);
      return expr1;
    case EXPR_AND:
      expr1->u.lval &= expr2->u.lval;
      free(expr2);
      return expr1;
    case EXPR_SHL:
      expr1->u.lval <<= expr2->u.lval;
      free(expr2);
      return expr1;
    case EXPR_SHR:
      expr1->u.lval >>= expr2->u.lval;
      free(expr2);
      return expr1;
    default:
    }
  }
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr1;
  e->u.ext = expr2;
  INIT_LINK(e);
  return e;
}

static type_t *make_type(BYTE type, type_t *ref)
{
  type_t *t = xmalloc(sizeof(type_t));
  t->name = NULL;
  t->type = type;
  t->ref = ref;
  t->rname = NULL;
  t->attrs = NULL;
  t->funcs = NULL;
  t->fields = NULL;
  t->ignore = parse_only;
  t->is_const = FALSE;
  t->sign = 0;
  t->defined = FALSE;
  t->written = FALSE;
  INIT_LINK(t);
  return t;
}

static typeref_t *make_tref(char *name, type_t *ref)
{
  typeref_t *t = xmalloc(sizeof(typeref_t));
  t->name = name;
  t->ref = ref;
  t->uniq = ref ? 0 : 1;
  return t;
}

static typeref_t *uniq_tref(typeref_t *ref)
{
  typeref_t *t = ref;
  type_t *tp;
  if (t->uniq) return t;
  tp = make_type(0, t->ref);
  tp->name = t->name;
  t->name = NULL;
  t->ref = tp;
  t->uniq = 1;
  return t;
}

static type_t *type_ref(typeref_t *ref)
{
  type_t *t = ref->ref;
  if (ref->name) free(ref->name);
  free(ref);
  return t;
}

static void set_type(var_t *v, typeref_t *ref, expr_t *arr)
{
  v->type = ref->ref;
  v->tname = ref->name;
  ref->name = NULL;
  free(ref);
  v->array = arr;
}

static var_t *make_var(char *name)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->ptr_level = 0;
  v->type = NULL;
  v->tname = NULL;
  v->attrs = NULL;
  v->array = NULL;
  v->has_val = FALSE;
  v->lval = 0;
  INIT_LINK(v);
  return v;
}

static func_t *make_func(var_t *def, var_t *args)
{
  func_t *f = xmalloc(sizeof(func_t));
  f->def = def;
  f->args = args;
  f->ignore = parse_only;
  f->idx = -1;
  INIT_LINK(f);
  return f;
}

struct rtype {
  char *name;
  type_t *type;
  int t;
  struct rtype *next;
};

struct rtype *first_type;

static type_t *reg_type(type_t *type, char *name, int t)
{
  struct rtype *nt;
  if (!name) {
    yyerror("registering named type without name\n");
    return type;
  }
  nt = xmalloc(sizeof(struct rtype));
  nt->name = name;
  nt->type = type;
  nt->t = t;
  nt->next = first_type;
  first_type = nt;
  return type;
}

static type_t *reg_types(type_t *type, var_t *names, int t)
{
  type_t *ptr = type;
  int ptrc = 0;

  while (names) {
    var_t *next = NEXT_LINK(names);
    if (names->name) {
      type_t *cur = ptr;
      int cptr = names->ptr_level;
      if (cptr > ptrc) {
        while (cptr > ptrc) {
          cur = ptr = make_type(RPC_FC_FP, cur); /* FIXME: pointer type from attrs? */
          ptrc++;
        }
      } else {
        while (cptr < ptrc) {
          cur = cur->ref;
          cptr++;
        }
      }
      reg_type(cur, names->name, t);
    }
    free(names);
    names = next;
  }
  return type;
}

static type_t *find_type(char *name, int t)
{
  struct rtype *cur = first_type;
  while (cur && (cur->t != t || strcmp(cur->name, name)))
    cur = cur->next;
  if (!cur) {
    yyerror("type %s not found\n", name);
    return NULL;
  }
  return cur->type;
}

static type_t *find_type2(char *name, int t)
{
  type_t *tp = find_type(name, t);
  free(name);
  return tp;
}

int is_type(const char *name)
{
  struct rtype *cur = first_type;
  while (cur && (cur->t || strcmp(cur->name, name)))
    cur = cur->next;
  if (cur) return TRUE;
  return FALSE;
}

static type_t *get_type(BYTE type, char *name, int t)
{
  struct rtype *cur = NULL;
  type_t *tp;
  if (name) {
    cur = first_type;
    while (cur && (cur->t != t || strcmp(cur->name, name)))
      cur = cur->next;
  }
  if (cur) {
    free(name);
    return cur->type;
  }
  tp = make_type(type, NULL);
  tp->name = name;
  if (!name) return tp;
  return reg_type(tp, name, t);
}

static type_t *get_typev(BYTE type, var_t *name, int t)
{
  char *sname = NULL;
  if (name) {
    sname = name->name;
    free(name);
  }
  return get_type(type, sname, t);
}
