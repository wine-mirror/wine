/*
 * Wrc preprocessor syntax analysis
 *
 * Copyright 1999-2000	Bertho A. Stultiens (BS)
 *
 * 24-Apr-2000 BS	Restructured the lot to fit the new scanner
 *			and reintegrate into the wine-tree.
 * 01-Jan-2000 BS	FIXME: win16 preprocessor calculates with
 *			16 bit ints and overflows...?
 * 26-Dec-1999 BS	Started this file
 *
 */

%{
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "utils.h"
#include "newstruc.h"
#include "wrc.h"
#include "preproc.h"


#define UNARY_OP(r, v, OP)					\
	switch(v.type)						\
	{							\
	case cv_sint:	r.val.si  = OP v.val.si; break;		\
	case cv_uint:	r.val.ui  = OP v.val.ui; break;		\
	case cv_slong:	r.val.sl  = OP v.val.sl; break;		\
	case cv_ulong:	r.val.ul  = OP v.val.ul; break;		\
	case cv_sll:	r.val.sll = OP v.val.sll; break;	\
	case cv_ull:	r.val.ull = OP v.val.ull; break;	\
	}

#define cv_signed(v)	((v.type & FLAG_SIGNED) != 0)

#define BIN_OP_INT(r, v1, v2, OP)			\
	r.type = v1.type;				\
	if(cv_signed(v1) && cv_signed(v2))		\
		r.val.si = v1.val.si OP v2.val.si;	\
	else if(cv_signed(v1) && !cv_signed(v2))	\
		r.val.si = v1.val.si OP v2.val.ui;	\
	else if(!cv_signed(v1) && cv_signed(v2))	\
		r.val.ui = v1.val.ui OP v2.val.si;	\
	else						\
		r.val.ui = v1.val.ui OP v2.val.ui;

#define BIN_OP_LONG(r, v1, v2, OP)			\
	r.type = v1.type;				\
	if(cv_signed(v1) && cv_signed(v2))		\
		r.val.sl = v1.val.sl OP v2.val.sl;	\
	else if(cv_signed(v1) && !cv_signed(v2))	\
		r.val.sl = v1.val.sl OP v2.val.ul;	\
	else if(!cv_signed(v1) && cv_signed(v2))	\
		r.val.ul = v1.val.ul OP v2.val.sl;	\
	else						\
		r.val.ul = v1.val.ul OP v2.val.ul;

#define BIN_OP_LONGLONG(r, v1, v2, OP)			\
	r.type = v1.type;				\
	if(cv_signed(v1) && cv_signed(v2))		\
		r.val.sll = v1.val.sll OP v2.val.sll;	\
	else if(cv_signed(v1) && !cv_signed(v2))	\
		r.val.sll = v1.val.sll OP v2.val.ull;	\
	else if(!cv_signed(v1) && cv_signed(v2))	\
		r.val.ull = v1.val.ull OP v2.val.sll;	\
	else						\
		r.val.ull = v1.val.ull OP v2.val.ull;

#define BIN_OP(r, v1, v2, OP)						\
	switch(v1.type & SIZE_MASK)					\
	{								\
	case SIZE_INT:		BIN_OP_INT(r, v1, v2, OP); break;	\
	case SIZE_LONG:		BIN_OP_LONG(r, v1, v2, OP); break;	\
	case SIZE_LONGLONG:	BIN_OP_LONGLONG(r, v1, v2, OP); break;	\
	default: internal_error(__FILE__, __LINE__, "Invalid type indicator (0x%04x)", v1.type);	\
	}


/*
 * Prototypes
 */
static int boolean(cval_t *v);
static void promote_equal_size(cval_t *v1, cval_t *v2);
static void cast_to_sint(cval_t *v);
static void cast_to_uint(cval_t *v);
static void cast_to_slong(cval_t *v);
static void cast_to_ulong(cval_t *v);
static void cast_to_sll(cval_t *v);
static void cast_to_ull(cval_t *v);
static marg_t *new_marg(char *str, def_arg_t type);
static marg_t *add_new_marg(char *str, def_arg_t type);
static int marg_index(char *id);
static mtext_t *new_mtext(char *str, int idx, def_exp_t type);
static mtext_t *combine_mtext(mtext_t *tail, mtext_t *mtp);
static char *merge_text(char *s1, char *s2);

/*
 * Local variables
 */
static marg_t **macro_args;	/* Macro parameters array while parsing */
static int	nmacro_args;

%}

