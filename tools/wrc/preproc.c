/*
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wrc.h"
#include "utils.h"
#include "preproc.h"


extern void set_pp_ignore(int);	/* From parser.l */

#define HASHKEY		2039
static pp_entry_t *pp_defines[HASHKEY];

#define MAXIFSTACK	64
static if_state_t if_stack[MAXIFSTACK];
static int if_stack_idx = 0;

#if 0
void pp_status(void) __attribute__((destructor));
void pp_status(void)
{
	int i;
	int sum;
	int total = 0;
	pp_entry_t *ppp;

	fprintf(stderr, "Defines statistics:\n");
	for(i = 0; i < HASHKEY; i++)
	{
		sum = 0;
		for(ppp = pp_defines[i]; ppp; ppp = ppp->next)
			sum++;
		total += sum;
		fprintf(stderr, "%4d, %3d\n", i, sum);
	}
	fprintf(stderr, "Total defines: %d\n", total);
}
#endif

/* Don't comment on the hash, its primitive but functional... */
int pphash(char *str)
{
	int sum = 0;
	while(*str)
		sum += *str++;
	return sum % HASHKEY;
}

pp_entry_t *pplookup(char *ident)
{
	int idx = pphash(ident);
	pp_entry_t *ppp;

	for(ppp = pp_defines[idx]; ppp; ppp = ppp->next)
	{
		if(!strcmp(ident, ppp->ident))
			return ppp;
	}
	return NULL;
}

void del_define(char *name)
{
	int idx;
	pp_entry_t *ppp;

	if((ppp = pplookup(name)) == NULL)
	{
		if(pedantic)
			yywarning("%s was not defined", name);
		return;
	}

	if(ppp->iep)
	{
		if(debuglevel & DEBUGLEVEL_PPMSG)
			fprintf(stderr, "del_define: %s:%d: includelogic removed, include_ppp='%s', file=%s\n", input_name, line_number, name, ppp->iep->filename);
		if(ppp->iep == includelogiclist)
		{
			includelogiclist = ppp->iep->next;
			if(includelogiclist)
				includelogiclist->prev = NULL;
		}
		else
		{
			ppp->iep->prev->next = ppp->iep->next;
			if(ppp->iep->next)
				ppp->iep->next->prev = ppp->iep->prev;
		}
		free(ppp->iep->filename);
		free(ppp->iep);
	}

	idx = pphash(name);
	if(pp_defines[idx] == ppp)
	{
		pp_defines[idx] = ppp->next;
		if(pp_defines[idx])
			pp_defines[idx]->prev = NULL;
	}
	else
	{
		ppp->prev->next = ppp->next;
		if(ppp->next)
			ppp->next->prev = ppp->prev;
	}

	free(ppp);

	if(debuglevel & DEBUGLEVEL_PPMSG)
		printf("Deleted (%s, %d) <%s>\n", input_name, line_number, name);
}

pp_entry_t *add_define(char *def, char *text)
{
	int len;
	char *cptr;
	int idx = pphash(def);
	pp_entry_t *ppp;

	if((ppp = pplookup(def)) != NULL)
	{
		if(pedantic)
			yywarning("Redefinition of %s\n\tPrevious definition: %s:%d", def, ppp->filename, ppp->linenumber);
		del_define(def);
	}
	ppp = (pp_entry_t *)xmalloc(sizeof(pp_entry_t));
	ppp->ident = def;
	ppp->type = def_define;
	ppp->subst.text = text;
	ppp->filename = input_name ? xstrdup(input_name) : "<internal or cmdline>";
	ppp->linenumber = input_name ? line_number : 0;
	ppp->next = pp_defines[idx];
	pp_defines[idx] = ppp;
	if(ppp->next)
		ppp->next->prev = ppp;
	if(text)
	{
		/* Strip trailing white space from subst text */
		len = strlen(text);
		while(len && strchr(" \t\r\n", text[len-1]))
		{
			text[--len] = '\0';
		}
		/* Strip leading white space from subst text */
		for(cptr = text; *cptr && strchr(" \t\r", *cptr); cptr++)
		;
		if(text != cptr)
			memmove(text, cptr, strlen(cptr)+1);
	}
	if(debuglevel & DEBUGLEVEL_PPMSG)
		printf("Added define (%s, %d) <%s> to <%s>\n", input_name, line_number, ppp->ident, text ? text : "(null)");

	return ppp;
}

pp_entry_t *add_cmdline_define(char *set)
{
	char *cpy = xstrdup(set);	/* Because gcc passes a R/O string */
	char *cptr = strchr(cpy, '=');
	if(cptr)
		*cptr = '\0';
	return add_define(cpy, xstrdup(cptr ? cptr+1 : ""));
}

