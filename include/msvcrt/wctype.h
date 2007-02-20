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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __WINE_WCTYPE_H
#define __WINE_WCTYPE_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#include <pshpack8.h>

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#endif

/* ASCII char classification table - binary compatible */
#define _UPPER        0x0001  /* C1_UPPER */
#define _LOWER        0x0002  /* C1_LOWER */
#define _DIGIT        0x0004  /* C1_DIGIT */
#define _SPACE        0x0008  /* C1_SPACE */
#define _PUNCT        0x0010  /* C1_PUNCT */
#define _CONTROL      0x0020  /* C1_CNTRL */
#define _BLANK        0x0040  /* C1_BLANK */
#define _HEX          0x0080  /* C1_XDIGIT */
#define _LEADBYTE     0x8000
#define _ALPHA       (0x0100|_UPPER|_LOWER)  /* (C1_ALPHA|_UPPER|_LOWER) */

#ifndef WEOF
#define WEOF        (wint_t)(0xFFFF)
#endif

#ifndef _WCTYPE_T_DEFINED
typedef unsigned short  wint_t;
typedef unsigned short  wctype_t;
#define _WCTYPE_T_DEFINED
#endif

/* FIXME: there's something to do with __p__pctype and __p__pwctype */


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WCTYPE_DEFINED
#define _WCTYPE_DEFINED
int is_wctype(wint_t,wctype_t);
int isleadbyte(int);
int iswalnum(wint_t);
int iswalpha(wint_t);
int iswascii(wint_t);
int iswcntrl(wint_t);
int iswctype(wint_t,wctype_t);
int iswdigit(wint_t);
int iswgraph(wint_t);
int iswlower(wint_t);
int iswprint(wint_t);
int iswpunct(wint_t);
int iswspace(wint_t);
int iswupper(wint_t);
int iswxdigit(wint_t);
wchar_t towlower(wchar_t);
wchar_t towupper(wchar_t);
#endif /* _WCTYPE_DEFINED */

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_WCTYPE_H */
