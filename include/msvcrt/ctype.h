/*
 * Character type definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_CTYPE_H
#define __WINE_CTYPE_H

#include <corecrt_wctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEOF
#define WEOF        (wint_t)(0xFFFF)
#endif

int __cdecl __isascii(int);
int __cdecl __iscsym(int);
int __cdecl __iscsymf(int);
int __cdecl __toascii(int);
int __cdecl _isblank_l(int,_locale_t);
int __cdecl _isctype(int,int);
int __cdecl _tolower(int);
int __cdecl _tolower_l(int,_locale_t);
int __cdecl _toupper(int);
int __cdecl _toupper_l(int,_locale_t);
int __cdecl isalnum(int);
int __cdecl isalpha(int);
int __cdecl isblank(int);
int __cdecl iscntrl(int);
int __cdecl isdigit(int);
int __cdecl isgraph(int);
int __cdecl islower(int);
int __cdecl isprint(int);
int __cdecl ispunct(int);
int __cdecl isspace(int);
int __cdecl isupper(int);
int __cdecl isxdigit(int);
int __cdecl tolower(int);
int __cdecl toupper(int);

#ifdef __cplusplus
}
#endif


static inline int isascii(int c) { return __isascii(c); }
static inline int iscsym(int c) { return __iscsym(c); }
static inline int iscsymf(int c) { return __iscsymf(c); }
static inline int toascii(int c) { return __toascii(c); }

#endif /* __WINE_CTYPE_H */
