/*
 * Character type definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_CTYPE_H
#define __WINE_CTYPE_H
#define __WINE_USE_MSVCRT

#include "msvcrt/wctype.h"


#ifdef __cplusplus
extern "C" {
#endif

int MSVCRT(__isascii)(int);
int MSVCRT(__iscsym)(int);
int MSVCRT(__iscsymf)(int);
int MSVCRT(__toascii)(int);
int MSVCRT(_isctype)(int,int);
int MSVCRT(_tolower)(int);
int MSVCRT(_toupper)(int);
int MSVCRT(isalnum)(int);
int MSVCRT(isalpha)(int);
int MSVCRT(iscntrl)(int);
int MSVCRT(isdigit)(int);
int MSVCRT(isgraph)(int);
int MSVCRT(islower)(int);
int MSVCRT(isprint)(int);
int MSVCRT(ispunct)(int);
int MSVCRT(isspace)(int);
int MSVCRT(isupper)(int);
int MSVCRT(isxdigit)(int);
int MSVCRT(tolower)(int);
int MSVCRT(toupper)(int);

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define isascii __isascii
#define iscsym  __iscsym
#define iscsymf __iscsymf
#define toascii __toascii
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_CTYPE_H */
