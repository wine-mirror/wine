/*
 * NTDLL wide-char functions
 *
 * Copyright 2000 Alexandre Julliard
 */

#include "config.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_WCTYPE_H
#include <wctype.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ntdll);


/*********************************************************************
 *           NTDLL__wcsicmp    (NTDLL)
 */
INT __cdecl NTDLL__wcsicmp( LPCWSTR str1, LPCWSTR str2 )
{
    return strcmpiW( str1, str2 );
}


/*********************************************************************
 *           NTDLL__wcslwr    (NTDLL)
 */
LPWSTR __cdecl NTDLL__wcslwr( LPWSTR str )
{
    return strlwrW( str );
}


/*********************************************************************
 *           NTDLL__wcsnicmp    (NTDLL)
 */
INT __cdecl NTDLL__wcsnicmp( LPCWSTR str1, LPCWSTR str2, INT n )
{
    return strncmpiW( str1, str2, n );
}


/*********************************************************************
 *           NTDLL__wcsupr    (NTDLL)
 */
LPWSTR __cdecl NTDLL__wcsupr( LPWSTR str )
{
    return struprW( str );
}


/*********************************************************************
 *           NTDLL_towlower    (NTDLL)
 */
WCHAR __cdecl NTDLL_towlower( WCHAR ch )
{
    return tolowerW(ch);
}


/*********************************************************************
 *           NTDLL_towupper    (NTDLL)
 */
WCHAR __cdecl NTDLL_towupper( WCHAR ch )
{
    return toupperW(ch);
}


/***********************************************************************
 *           NTDLL_wcscat    (NTDLL)
 */
LPWSTR __cdecl NTDLL_wcscat( LPWSTR dst, LPCWSTR src )
{
    return strcatW( dst, src );
}


/*********************************************************************
 *           NTDLL_wcschr    (NTDLL)
 */
LPWSTR __cdecl NTDLL_wcschr( LPCWSTR str, WCHAR ch )
{
    return strchrW( str, ch );
}


/*********************************************************************
 *           NTDLL_wcscmp    (NTDLL)
 */
INT __cdecl NTDLL_wcscmp( LPCWSTR str1, LPCWSTR str2 )
{
    return strcmpW( str1, str2 );
}


/***********************************************************************
 *           NTDLL_wcscpy    (NTDLL)
 */
LPWSTR __cdecl NTDLL_wcscpy( LPWSTR dst, LPCWSTR src )
{
    return strcpyW( dst, src );
}


/*********************************************************************
 *           NTDLL_wcscspn    (NTDLL)
 */
INT __cdecl NTDLL_wcscspn( LPCWSTR str, LPCWSTR reject )
{
    LPCWSTR start = str;
    while (*str)
    {
        LPCWSTR p = reject;
        while (*p && (*p != *str)) p++;
        if (*p) break;
        str++;
    }
    return str - start;
}


/***********************************************************************
 *           NTDLL_wcslen    (NTDLL)
 */
INT __cdecl NTDLL_wcslen( LPCWSTR str )
{
    return strlenW( str );
}


/*********************************************************************
 *           NTDLL_wcsncat    (NTDLL)
 */
LPWSTR __cdecl NTDLL_wcsncat( LPWSTR s1, LPCWSTR s2, INT n )
{
    LPWSTR ret = s1;
    while (*s1) s1++;
    while (n-- > 0) if (!(*s1++ = *s2++)) return ret;
    *s1 = 0;
    return ret;
}


/*********************************************************************
 *           NTDLL_wcsncmp    (NTDLL)
 */
INT __cdecl NTDLL_wcsncmp( LPCWSTR str1, LPCWSTR str2, INT n )
{
    return strncmpW( str1, str2, n );
}


/*********************************************************************
 *           NTDLL_wcsncpy    (NTDLL)
 */
LPWSTR __cdecl NTDLL_wcsncpy( LPWSTR s1, LPCWSTR s2, INT n )
{
    LPWSTR ret = s1;
    while (n-- > 0) if (!(*s1++ = *s2++)) break;
    while (n-- > 0) *s1++ = 0;
    return ret;
}


/*********************************************************************
 *           NTDLL_wcspbrk    (NTDLL)
 */
LPWSTR __cdecl NTDLL_wcspbrk( LPCWSTR str, LPCWSTR accept )
{
    LPCWSTR p;
    while (*str)
    {
        for (p = accept; *p; p++) if (*p == *str) return (LPWSTR)str;
        str++;
    }
    return NULL;
}


/*********************************************************************
 *           NTDLL_wcsrchr    (NTDLL)
 */
LPWSTR __cdecl NTDLL_wcsrchr( LPWSTR str, WCHAR ch )
{
    LPWSTR last = NULL;
    while (*str)
    {
        if (*str == ch) last = str;
        str++;
    }
    return last;
}


