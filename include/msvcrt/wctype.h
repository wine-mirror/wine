/*
 * Unicode definitions
 *
 * Copyright 2000 Francois Gouget.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_WCTYPE_H
#define __WINE_WCTYPE_H
#define __WINE_USE_MSVCRT


/* FIXME: winnt.h includes 'ctype.h' which includes 'wctype.h'. So we get 
 * there but WCHAR is not defined.
 */
/* Some systems might have wchar_t, but we really need 16 bit characters */
#ifndef WINE_WCHAR_DEFINED
#ifdef WINE_UNICODE_NATIVE
typedef wchar_t         WCHAR,      *PWCHAR;
#else
typedef unsigned short  WCHAR,      *PWCHAR;
#endif
#define WINE_WCHAR_DEFINED
#endif

#ifdef USE_MSVCRT_PREFIX
#define MSVCRT(x)    MSVCRT_##x
#else
#define MSVCRT(x)    x
#endif


/* ASCII char classification table - binary compatible */
#define _UPPER        C1_UPPER
#define _LOWER        C1_LOWER
#define _DIGIT        C1_DIGIT
#define _SPACE        C1_SPACE
#define _PUNCT        C1_PUNCT
#define _CONTROL      C1_CNTRL
#define _BLANK        C1_BLANK
#define _HEX          C1_XDIGIT
#define _LEADBYTE     0x8000
#define _ALPHA       (C1_ALPHA|_UPPER|_LOWER)

#ifndef USE_MSVCRT_PREFIX
# ifndef WEOF
#  define WEOF        (WCHAR)(0xFFFF)
# endif
#else
# ifndef MSVCRT_WEOF
#  define MSVCRT_WEOF (WCHAR)(0xFFFF)
# endif
#endif /* USE_MSVCRT_PREFIX */

typedef WCHAR MSVCRT(wctype_t);
typedef WCHAR MSVCRT(wint_t);

/* FIXME: there's something to do with __p__pctype and __p__pwctype */


#ifdef __cplusplus
extern "C" {
#endif

int MSVCRT(is_wctype)(MSVCRT(wint_t),MSVCRT(wctype_t));
int MSVCRT(isleadbyte)(int);
int MSVCRT(iswalnum)(MSVCRT(wint_t));
int MSVCRT(iswalpha)(MSVCRT(wint_t));
int MSVCRT(iswascii)(MSVCRT(wint_t));
int MSVCRT(iswcntrl)(MSVCRT(wint_t));
int MSVCRT(iswctype)(MSVCRT(wint_t),MSVCRT(wctype_t));
int MSVCRT(iswdigit)(MSVCRT(wint_t));
int MSVCRT(iswgraph)(MSVCRT(wint_t));
int MSVCRT(iswlower)(MSVCRT(wint_t));
int MSVCRT(iswprint)(MSVCRT(wint_t));
int MSVCRT(iswpunct)(MSVCRT(wint_t));
int MSVCRT(iswspace)(MSVCRT(wint_t));
int MSVCRT(iswupper)(MSVCRT(wint_t));
int MSVCRT(iswxdigit)(MSVCRT(wint_t));
WCHAR MSVCRT(towlower)(WCHAR);
WCHAR MSVCRT(towupper)(WCHAR);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_WCTYPE_H */