%union{
	int		sint;
	unsigned int	uint;
	long		slong;
	unsigned long	ulong;
	wrc_sll_t	sll;
	wrc_ull_t	ull;
	int		*iptr;
	char		*cptr;
	cval_t		cval;
	marg_t		*marg;
	mtext_t		*mtext;
}

%token tIF tIFDEF tIFNDEF tELSE tELIF tENDIF tDEFINED tNL
%token tINCLUDE tLINE tGCCLINE tERROR tWARNING tPRAGMA tPPIDENT
%token tUNDEF tMACROEND tCONCAT tELIPSIS tSTRINGIZE
%token <cptr> tIDENT tLITERAL tMACRO tDEFINE
%token <cptr> tDQSTRING tSQSTRING tIQSTRING
%token <uint> tUINT
%token <sint> tSINT
%token <ulong> tULONG
%token <slong> tSLONG
%token <ull> tULONGLONG
%token <sll> tSLONGLONG
%right '?' ':'
%left tLOGOR
%left tLOGAND
%left '|'
%left '^'
%left '&'
%left tEQ tNE
%left '<' tLTE '>' tGTE
%left tLSHIFT tRSHIFT
%left '+' '-'
%left '*' '/'
%right '~' '!'

%type <cval>	pp_expr
%type <marg>	emargs margs
%type <mtext>	opt_mtexts mtexts mtext
%type <sint>	nums allmargs
%type <cptr>	opt_text text

/*
 **************************************************************************
 * The parser starts here
 **************************************************************************
 */

%%

pp_file	: /* Empty */
	| pp_file preprocessor
	;

preprocessor
	: tINCLUDE tDQSTRING tNL	{ do_include($2, 1); }
	| tINCLUDE tIQSTRING tNL	{ do_include($2, 0); }
	| tIF pp_expr tNL	{ next_if_state(boolean(&$2)); }
	| tIFDEF tIDENT tNL	{ next_if_state(pplookup($2) != NULL); free($2); }
	| tIFNDEF tIDENT tNL	{
		int t = pplookup($2) == NULL;
		if(include_state == 0 && t && !seen_junk)
		{
			include_state	= 1;
			include_ppp	= $2;
			include_ifdepth	= get_if_depth();
		}
		else if(include_state != 1)
		{
			include_state = -1;
			free($2);
		}
		else
			free($2);
		next_if_state(t);
		if(debuglevel & DEBUGLEVEL_PPMSG)
			fprintf(stderr, "tIFNDEF: %s:%d: include_state=%d, include_ppp='%s', include_ifdepth=%d\n", input_name, line_number, include_state, include_ppp, include_ifdepth);
		}
	| tELIF pp_expr tNL	{
		if_state_t s = pop_if();
		switch(s)
		{
		case if_true:
		case if_elif:
			push_if(if_elif);
			break;
		case if_false:
			push_if(boolean(&$2) ? if_true : if_false);
			break;
		case if_ignore:
			push_if(if_ignore);
			break;
		case if_elsetrue:
		case if_elsefalse:
			pperror("#elif cannot follow #else");
		default:
			internal_error(__FILE__, __LINE__, "Invalid if_state (%d) in #elif directive", s);
		}
		}
	| tELSE tNL		{
		if_state_t s = pop_if();
		switch(s)
		{
		case if_true:
			push_if(if_elsefalse);
			break;
		case if_elif:
			push_if(if_elif);
			break;
		case if_false:
			push_if(if_elsetrue);
			break;
		case if_ignore:
			push_if(if_ignore);
			break;
		case if_elsetrue:
		case if_elsefalse:
			pperror("#else clause already defined");
		default:
			internal_error(__FILE__, __LINE__, "Invalid if_state (%d) in #else directive", s);
		}
		}
	| tENDIF tNL		{
		pop_if();
		if(include_ifdepth == get_if_depth() && include_state == 1)
		{
			include_state = 2;
			seen_junk = 0;
		}
		else if(include_state != 1)
		{
			include_state = -1;
		}
		if(debuglevel & DEBUGLEVEL_PPMSG)
			fprintf(stderr, "tENDIF: %s:%d: include_state=%d, include_ppp='%s', include_ifdepth=%d\n", input_name, line_number, include_state, include_ppp, include_ifdepth);
		}
	| tUNDEF tIDENT tNL	{ del_define($2); free($2); }
	| tDEFINE opt_text tNL	{ add_define($1, $2); }
	| tMACRO res_arg allmargs tMACROEND opt_mtexts tNL	{
		add_macro($1, macro_args, nmacro_args, $5);
		}
	| tLINE tSINT tDQSTRING	tNL	{ fprintf(ppout, "# %d %s\n", $2 - 1, $3); free($3); }
	| tGCCLINE tDQSTRING nums tNL	{ fprintf(ppout, "# %d %s\n", $3 - 1, $2); free($2); }
	| tGCCLINE tNL		/* The null-token */
	| tERROR opt_text tNL	{ pperror("#error directive: '%s'", $2); if($2) free($2); }
	| tWARNING opt_text tNL	{ ppwarning("#warning directive: '%s'", $2); if($2) free($2); }
	| tPRAGMA opt_text tNL	{ if(pedantic) ppwarning("#pragma ignored (arg: '%s')", $2); if($2) free($2); }
	| tPPIDENT opt_text tNL	{ if(pedantic) ppwarning("#ident ignored (arg: '%s')", $2); if($2) free($2); }
	/*| tNL*/
	;