/*********************************************************************
 *           NTDLL_wcsspn    (NTDLL)
 */
INT __cdecl NTDLL_wcsspn( LPCWSTR str, LPCWSTR accept )
{
    LPCWSTR start = str;
    while (*str)
    {
        LPCWSTR p = accept;
        while (*p && (*p != *str)) p++;
        if (!*p) break;
        str++;
    }
    return str - start;
}


/*********************************************************************
 *           NTDLL_wcsstr    (NTDLL)
 */
LPWSTR __cdecl NTDLL_wcsstr( LPCWSTR str, LPCWSTR sub )
{
    return strstrW( str, sub );
}


/*********************************************************************
 *           NTDLL_wcstok    (NTDLL)
 */
LPWSTR __cdecl NTDLL_wcstok( LPWSTR str, LPCWSTR delim )
{
    static LPWSTR next = NULL;
    LPWSTR ret;

    if (!str)
        if (!(str = next)) return NULL;

    while (*str && NTDLL_wcschr( delim, *str )) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !NTDLL_wcschr( delim, *str )) str++;
    if (*str) *str++ = 0;
    next = str;
    return ret;
}


/*********************************************************************
 *           NTDLL_wcstombs    (NTDLL)
 */
INT __cdecl NTDLL_wcstombs( LPSTR dst, LPCWSTR src, INT n )
{
    INT ret;
    if (n <= 0) return 0;
    ret = WideCharToMultiByte( CP_ACP, 0, src, -1, dst, dst ? n : 0, NULL, NULL );
    if (!ret) return n;  /* overflow */
    return ret - 1;  /* do not count terminating NULL */
}


/*********************************************************************
 *           NTDLL_mbstowcs    (NTDLL)
 */
INT __cdecl NTDLL_mbstowcs( LPWSTR dst, LPCSTR src, INT n )
{
    INT ret;
    if (n <= 0) return 0;
    ret = MultiByteToWideChar( CP_ACP, 0, src, -1, dst, dst ? n : 0 );
    if (!ret) return n;  /* overflow */
    return ret - 1;  /* do not count terminating NULL */
}


/*********************************************************************
 *                  wcstol  (NTDLL)
 * Like strtol, but for wide character strings.
 */
INT __cdecl NTDLL_wcstol(LPWSTR s,LPWSTR *end,INT base)
{
    LPSTR sA = HEAP_strdupWtoA(GetProcessHeap(),0,s),endA;
    INT	ret = strtol(sA,&endA,base);

    HeapFree(GetProcessHeap(),0,sA);
    if (end) *end = s+(endA-sA); /* pointer magic checked. */
    return ret;
}


/*********************************************************************
 *           NTDLL_iswctype    (NTDLL)
 */
INT __cdecl NTDLL_iswctype( WCHAR wc, WCHAR wct )
{
    INT res = 0;

#ifdef HAVE_WCTYPE_H
#undef iswupper
#undef iswlower
#undef iswdigit
#undef iswspace
#undef iswpunct
#undef iswcntrl
#undef iswxdigit
#undef iswalpha
    if (wct & 0x0001) res |= iswupper(wc);
    if (wct & 0x0002) res |= iswlower(wc);
    if (wct & 0x0004) res |= iswdigit(wc);
    if (wct & 0x0008) res |= iswspace(wc);
    if (wct & 0x0010) res |= iswpunct(wc);
    if (wct & 0x0020) res |= iswcntrl(wc);
    if (wct & 0x0080) res |= iswxdigit(wc);
    if (wct & 0x0100) res |= iswalpha(wc);
#else
    if (wct & 0x0001) res |= isupper(LOBYTE(wc));
    if (wct & 0x0002) res |= islower(LOBYTE(wc));
    if (wct & 0x0004) res |= isdigit(LOBYTE(wc));
    if (wct & 0x0008) res |= isspace(LOBYTE(wc));
    if (wct & 0x0010) res |= ispunct(LOBYTE(wc));
    if (wct & 0x0020) res |= iscntrl(LOBYTE(wc));
    if (wct & 0x0080) res |= isxdigit(LOBYTE(wc));
    if (wct & 0x0100) res |= isalpha(LOBYTE(wc));
#endif
    if (wct & 0x0040)
	FIXME(": iswctype(%04hx,_BLANK|...) requested\n",wc);
    if (wct & 0x8000)
	FIXME(": iswctype(%04hx,_LEADBYTE|...) requested\n",wc);
    return res;
}


/*********************************************************************
 *           NTDLL_iswalpha    (NTDLL)
 */
INT __cdecl NTDLL_iswalpha( WCHAR wc )
{
    return NTDLL_iswctype( wc, 0x0100 );
}
