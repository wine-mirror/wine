/*
 * Unicode definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_WCHAR_H
#define __WINE_WCHAR_H
#define __WINE_USE_MSVCRT

#include "msvcrt/io.h"
#include "msvcrt/locale.h"
#include "msvcrt/process.h"
#include "msvcrt/stdio.h"
#include "msvcrt/stdlib.h"
#include "msvcrt/string.h"
#include "msvcrt/sys/stat.h"
#include "msvcrt/sys/types.h"
#include "msvcrt/time.h"
#include "msvcrt/wctype.h"


#define WCHAR_MIN 0
#define WCHAR_MAX ((WCHAR)-1)

typedef int MSVCRT(mbstate_t);

#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif


#ifdef __cplusplus
extern "C" {
#endif

WCHAR       btowc(int);
MSVCRT(size_t) mbrlen(const char *,MSVCRT(size_t),MSVCRT(mbstate_t)*);
MSVCRT(size_t) mbrtowc(WCHAR*,const char*,MSVCRT(size_t),MSVCRT(mbstate_t)*);
MSVCRT(size_t) mbsrtowcs(WCHAR*,const char**,MSVCRT(size_t),MSVCRT(mbstate_t)*);
MSVCRT(size_t) wcrtomb(char*,WCHAR,MSVCRT(mbstate_t)*);
MSVCRT(size_t) wcsrtombs(char*,const WCHAR**,MSVCRT(size_t),MSVCRT(mbstate_t)*);
int         wctob(MSVCRT(wint_t));

#ifdef __cplusplus
}
#endif

#endif /* __WINE_WCHAR_H */
