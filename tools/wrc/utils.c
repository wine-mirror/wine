/*
 * Utility routines
 *
 * Copyright 1998 Bertho A. Stultiens
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "wrc.h"
#include "utils.h"
#include "parser.h"
#include "preproc.h"

/* #define WANT_NEAR_INDICATION */


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

static void generic_msg(const char *s, const char *t, const char *n, va_list ap)
{
	fprintf(stderr, "%s %s: %d, %d: ", t, input_name ? input_name : "stdin", line_number, char_number);
	vfprintf(stderr, s, ap);
#ifdef WANT_NEAR_INDICATION
	{
		char *cpy;
		if(n)
		{
			cpy = xstrdup(n);
			make_print(cpy);
			fprintf(stderr, " near '%s'", cpy);
			free(cpy);
		}
	}
#endif
	fprintf(stderr, "\n");
}


int yyerror(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(s, "Error", yytext, ap);
	va_end(ap);
	exit(1);
	return 1;
}

int yywarning(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(s, "Warning", yytext, ap);
	va_end(ap);
	return 0;
}

int pperror(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(s, "Error", pptext, ap);
	va_end(ap);
	exit(1);
	return 1;
}

int ppwarning(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(s, "Warning", pptext, ap);
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
	char *slash;

	if(!name)
		name = "wrc.tab";

	slash = strrchr(name, '/');
	if (slash)
		name = slash + 1;

	namelen = strlen(name);

	/* +4 for later extension and +1 for '\0' */
	base = (char *)xmalloc(namelen +4 +1);
	strcpy(base, name);
	if(!strcasecmp(name + namelen-extlen, ext))
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
    /*
     * We set it to 0.
     * This is *paramount* because we depend on it
     * just about everywhere in the rest of the code.
     */
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
	char *s;

	assert(str != NULL);
	s = (char *)xmalloc(strlen(str)+1);
	return strcpy(s, str);
}

int string_compare(const string_t *s1, const string_t *s2)
{
	if(s1->type == str_char && s2->type == str_char)
	{
		return strcasecmp(s1->str.cstr, s2->str.cstr);
	}
	else
	{
		internal_error(__FILE__, __LINE__, "Cannot yet compare unicode strings");
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
	int retval = strcasecmp(cs1, cs2);
	free(cs1);
	free(cs2);
	warning("Comparing unicode strings without case -> converting to ascii");
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

/*
 *****************************************************************************
 * Function	: compare_name_id
 * Syntax	: int compare_name_id(name_id_t *n1, name_id_t *n2)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
int compare_name_id(name_id_t *n1, name_id_t *n2)
{
	if(n1->type == name_ord && n2->type == name_ord)
	{
		return n1->name.i_name - n2->name.i_name;
	}
	else if(n1->type == name_str && n2->type == name_str)
	{
		if(n1->name.s_name->type == str_char
		&& n2->name.s_name->type == str_char)
		{
			return strcasecmp(n1->name.s_name->str.cstr, n2->name.s_name->str.cstr);
		}
		else if(n1->name.s_name->type == str_unicode
		&& n2->name.s_name->type == str_unicode)
		{
			return wstricmp(n1->name.s_name->str.wstr, n2->name.s_name->str.wstr);
		}
		else
		{
			internal_error(__FILE__, __LINE__, "Can't yet compare strings of mixed type");
		}
	}
	else if(n1->type == name_ord && n2->type == name_str)
		return 1;
	else if(n1->type == name_str && n2->type == name_ord)
		return -1;
	else
		internal_error(__FILE__, __LINE__, "Comparing name-ids with unknown types (%d, %d)",
				n1->type, n2->type);

	return 0; /* Keep the compiler happy */
}

