/*
 * Utility routines
 *
 * Copyright 1998 Bertho A. Stultiens
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <config.h>
#include "wrc.h"
#include "utils.h"

#define WANT_NEAR_INDICATION

extern int line_number;
extern char* yytext;

#ifdef WANT_NEAR_INDICATION
void make_print(char *str)
{
	while(*str)
	{
		if(!isprint(*str))
			*str = ' ';
		str++;
	}
}
#endif

int yyerror(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "Error %s: %d: ", input_name ? input_name : "stdin", line_number);
	vfprintf(stderr, s, ap);
#ifdef WANT_NEAR_INDICATION
	{
		char *cpy = xstrdup(yytext);
		make_print(cpy);
		fprintf(stderr, " near '%s'\n", cpy);
		free(cpy);
	}
#else
	fprintf(stderr, "\n", yytext);
#endif
	va_end(ap);
	exit(1);
	return 1;
}

int yywarning(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "Warning %s:%d: ", input_name ? input_name : "stdin", line_number);
	vfprintf(stderr, s, ap);
#ifdef WANT_NEAR_INDICATION
	{
		char *cpy = xstrdup(yytext);
		make_print(cpy);
		fprintf(stderr, " near '%s'\n", cpy);
		free(cpy);
	}
#else
	fprintf(stderr, "\n", yytext);
#endif
	va_end(ap);
	return 0;
}

void internal_error(const char *file, int line, const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "Internal error (please report) %s %d: ", file, line);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(3);
}

void error(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(2);
}

void warning(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "Warning: ");
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

void chat(const char *s, ...)
{
	if(debuglevel & DEBUGLEVEL_CHAT)
	{
		va_list ap;
		va_start(ap, s);
		fprintf(stderr, "FYI: ");
		vfprintf(stderr, s, ap);
		fprintf(stderr, "\n");
		va_end(ap);
	}
}

char *dup_basename(const char *name, const char *ext)
{
	int namelen;
	int extlen = strlen(ext);
	char *base;

	if(!name)
		name = "wrc.tab";

	namelen = strlen(name);

	/* +4 for later extension and +1 for '\0' */
	base = (char *)xmalloc(namelen +4 +1);
	strcpy(base, name);
	if(!stricmp(name + namelen-extlen, ext))
	{
		base[namelen - extlen] = '\0';
	}
	return base;
}

void *xmalloc(size_t size)
{
    void *res;

    assert(size > 0);
    assert(size < 102400);
    res = malloc(size);
    if(res == NULL)
    {
	error("Virtual memory exhausted.\n");
    }
    memset(res, 0, size);
    return res;
}


void *xrealloc(void *p, size_t size)
{
    void *res;

    assert(size > 0);
    assert(size < 102400);
    res = realloc(p, size);
    if(res == NULL)
    {
	error("Virtual memory exhausted.\n");
    }
    return res;
}

char *xstrdup(const char *str)
{
	char *s = (char *)xmalloc(strlen(str)+1);
	return strcpy(s, str);
}

int string_compare(const string_t *s1, const string_t *s2)
{
	if(s1->type == str_char && s2->type == str_char)
	{
		return stricmp(s1->str.cstr, s2->str.cstr);
	}
	else
	{
		error("Cannot yet compare unicode strings");
	}
	return 0;
}

int wstrlen(const short *s)
{
	int cnt = 0;
	while(*s++)
		cnt++;
	return cnt;
}

short *wstrcpy(short *dst, const short *src)
{
	short *d = dst;
	while(*src)
		*d++ = *src++;
	return dst;
}

int wstricmp(const short *s1, const short *s2)
{
	char *cs1 = dupwstr2cstr(s1);
	char *cs2 = dupwstr2cstr(s2);
	int retval = stricmp(cs1, cs2);
	free(cs1);
	free(cs2);
	warning("Comparing unicode strings -> converting to ascii");
	return retval;;
}

short *dupcstr2wstr(const char *str)
{
	int len = strlen(str) + 1;
	short *ws = (short *)xmalloc(len*2);
	short *wptr;

	wptr = ws;
	/* FIXME: codepage translation */
	while(*str)
		*wptr++ = (short)(*str++ & 0xff);
	*wptr = 0;
	return ws;
}

char *dupwstr2cstr(const short *str)
{
	int len = wstrlen(str) + 1;
	char *cs = (char *)xmalloc(len);
	char *cptr;

	cptr = cs;
	/* FIXME: codepage translation */
	while(*str)
		*cptr++ = (char)*str++;
	*cptr = 0;
	return cs;
}

#ifndef HAVE_STRICMP
int stricmp(const char *s1, const char *s2)
{
	while(*s1 && *s2 && !(toupper(*s1) - toupper(*s2)))
	{
		s1++;
		s2++;
	}
	return *s2 - *s1;
}
#endif