opt_text: /* Empty */	{ $$ = NULL; }
	| text		{ $$ = $1; }
	;

text	: tLITERAL		{ $$ = $1; }
	| tDQSTRING		{ $$ = $1; }
	| tSQSTRING		{ $$ = $1; }
	| text tLITERAL		{ $$ = merge_text($1, $2); }
	| text tDQSTRING	{ $$ = merge_text($1, $2); }
	| text tSQSTRING	{ $$ = merge_text($1, $2); }
	;

res_arg	: /* Empty */	{ macro_args = NULL; nmacro_args = 0; }
	;

nums	: tSINT		{ $$ = $1; }
	| nums tSINT	/* Ignore */
	;

allmargs: /* Empty */		{ $$ = 0; macro_args = NULL; nmacro_args = 0; }
	| emargs		{ $$ = nmacro_args; }
	;

emargs	: margs			{ $$ = $1; }
	| margs ',' tELIPSIS	{ $$ = add_new_marg(NULL, arg_list); nmacro_args *= -1; }
	;

margs	: margs ',' tIDENT	{ $$ = add_new_marg($3, arg_single); }
	| tIDENT		{ $$ = add_new_marg($1, arg_single); }
	;

opt_mtexts
	: /* Empty */	{ $$ = NULL; }
	| mtexts	{
		for($$ = $1; $$ && $$->prev; $$ = $$->prev)
			;
		}
	;

mtexts	: mtext		{ $$ = $1; }
	| mtexts mtext	{ $$ = combine_mtext($1, $2); }
	;

mtext	: tLITERAL	{ $$ = new_mtext($1, 0, exp_text); }
	| tDQSTRING	{ $$ = new_mtext($1, 0, exp_text); }
	| tSQSTRING	{ $$ = new_mtext($1, 0, exp_text); }
	| tCONCAT	{ $$ = new_mtext(NULL, 0, exp_concat); }
	| tSTRINGIZE tIDENT	{
		int mat = marg_index($2);
		if(mat < 0)
			pperror("Stringification identifier must be an argument parameter");
		$$ = new_mtext(NULL, mat, exp_stringize);
		}
	| tIDENT	{
		int mat = marg_index($1);
		if(mat >= 0)
			$$ = new_mtext(NULL, mat, exp_subst);
		else
			$$ = new_mtext($1, 0, exp_text);
		}
	;

