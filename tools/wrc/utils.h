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
char *dupwstr2cstr(const short *str);
short *dupcstr2wstr(const char *str);
int compare_name_id(name_id_t *n1, name_id_t *n2);
string_t *convert_string(const string_t *str, enum str_e type);
void set_language(unsigned short lang, unsigned short sublang);

#endif