pp_entry_t *add_special_define(char *id)
{
	pp_entry_t *ppp = add_define(xstrdup(id), xstrdup(""));
	ppp->type = def_special;
	return ppp;
}

pp_entry_t *add_macro(char *id, marg_t *args[], int nargs, mtext_t *exp)
{
	int idx = pphash(id);
	pp_entry_t *ppp;

	if((ppp = pplookup(id)) != NULL)
	{
		if(pedantic)
			yywarning("Redefinition of %s\n\tPrevious definition: %s:%d", id, ppp->filename, ppp->linenumber);
		del_define(id);
	}
	ppp = (pp_entry_t *)xmalloc(sizeof(pp_entry_t));
	ppp->ident	= id;
	ppp->type	= def_macro;
	ppp->margs	= args;
	ppp->nargs	= nargs;
	ppp->subst.mtext= exp;
	ppp->filename = input_name ? xstrdup(input_name) : "<internal or cmdline>";
	ppp->linenumber = input_name ? line_number : 0;
	ppp->next	= pp_defines[idx];
	pp_defines[idx] = ppp;
	if(ppp->next)
		ppp->next->prev = ppp;

	if(debuglevel & DEBUGLEVEL_PPMSG)
	{
		fprintf(stderr, "Added macro (%s, %d) <%s(%d)> to <", input_name, line_number, ppp->ident, nargs);
		for(; exp; exp = exp->next)
		{
			switch(exp->type)
			{
			case exp_text:
				fprintf(stderr, " \"%s\" ", exp->subst.text);
				break;
			case exp_stringize:
				fprintf(stderr, " #(%d) ", exp->subst.argidx);
				break;
			case exp_concat:
				fprintf(stderr, "##");
				break;
			case exp_subst:
				fprintf(stderr, " <%d> ", exp->subst.argidx);
				break;
			}
		}
		fprintf(stderr, ">\n");
	}
	return ppp;
}


/*
 *-------------------------------------------------------------------------
 * Include management
 *-------------------------------------------------------------------------
 */
#if defined(_Windows) || defined(__MSDOS__)
#define INCLUDESEPARATOR	";"
#else
#define INCLUDESEPARATOR	":"
#endif

static char **includepath;
static int nincludepath = 0;

void add_include_path(char *path)
{
	char *tok;
	char *cpy = xstrdup(path);

	tok = strtok(cpy, INCLUDESEPARATOR);
	while(tok)
	{
		char *dir;
		char *cptr;
		if(strlen(tok) == 0)
			continue;
		dir = xstrdup(tok);
		for(cptr = dir; *cptr; cptr++)
		{
			/* Convert to forward slash */
			if(*cptr == '\\')
				*cptr = '/';
		}
		/* Kill eventual trailing '/' */
		if(*(cptr = dir + strlen(dir)-1) == '/')
			*cptr = '\0';

		/* Add to list */
		nincludepath++;
		includepath = (char **)xrealloc(includepath, nincludepath * sizeof(*includepath));
		includepath[nincludepath-1] = dir;
		tok = strtok(NULL, INCLUDESEPARATOR);
	}
	free(cpy);
}

FILE *open_include(const char *name, int search, char **newpath)
{
	char *cpy = xstrdup(name);
	char *cptr;
	FILE *fp;
	int i;

	for(cptr = cpy; *cptr; cptr++)
	{
		/* kill double backslash */
		if(*cptr == '\\' && *(cptr+1) == '\\')
			memmove(cptr, cptr+1, strlen(cptr));
		/* Convert to forward slash */
		if(*cptr == '\\')
			*cptr = '/';
	}

	if(search)
	{
		/* Search current dir and then -I path */
		fp = fopen(cpy, "rt");
		if(fp)
		{
			if(debuglevel & DEBUGLEVEL_PPMSG)
				printf("Going to include <%s>\n", name);
			if(newpath)
				*newpath = cpy;
			else
				free(cpy);
			return fp;
		}
	}
	/* Search -I path */
	for(i = 0; i < nincludepath; i++)
	{
		char *path;
		path = (char *)xmalloc(strlen(includepath[i]) + strlen(cpy) + 2);
		strcpy(path, includepath[i]);
		strcat(path, "/");
		strcat(path, cpy);
		fp = fopen(path, "rt");
		if(fp && (debuglevel & DEBUGLEVEL_PPMSG))
			printf("Going to include <%s>\n", path);
		if(fp)
		{
			if(newpath)
				*newpath = path;
			else
				free(path);
			free(cpy);
			return fp;
		}
		free(path);
	}
	free(cpy);
	if(newpath)
		*newpath = NULL;
	return NULL;
}

