/*
 * Utility routines' prototypes etc.
 *
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 */

#ifndef __WRC_UTILS_H
#define __WRC_UTILS_H

#ifndef __WRC_WRCTYPES_H
#include "wrctypes.h"
#endif

#include <stddef.h>	/* size_t */

void *xmalloc(size_t);
void *xrealloc(void *, size_t);
char *xstrdup(const char *str);

int pperror(const char *s, ...) __attribute__((format (printf, 1, 2)));
int ppwarning(const char *s, ...) __attribute__((format (printf, 1, 2)));
int yyerror(const char *s, ...) __attribute__((format (printf, 1, 2)));
int yywarning(const char *s, ...) __attribute__((format (printf, 1, 2)));
void internal_error(const char *file, int line, const char *s, ...) __attribute__((format (printf, 3, 4)));
void error(const char *s, ...) __attribute__((format (printf, 1, 2)));
void warning(const char *s, ...) __attribute__((format (printf, 1, 2)));
void chat(const char *s, ...) __attribute__((format (printf, 1, 2)));

char *dup_basename(const char *name, const char *ext);
int string_compare(const string_t *s1, const string_t *s2);
int wstrlen(const short *s);
short *wstrcpy(short *dst, const short *src);
int wstricmp(const short *s1, const short *s2);
char *dupwstr2cstr(const short *str);
short *dupcstr2wstr(const char *str);

#endif
