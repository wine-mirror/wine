/*
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <config.h>
#include "wrc.h"
#include "utils.h"
#include "preproc.h"
#include "parser.h"


extern void set_pp_ignore(int);	/* From parser.l */

static char *current_define;

#define HASHKEY		2039
static struct pp_entry *pp_defines[HASHKEY];

#define MAXIFSTACK	64
static struct if_state ifstack[MAXIFSTACK];
static int ifstackidx = 0;

#if 0
void pp_status(void)
{
	int i;
	int sum;
	int total = 0;
	struct pp_entry *ppp;

	printf("Defines statistics:\n");
	for(i = 0; i < HASHKEY; i++)
	{
		sum = 0;
		for(ppp = pp_defines[i]; ppp; ppp = ppp->next)
			sum++;
		total += sum;
		printf("%4d, %3d\n", i, sum);
	}
	printf("Total defines: %d\n", total);
}
#pragma exit pp_status
#endif

/* Don't comment on the hash, its primitive but functional... */
int pp_hash(char *str)
{
	int sum = 0;
	while(*str)
		sum += *str++;
	return sum % HASHKEY;
}

struct pp_entry *pp_lookup(char *ident)
{
	int index = pp_hash(ident);
	struct pp_entry *ppp;
	for(ppp = pp_defines[index]; ppp; ppp = ppp->next)
	{
		if(!strcmp(ident, ppp->ident))
			return ppp;
	}
	return NULL;
}

void set_define(char *name)
{
	current_define = xstrdup(name);
}

void del_define(char *name)
{
	int index;
	struct pp_entry *ppp;

	if((ppp = pp_lookup(name)) == NULL)
	{
		if(pedantic)
			yywarning("%s was not defined", name);
		return;
	}

	index = pp_hash(name);
	if(pp_defines[index] == ppp)
	{
		pp_defines[index] = ppp->next;
		if(pp_defines[index])
			pp_defines[index]->prev = NULL;
	}
	else
	{
		ppp->prev->next = ppp->next;
		if(ppp->next)
			ppp->next->prev = ppp->prev;
	}
	free(ppp);
}

void add_define(char *text)
{
	int len;
	char *cptr;
	int index = pp_hash(current_define);
	struct pp_entry *ppp;
	if(pp_lookup(current_define) != NULL)
	{
		if(pedantic)
			yywarning("Redefinition of %s", current_define);
		del_define(current_define);
	}
	ppp = (struct pp_entry *)xmalloc(sizeof(struct pp_entry));
	ppp->ident = current_define;
	ppp->subst = xstrdup(text);
	ppp->next = pp_defines[index];
	pp_defines[index] = ppp;
	if(ppp->next)
		ppp->next->prev = ppp;
	/* Strip trailing white space from subst text */
	len = strlen(ppp->subst);
	while(len && strchr(" \t\r\n", ppp->subst[len-1]))
	{
		ppp->subst[--len] = '\0';
	}
	/* Strip leading white space from subst text */
	for(cptr = ppp->subst; *cptr && strchr(" \t\r", *cptr); cptr++)
		;
	if(ppp->subst != cptr)
		memmove(ppp->subst, cptr, strlen(cptr)+1);
	if(yydebug)
		printf("Added (%s, %d) <%s> to <%s>\n", input_name, line_number, ppp->ident, ppp->subst);
}

void add_cmdline_define(char *set)
{
	char *cpy = xstrdup(set);	/* Because gcc passes a R/O string */
	char *cptr = strchr(cpy, '=');
	if(cptr)
		*cptr = '\0';
	set_define(cpy);
	add_define(cptr ? cptr+1 : "");
	free(cpy);
}

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

FILE *open_include(const char *name, int search)
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
		fp = fopen(name, "rt");
		if(fp)
		{
			if(yydebug)
				printf("Going to include <%s>\n", name);
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
		if(fp && yydebug)
			printf("Going to include <%s>\n", path);
		free(path);
		if(fp)
		{
			free(cpy);
			return fp;
		}

	}
	free(cpy);
	return NULL;
}

void push_if(int truecase, int wastrue, int nevertrue)
{
	if(ifstackidx >= MAXIFSTACK-1)
		internal_error(__FILE__, __LINE__, "#if stack overflow");
	ifstack[ifstackidx].current = truecase && !wastrue;
	ifstack[ifstackidx].hasbeentrue = wastrue;
	ifstack[ifstackidx].nevertrue = nevertrue;
	if(nevertrue || !(truecase && !wastrue))
		set_pp_ignore(1);
	if(yydebug)
		printf("push_if: %d %d %d (%d %d %d)\n",
			truecase,
			wastrue,
			nevertrue,
			ifstack[ifstackidx].current,
			ifstack[ifstackidx].hasbeentrue,
			ifstack[ifstackidx].nevertrue);
	ifstackidx++;
}

int pop_if(void)
{
	if(ifstackidx <= 0)
		yyerror("#endif without #if|#ifdef|#ifndef (#if stack underflow)");
	ifstackidx--;
	if(yydebug)
		printf("pop_if: %d %d %d\n",
			ifstack[ifstackidx].current,
			ifstack[ifstackidx].hasbeentrue,
			ifstack[ifstackidx].nevertrue);
	if(ifstack[ifstackidx].nevertrue || !ifstack[ifstackidx].current)
		set_pp_ignore(0);
	return ifstack[ifstackidx].hasbeentrue || ifstack[ifstackidx].current;
}

int isnevertrue_if(void)
{
	return ifstackidx > 0 && ifstack[ifstackidx-1].nevertrue;
}