/*
 *-------------------------------------------------------------------------
 * #if, #ifdef, #ifndef, #else, #elif and #endif state management
 *
 * #if state transitions are made on basis of the current TOS and the next
 * required state. The state transitions are required to housekeep because
 * #if:s can be nested. The ignore case is activated to prevent output from
 * within a false clause.
 * Some special cases come from the fact that the #elif cases are not
 * binary, but three-state. The problem is that all other elif-cases must
 * be false when one true one has been found. A second problem is that the
 * #else clause is a final clause. No extra #else:s may follow.
 *
 * The states mean:
 * if_true	Process input to output
 * if_false	Process input but no output
 * if_ignore	Process input but no output
 * if_elif	Process input but no output
 * if_elsefalse	Process input but no output
 * if_elsettrue	Process input to output
 *
 * The possible state-sequences are [state(stack depth)] (rest can be deduced):
 *	TOS		#if 1		#else			#endif
 *	if_true(n)	if_true(n+1)	if_elsefalse(n+1)
 *	if_false(n)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elsetrue(n)	if_true(n+1)	if_elsefalse(n+1)
 *	if_elsefalse(n)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elif(n)	if_ignore(n+1)	if_ignore(n+1)
 *	if_ignore(n)	if_ignore(n+1)	if_ignore(n+1)
 *
 *	TOS		#if 1		#elif 0		#else		#endif
 *	if_true(n)	if_true(n+1)	if_elif(n+1)	if_elif(n+1)
 *	if_false(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elsetrue(n)	if_true(n+1)	if_elif(n+1)	if_elif(n+1)
 *	if_elsefalse(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elif(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_ignore(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *
 *	TOS		#if 0		#elif 1		#else		#endif
 *	if_true(n)	if_false(n+1)	if_true(n+1)	if_elsefalse(n+1)
 *	if_false(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elsetrue(n)	if_false(n+1)	if_true(n+1)	if_elsefalse(n+1)
 *	if_elsefalse(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elif(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_ignore(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *
 *-------------------------------------------------------------------------
 */
static char *if_state_str[] = {
	"if_false",
	"if_true",
	"if_elif",
	"if_elsefalse",
	"if_elsetrue",
	"if_ignore"
};

void push_if(if_state_t s)
{
	if(if_stack_idx >= MAXIFSTACK)
		internal_error(__FILE__, __LINE__, "#if-stack overflow; #{if,ifdef,ifndef} nested too deeply (> %d)", MAXIFSTACK);

	if(debuglevel & DEBUGLEVEL_PPLEX)
		fprintf(stderr, "Push if %s:%d: %s(%d) -> %s(%d)\n", input_name, line_number, if_state_str[if_state()], if_stack_idx, if_state_str[s], if_stack_idx+1);

	if_stack[if_stack_idx++] = s;

	switch(s)
	{
	case if_true:
	case if_elsetrue:
		break;
	case if_false:
	case if_elsefalse:
	case if_elif:
	case if_ignore:
		push_ignore_state();
		break;
	}
}

if_state_t pop_if(void)
{
	if(if_stack_idx <= 0)
		yyerror("#{endif,else,elif} without #{if,ifdef,ifndef} (#if-stack underflow)");

	switch(if_state())
	{
	case if_true:
	case if_elsetrue:
		break;
	case if_false:
	case if_elsefalse:
	case if_elif:
	case if_ignore:
		pop_ignore_state();
		break;
	}

	if(debuglevel & DEBUGLEVEL_PPLEX)
		fprintf(stderr, "Pop if %s:%d: %s(%d) -> %s(%d)\n",
				input_name,
				line_number,
				if_state_str[if_state()],
				if_stack_idx,
				if_state_str[if_stack[if_stack_idx <= 1 ? if_true : if_stack_idx-2]],
				if_stack_idx-1);

	return if_stack[--if_stack_idx];
}

if_state_t if_state(void)
{
	if(!if_stack_idx)
		return if_true;
	else
		return if_stack[if_stack_idx-1];
}


void next_if_state(int i)
{
	switch(if_state())
	{
	case if_true:
	case if_elsetrue:
		push_if(i ? if_true : if_false);
		break;
	case if_false:
	case if_elsefalse:
	case if_elif:
	case if_ignore:
		push_if(if_ignore);
		break;
	default:
		internal_error(__FILE__, __LINE__, "Invalid if_state (%d) in #{if,ifdef,ifndef} directive", (int)if_state());
	}
}

int get_if_depth(void)
{
	return if_stack_idx;
}

