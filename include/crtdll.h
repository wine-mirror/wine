#ifndef __WINE_CRTDLL_H
#define __WINE_CRTDLL_H

#include "windef.h"

#define CRTDLL_LC_ALL		0
#define CRTDLL_LC_COLLATE	1
#define CRTDLL_LC_CTYPE		2
#define CRTDLL_LC_MONETARY	3
#define CRTDLL_LC_NUMERIC	4
#define CRTDLL_LC_TIME		5
#define CRTDLL_LC_MIN		LC_ALL
#define CRTDLL_LC_MAX		LC_TIME

/* ctype defines */
#define CRTDLL_UPPER		0x1
#define CRTDLL_LOWER		0x2
#define CRTDLL_DIGIT		0x4
#define CRTDLL_SPACE		0x8
#define CRTDLL_PUNCT		0x10
#define CRTDLL_CONTROL		0x20
#define CRTDLL_BLANK		0x40
#define CRTDLL_HEX		0x80
#define CRTDLL_LEADBYTE		0x8000
#define CRTDLL_ALPHA		(0x0100|CRTDLL_UPPER|CRTDLL_LOWER)

/* function prototypes used in crtdll.c */
extern int LastErrorToErrno(DWORD);

void   __cdecl *CRTDLL_malloc( DWORD size );
void   __cdecl CRTDLL_free( void *ptr );

LPSTR  __cdecl CRTDLL__mbsinc( LPCSTR str );
INT    __cdecl CRTDLL__mbslen( LPCSTR str );
LPWSTR __cdecl CRTDLL__wcsdup( LPCWSTR str );
INT    __cdecl CRTDLL__wcsicoll( LPCWSTR str1, LPCWSTR str2 );
LPWSTR __cdecl CRTDLL__wcsnset( LPWSTR str, WCHAR c, INT n );
LPWSTR __cdecl CRTDLL__wcsrev( LPWSTR str );
LPWSTR __cdecl CRTDLL__wcsset( LPWSTR str, WCHAR c );
INT    __cdecl CRTDLL_iswalnum( WCHAR wc );
INT    __cdecl CRTDLL_iswalpha( WCHAR wc );
INT    __cdecl CRTDLL_iswcntrl( WCHAR wc );
INT    __cdecl CRTDLL_iswdigit( WCHAR wc );
INT    __cdecl CRTDLL_iswgraph( WCHAR wc );
INT    __cdecl CRTDLL_iswlower( WCHAR wc );
INT    __cdecl CRTDLL_iswprint( WCHAR wc );
INT    __cdecl CRTDLL_iswpunct( WCHAR wc );
INT    __cdecl CRTDLL_iswspace( WCHAR wc );
INT    __cdecl CRTDLL_iswupper( WCHAR wc );
INT    __cdecl CRTDLL_iswxdigit( WCHAR wc );
INT    __cdecl CRTDLL_iswctype( WCHAR wc, WCHAR wct );
INT    __cdecl CRTDLL_mbstowcs( LPWSTR dst, LPCSTR src, INT n );
INT    __cdecl CRTDLL_mbtowc( WCHAR *dst, LPCSTR str, INT n );
DWORD  __cdecl CRTDLL_wcscoll( LPCWSTR str1, LPCWSTR str2 );
LPWSTR __cdecl CRTDLL_wcspbrk( LPCWSTR str, LPCWSTR accept );
INT    __cdecl CRTDLL_wctomb( LPSTR dst, WCHAR ch );

#ifdef notyet
#define _mbsinc      CRTDLL__mbsinc
#define _mbslen      CRTDLL__mbslen
#define _wcsdup      CRTDLL__wcsdup
#define _wcsicoll    CRTDLL__wcsicoll
#define _wcsnset     CRTDLL__wcsnset
#define _wcsrev      CRTDLL__wcsrev
#define _wcsset      CRTDLL__wcsset
#define iswalnum     CRTDLL_iswalnum
#define iswalpha     CRTDLL_iswalpha
#define iswcntrl     CRTDLL_iswcntrl
#define iswdigit     CRTDLL_iswdigit
#define iswgraph     CRTDLL_iswgraph
#define iswlower     CRTDLL_iswlower
#define iswprint     CRTDLL_iswprint
#define iswpunct     CRTDLL_iswpunct
#define iswspace     CRTDLL_iswspace
#define iswupper     CRTDLL_iswupper
#define iswxdigit    CRTDLL_iswxdigit
#define mbtowc       CRTDLL_mbtowc
#define wcscoll      CRTDLL_wcscoll
#define wctomb       CRTDLL_wctomb
#endif

#endif /* __WINE_CRTDLL_H */