pp_expr	: tSINT				{ $$.type = cv_sint;  $$.val.si = $1; }
	| tUINT				{ $$.type = cv_uint;  $$.val.ui = $1; }
	| tSLONG			{ $$.type = cv_slong; $$.val.sl = $1; }
	| tULONG			{ $$.type = cv_ulong; $$.val.ul = $1; }
	| tSLONGLONG			{ $$.type = cv_sll;   $$.val.sl = $1; }
	| tULONGLONG			{ $$.type = cv_ull;   $$.val.ul = $1; }
	| tDEFINED tIDENT		{ $$.type = cv_sint;  $$.val.si = pplookup($2) != NULL; }
	| tDEFINED '(' tIDENT ')'	{ $$.type = cv_sint;  $$.val.si = pplookup($3) != NULL; }
	| tIDENT			{ $$.type = cv_sint;  $$.val.si = 0; }
	| pp_expr tLOGOR pp_expr	{ $$.type = cv_sint; $$.val.si = boolean(&$1) || boolean(&$3); }
	| pp_expr tLOGAND pp_expr	{ $$.type = cv_sint; $$.val.si = boolean(&$1) && boolean(&$3); }
	| pp_expr tEQ pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3, ==) }
	| pp_expr tNE pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3, !=) }
	| pp_expr '<' pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3,  <) }
	| pp_expr '>' pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3,  >) }
	| pp_expr tLTE pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3, <=) }
	| pp_expr tGTE pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3, >=) }
	| pp_expr '+' pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3,  +) }
	| pp_expr '-' pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3,  -) }
	| pp_expr '^' pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3,  ^) }
	| pp_expr '&' pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3,  &) }
	| pp_expr '|' pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3,  |) }
	| pp_expr '*' pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3,  *) }
	| pp_expr '/' pp_expr		{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3,  /) }
	| pp_expr tLSHIFT pp_expr	{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3, <<) }
	| pp_expr tRSHIFT pp_expr	{ promote_equal_size(&$1, &$3); BIN_OP($$, $1, $3, >>) }
	| '+' pp_expr			{ $$ =  $2; }
	| '-' pp_expr			{ UNARY_OP($$, $2, -) }
	| '~' pp_expr			{ UNARY_OP($$, $2, ~) }
	| '!' pp_expr			{ $$.type = cv_sint; $$.val.si = !boolean(&$2); }
	| '(' pp_expr ')'		{ $$ =  $2; }
	| pp_expr '?' pp_expr ':' pp_expr { $$ = boolean(&$1) ? $3 : $5; }
	;

%%

/*
 **************************************************************************
 * Support functions
 **************************************************************************
 */

static void cast_to_sint(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	break;
	case cv_uint:	break;
	case cv_slong:	v->val.si = v->val.sl;	break;
	case cv_ulong:	v->val.si = v->val.ul;	break;
	case cv_sll:	v->val.si = v->val.sll;	break;
	case cv_ull:	v->val.si = v->val.ull;	break;
	}
	v->type = cv_sint;
}

static void cast_to_uint(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	break;
	case cv_uint:	break;
	case cv_slong:	v->val.ui = v->val.sl;	break;
	case cv_ulong:	v->val.ui = v->val.ul;	break;
	case cv_sll:	v->val.ui = v->val.sll;	break;
	case cv_ull:	v->val.ui = v->val.ull;	break;
	}
	v->type = cv_uint;
}

static void cast_to_slong(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	v->val.sl = v->val.si;	break;
	case cv_uint:	v->val.sl = v->val.ui;	break;
	case cv_slong:	break;
	case cv_ulong:	break;
	case cv_sll:	v->val.sl = v->val.sll;	break;
	case cv_ull:	v->val.sl = v->val.ull;	break;
	}
	v->type = cv_slong;
}

static void cast_to_ulong(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	v->val.ul = v->val.si;	break;
	case cv_uint:	v->val.ul = v->val.ui;	break;
	case cv_slong:	break;
	case cv_ulong:	break;
	case cv_sll:	v->val.ul = v->val.sll;	break;
	case cv_ull:	v->val.ul = v->val.ull;	break;
	}
	v->type = cv_ulong;
}

static void cast_to_sll(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	v->val.sll = v->val.si;	break;
	case cv_uint:	v->val.sll = v->val.ui;	break;
	case cv_slong:	v->val.sll = v->val.sl;	break;
	case cv_ulong:	v->val.sll = v->val.ul;	break;
	case cv_sll:	break;
	case cv_ull:	break;
	}
	v->type = cv_sll;
}

static void cast_to_ull(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	v->val.ull = v->val.si;	break;
	case cv_uint:	v->val.ull = v->val.ui;	break;
	case cv_slong:	v->val.ull = v->val.sl;	break;
	case cv_ulong:	v->val.ull = v->val.ul;	break;
	case cv_sll:	break;
	case cv_ull:	break;
	}
	v->type = cv_ull;
}


