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

void * __cdecl CRTDLL_malloc( DWORD size );
void   __cdecl CRTDLL_free( void *ptr );

LPSTR  __cdecl CRTDLL__mbsinc( LPCSTR str );
INT    __cdecl CRTDLL__mbslen( LPCSTR str );
LPWSTR __cdecl CRTDLL__wcsdup( LPCWSTR str );
INT    __cdecl CRTDLL__wcsicmp( LPCWSTR str1, LPCWSTR str2 );
INT    __cdecl CRTDLL__wcsicoll( LPCWSTR str1, LPCWSTR str2 );
LPWSTR __cdecl CRTDLL__wcslwr( LPWSTR str );
INT    __cdecl CRTDLL__wcsnicmp( LPCWSTR str1, LPCWSTR str2, INT n );
LPWSTR __cdecl CRTDLL__wcsnset( LPWSTR str, WCHAR c, INT n );
LPWSTR __cdecl CRTDLL__wcsrev( LPWSTR str );
LPWSTR __cdecl CRTDLL__wcsset( LPWSTR str, WCHAR c );
LPWSTR __cdecl CRTDLL__wcsupr( LPWSTR str );
INT    __cdecl CRTDLL_mbstowcs( LPWSTR dst, LPCSTR src, INT n );
INT    __cdecl CRTDLL_mbtowc( WCHAR *dst, LPCSTR str, INT n );
WCHAR  __cdecl CRTDLL_towlower( WCHAR ch );
WCHAR  __cdecl CRTDLL_towupper( WCHAR ch );
LPWSTR __cdecl CRTDLL_wcscat( LPWSTR dst, LPCWSTR src );
LPWSTR __cdecl CRTDLL_wcschr( LPCWSTR str, WCHAR ch );
INT    __cdecl CRTDLL_wcscmp( LPCWSTR str1, LPCWSTR str2 );
DWORD  __cdecl CRTDLL_wcscoll( LPCWSTR str1, LPCWSTR str2 );
LPWSTR __cdecl CRTDLL_wcscpy( LPWSTR dst, LPCWSTR src );
INT    __cdecl CRTDLL_wcscspn( LPCWSTR str, LPCWSTR reject );
INT    __cdecl CRTDLL_wcslen( LPCWSTR str );
LPWSTR __cdecl CRTDLL_wcsncat( LPWSTR s1, LPCWSTR s2, INT n );
INT    __cdecl CRTDLL_wcsncmp( LPCWSTR str1, LPCWSTR str2, INT n );
LPWSTR __cdecl CRTDLL_wcsncpy( LPWSTR s1, LPCWSTR s2, INT n );
LPWSTR __cdecl CRTDLL_wcspbrk( LPCWSTR str, LPCWSTR accept );
LPWSTR __cdecl CRTDLL_wcsrchr( LPWSTR str, WCHAR ch );
INT    __cdecl CRTDLL_wcsspn( LPCWSTR str, LPCWSTR accept );
LPWSTR __cdecl CRTDLL_wcsstr( LPCWSTR str, LPCWSTR sub );
LPWSTR __cdecl CRTDLL_wcstok( LPWSTR str, LPCWSTR delim );
INT    __cdecl CRTDLL_wcstombs( LPSTR dst, LPCWSTR src, INT n );
INT    __cdecl CRTDLL_wctomb( LPSTR dst, WCHAR ch );

#ifdef notyet
#define _mbsinc      CRTDLL__mbsinc
#define _mbslen      CRTDLL__mbslen
#define _wcsdup      CRTDLL__wcsdup
#define _wcsicmp     CRTDLL__wcsicmp
#define _wcsicoll    CRTDLL__wcsicoll
#define _wcslwr      CRTDLL__wcslwr
#define _wcsnicmp    CRTDLL__wcsnicmp
#define _wcsnset     CRTDLL__wcsnset
#define _wcsrev      CRTDLL__wcsrev
#define _wcsset      CRTDLL__wcsset
#define _wcsupr      CRTDLL__wcsupr
#define mbstowcs     CRTDLL_mbstowcs
#define mbtowc       CRTDLL_mbtowc
#define towlower     CRTDLL_towlower
#define towupper     CRTDLL_towupper
#define wcscat       CRTDLL_wcscat
#define wcschr       CRTDLL_wcschr
#define wcscmp       CRTDLL_wcscmp
#define wcscoll      CRTDLL_wcscoll
#define wcscpy       CRTDLL_wcscpy
#define wcscspn      CRTDLL_wcscspn
#define wcslen       CRTDLL_wcslen
#define wcsncat      CRTDLL_wcsncat
#define wcsncmp      CRTDLL_wcsncmp
#define wcsncpy      CRTDLL_wcsncpy
#define wcspbrk      CRTDLL_wcspbrk
#define wcsrchr      CRTDLL_wcsrchr
#define wcsspn       CRTDLL_wcsspn
#define wcsstr       CRTDLL_wcsstr
#define wcstok       CRTDLL_wcstok
#define wcstombs     CRTDLL_wcstombs
#define wctomb       CRTDLL_wctomb
#endif

#endif /* __WINE_CRTDLL_H */
