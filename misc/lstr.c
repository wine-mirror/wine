/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 * Copyright 1996 Marcus Meissner
 */

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_WCTYPE_H
# include <wctype.h>
#else
# define iswalnum(c) isalnum(c)
# define iswalpha(c) isalpha(c)
# define iswupper(c) isupper(c)
# define iswlower(c) islower(c)
#endif  /* HAVE_WCTYPE_H */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "winnls.h"
#include "heap.h"
#include "ldt.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(resource);

extern const WORD OLE2NLS_CT_CType3_LUT[]; /* FIXME: does not belong here */

/***********************************************************************
 *           CharLowerA   (USER32.25)
 * FIXME: handle current locale
 */
LPSTR WINAPI CharLowerA(LPSTR x)
{
    LPSTR	s;

    if (HIWORD(x))
    {
        s=x;
        while (*s)
        {
            *s=tolower(*s);
            s++;
        }
        return x;
    }
    else return (LPSTR)tolower((char)(int)x);
}

/***********************************************************************
 *           CharLowerBuffA   (USER32.26)
 * FIXME: handle current locale
 */
DWORD WINAPI CharLowerBuffA(LPSTR x,DWORD buflen)
{
    DWORD done=0;

    if (!x) return 0; /* YES */
    while (*x && (buflen--))
    {
        *x=tolower(*x);
        x++;
        done++;
    }
    return done;
}

/***********************************************************************
 *           CharLowerBuffW   (USER32.27)
 */
DWORD WINAPI CharLowerBuffW(LPWSTR x,DWORD buflen)
{
    DWORD done=0;

    if (!x) return 0; /* YES */
    while (*x && (buflen--))
    {
        *x=tolowerW(*x);
        x++;
        done++;
    }
    return done;
}

/***********************************************************************
 *           CharLowerW   (USER32.28)
 * FIXME: handle current locale
 */
LPWSTR WINAPI CharLowerW(LPWSTR x)
{
    if (HIWORD(x))
    {
        LPWSTR s = x;
        while (*s)
        {
            *s = tolowerW(*s);
            s++;
        }
        return x;
    }
    else return (LPWSTR)((UINT)tolowerW(LOWORD(x)));
}

/***********************************************************************
 *           CharUpperA   (USER32.41)
 * FIXME: handle current locale
 */
LPSTR WINAPI CharUpperA(LPSTR x)
{
    if (HIWORD(x))
    {
        LPSTR s = x;
        while (*s)
        {
            *s=toupper(*s);
            s++;
        }
        return x;
    }
    return (LPSTR)toupper((char)(int)x);
}

/***********************************************************************
 *           CharUpperBuffA   (USER32.42)
 * FIXME: handle current locale
 */
DWORD WINAPI CharUpperBuffA( LPSTR str, DWORD len )
{
    DWORD ret = len;
    if (!str) return 0; /* YES */
    for (; len; len--, str++) *str = toupper(*str);
    return ret;
}

/***********************************************************************
 *           CharUpperBuffW   (USER32.43)
 * FIXME: handle current locale
 */
DWORD WINAPI CharUpperBuffW( LPWSTR str, DWORD len )
{
    DWORD ret = len;
    if (!str) return 0; /* YES */
    for (; len; len--, str++) *str = toupperW(*str);
    return ret;
}

/***********************************************************************
 *           CharUpperW   (USER32.44)
 * FIXME: handle current locale
 */
LPWSTR WINAPI CharUpperW(LPWSTR x)
{
    if (HIWORD(x))
    {
        LPWSTR s = x;
        while (*s)
        {
            *s = toupperW(*s);
            s++;
        }
        return x;
    }
    else return (LPWSTR)((UINT)toupperW(LOWORD(x)));
}

/***********************************************************************
 *           IsCharAlphaA   (USER.433) (USER32.331)
 * FIXME: handle current locale
 */
BOOL WINAPI IsCharAlphaA(CHAR x)
{
    return (OLE2NLS_CT_CType3_LUT[(unsigned char)x] & C3_ALPHA);
}

/***********************************************************************
 *           IsCharAlphaNumericA   (USER.434) (USER32.332)
 * FIXME: handle current locale
 */
BOOL WINAPI IsCharAlphaNumericA(CHAR x)
{
    return IsCharAlphaA(x) || isdigit(x) ;
}

/***********************************************************************
 *           IsCharAlphaNumericW   (USER32.333)
 * FIXME: handle current locale
 */
BOOL WINAPI IsCharAlphaNumericW(WCHAR x)
{
    return iswalnum(x);
}

/***********************************************************************
 *           IsCharAlphaW   (USER32.334)
 * FIXME: handle current locale
 */
BOOL WINAPI IsCharAlphaW(WCHAR x)
{
    return iswalpha(x);
}

/***********************************************************************
 *           IsCharLowerA   (USER.436) (USER32.335)
 * FIXME: handle current locale
 */
BOOL WINAPI IsCharLowerA(CHAR x)
{
    return islower(x);
}

/***********************************************************************
 *           IsCharLowerW   (USER32.336)
 * FIXME: handle current locale
 */
BOOL WINAPI IsCharLowerW(WCHAR x)
{
    return iswlower(x);
}

/***********************************************************************
 *           IsCharUpperA   (USER.435) (USER32.337)
 * FIXME: handle current locale
 */
BOOL WINAPI IsCharUpperA(CHAR x)
{
    return isupper(x);
}

/***********************************************************************
 *           IsCharUpperW   (USER32.338)
 * FIXME: handle current locale
 */
BOOL WINAPI IsCharUpperW(WCHAR x)
{
    return iswupper(x);
}