static void promote_equal_size(cval_t *v1, cval_t *v2)
{
#define cv_sizeof(v)	((int)(v->type & SIZE_MASK))
	int s1 = cv_sizeof(v1);
	int s2 = cv_sizeof(v2);
#undef cv_sizeof

	if(s1 == s2)
		return;
	else if(s1 > s2)
	{
		switch(v1->type)
		{
		case cv_sint:	cast_to_sint(v2); break;
		case cv_uint:	cast_to_uint(v2); break;
		case cv_slong:	cast_to_slong(v2); break;
		case cv_ulong:	cast_to_ulong(v2); break;
		case cv_sll:	cast_to_sll(v2); break;
		case cv_ull:	cast_to_ull(v2); break;
		}
	}
	else
	{
		switch(v2->type)
		{
		case cv_sint:	cast_to_sint(v1); break;
		case cv_uint:	cast_to_uint(v1); break;
		case cv_slong:	cast_to_slong(v1); break;
		case cv_ulong:	cast_to_ulong(v1); break;
		case cv_sll:	cast_to_sll(v1); break;
		case cv_ull:	cast_to_ull(v1); break;
		}
	}
}


static int boolean(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	return v->val.si != (int)0;
	case cv_uint:	return v->val.ui != (unsigned int)0;
	case cv_slong:	return v->val.sl != (long)0;
	case cv_ulong:	return v->val.ul != (unsigned long)0;
	case cv_sll:	return v->val.sll != (wrc_sll_t)0;
	case cv_ull:	return v->val.ull != (wrc_ull_t)0;
	}
	return 0;
}

static marg_t *new_marg(char *str, def_arg_t type)
{
	marg_t *ma = (marg_t *)xmalloc(sizeof(marg_t));
	ma->arg = str;
	ma->type = type;
	return ma;
}

static marg_t *add_new_marg(char *str, def_arg_t type)
{
	marg_t *ma = new_marg(str, type);
	nmacro_args++;
	macro_args = (marg_t **)xrealloc(macro_args, nmacro_args * sizeof(macro_args[0]));
	macro_args[nmacro_args-1] = ma;
	return ma;
}

static int marg_index(char *id)
{
	int t;
	for(t = 0; t < nmacro_args; t++)
	{
		if(!strcmp(id, macro_args[t]->arg))
			break;
	}
	return t < nmacro_args ? t : -1;
}

static mtext_t *new_mtext(char *str, int idx, def_exp_t type)
{
	mtext_t *mt = (mtext_t *)xmalloc(sizeof(mtext_t));
	if(str == NULL) 
		mt->subst.argidx = idx;
	else
		mt->subst.text = str;
	mt->type = type;
	return mt;
}

static mtext_t *combine_mtext(mtext_t *tail, mtext_t *mtp)
{
	if(!tail)
		return mtp;

	if(!mtp)
		return tail;

	if(tail->type == exp_text && mtp->type == exp_text)
	{
		tail->subst.text = xrealloc(tail->subst.text, strlen(tail->subst.text)+strlen(mtp->subst.text)+1);
		strcat(tail->subst.text, mtp->subst.text);
		free(mtp->subst.text);
		free(mtp);
		return tail;
	}

	if(tail->type == exp_concat && mtp->type == exp_concat)
	{
		free(mtp);
		return tail;
	}

	if(tail->type == exp_concat && mtp->type == exp_text)
	{
		int len = strlen(mtp->subst.text);
		while(len)
		{
/* FIXME: should delete space from head of string */
			if(isspace(mtp->subst.text[len-1] & 0xff))
				mtp->subst.text[--len] = '\0';
			else
				break;
		}

		if(!len)
		{
			free(mtp->subst.text);
			free(mtp);
			return tail;
		}
	}

	if(tail->type == exp_text && mtp->type == exp_concat)
	{
		int len = strlen(tail->subst.text);
		while(len)
		{
			if(isspace(tail->subst.text[len-1] & 0xff))
				tail->subst.text[--len] = '\0';
			else
				break;
		}

		if(!len)
		{
			mtp->prev = tail->prev;
			mtp->next = tail->next;
			if(tail->prev)
				tail->prev->next = mtp;
			free(tail->subst.text);
			free(tail);
			return mtp;
		}
	}

	tail->next = mtp;
	mtp->prev = tail;

	return mtp;
}

static char *merge_text(char *s1, char *s2)
{
	int l1 = strlen(s1);
	int l2 = strlen(s2);
	s1 = xrealloc(s1, l1+l2+1);
	memcpy(s1+l1, s2, l2+1);
	free(s2);
	return s1;
}

