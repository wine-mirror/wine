/*
 * Shlwapi string functions
 *
 * Copyright 1998 Juergen Schmied
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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 * ChrCmpIA					[SHLWAPI.385]
 *
 * Note: Returns 0 (FALSE) if characters are equal (insensitive).
 */
BOOL WINAPI ChrCmpIA (WORD w1, WORD w2)
{
	TRACE("%c ? %c\n", w1, w2);
	return (toupper(w1) != toupper(w2));
}

/*************************************************************************
 * ChrCmpIW					[SHLWAPI.386]
 *
 * Note: Returns 0 (FALSE) if characters are equal (insensitive).
 */
BOOL WINAPI ChrCmpIW (WCHAR w1, WCHAR w2)
{
	TRACE("%c ? %c\n", w1, w2);
	return (toupperW(w1) != toupperW(w2));
}

/*************************************************************************
 * StrChrA					[SHLWAPI.@]
 */
LPSTR WINAPI StrChrA (LPCSTR str, WORD c)
{
	TRACE("%s %i\n", str,c);
	return strchr(str, c);
}

/*************************************************************************
 * StrChrW					[SHLWAPI.@]
 *
 */
LPWSTR WINAPI StrChrW (LPCWSTR str, WCHAR x )
{
	TRACE("%s 0x%04x\n",debugstr_w(str),x);
	return strchrW(str, x);
}

/*************************************************************************
 * StrCmpIW					[SHLWAPI.@]
 */
int WINAPI StrCmpIW ( LPCWSTR wstr1, LPCWSTR wstr2 )
{
    TRACE("%s %s\n", debugstr_w(wstr1),debugstr_w(wstr2));
    return strcmpiW( wstr1, wstr2 );
}

/*************************************************************************
 * StrCmpNA					[SHLWAPI.@]
 */
INT WINAPI StrCmpNA ( LPCSTR str1, LPCSTR str2, INT len)
{
	TRACE("%s %s %i stub\n", str1,str2,len);
	return strncmp(str1, str2, len);
}

/*************************************************************************
 * StrCmpNW					[SHLWAPI.@]
 */
INT WINAPI StrCmpNW ( LPCWSTR wstr1, LPCWSTR wstr2, INT len)
{
	TRACE("%s %s %i stub\n", debugstr_w(wstr1),debugstr_w(wstr2),len);
	return strncmpW(wstr1, wstr2, len);
}

/*************************************************************************
 * StrCmpNIA					[SHLWAPI.@]
 */
int WINAPI StrCmpNIA ( LPCSTR str1, LPCSTR str2, int len)
{
	TRACE("%s %s %i stub\n", str1,str2,len);
	return strncasecmp(str1, str2, len);
}

/*************************************************************************
 * StrCmpNIW					[SHLWAPI.@]
 */
int WINAPI StrCmpNIW ( LPCWSTR wstr1, LPCWSTR wstr2, int len)
{
	TRACE("%s %s %i stub\n", debugstr_w(wstr1),debugstr_w(wstr2),len);
	return strncmpiW(wstr1, wstr2, len);
}

/*************************************************************************
 * StrCmpW					[SHLWAPI.@]
 */
int WINAPI StrCmpW ( LPCWSTR wstr1, LPCWSTR wstr2 )
{
    TRACE("%s %s\n", debugstr_w(wstr1),debugstr_w(wstr2));
    return strcmpW( wstr1, wstr2 );
}

/*************************************************************************
 * StrCatW					[SHLWAPI.@]
 */
LPWSTR WINAPI StrCatW( LPWSTR wstr1, LPCWSTR wstr2 )
{
    return strcatW( wstr1, wstr2 );
}


/*************************************************************************
 * StrCpyW					[SHLWAPI.@]
 */
LPWSTR WINAPI StrCpyW( LPWSTR wstr1, LPCWSTR wstr2 )
{
    return strcpyW( wstr1, wstr2 );
}


/*************************************************************************
 * StrCpyNW					[SHLWAPI.@]
 */
LPWSTR WINAPI StrCpyNW( LPWSTR wstr1, LPCWSTR wstr2, int n )
{
    return lstrcpynW( wstr1, wstr2, n );
}


/*************************************************************************
 * StrStrA					[SHLWAPI.@]
 */
LPSTR WINAPI StrStrA(LPCSTR lpFirst, LPCSTR lpSrch)
{
    while (*lpFirst)
    {
        LPCSTR p1 = lpFirst, p2 = lpSrch;
        while (*p1 && *p2 && *p1 == *p2) { p1++; p2++; }
        if (!*p2) return (LPSTR)lpFirst;
        lpFirst++;
    }
    return NULL;
}

/*************************************************************************
 * StrStrW					[SHLWAPI.@]
 */
LPWSTR WINAPI StrStrW(LPCWSTR lpFirst, LPCWSTR lpSrch)
{
    while (*lpFirst)
    {
        LPCWSTR p1 = lpFirst, p2 = lpSrch;
        while (*p1 && *p2 && *p1 == *p2) { p1++; p2++; }
        if (!*p2) return (LPWSTR)lpFirst;
        lpFirst++;
    }
    return NULL;
}

/*************************************************************************
 * StrStrIA					[SHLWAPI.@]
 */
LPSTR WINAPI StrStrIA(LPCSTR lpFirst, LPCSTR lpSrch)
{
    while (*lpFirst)
    {
        LPCSTR p1 = lpFirst, p2 = lpSrch;
        while (*p1 && *p2 && toupper(*p1) == toupper(*p2)) { p1++; p2++; }
        if (!*p2) return (LPSTR)lpFirst;
        lpFirst++;
    }
    return NULL;
}

/*************************************************************************
 * StrStrIW					[SHLWAPI.@]
 */
LPWSTR WINAPI StrStrIW(LPCWSTR lpFirst, LPCWSTR lpSrch)
{
    while (*lpFirst)
    {
        LPCWSTR p1 = lpFirst, p2 = lpSrch;
        while (*p1 && *p2 && toupperW(*p1) == toupperW(*p2)) { p1++; p2++; }
        if (!*p2) return (LPWSTR)lpFirst;
        lpFirst++;
    }
    return NULL;
}

/*************************************************************************
 *	StrToIntA			[SHLWAPI.@]
 */
int WINAPI StrToIntA(LPCSTR lpSrc)
{
	TRACE("%s\n", lpSrc);
	return atol(lpSrc);
}

/*************************************************************************
 *	StrToIntW			[SHLWAPI.@]
 */
int WINAPI StrToIntW(LPCWSTR lpSrc)
{
    char buffer[32];

    TRACE("%s\n", debugstr_w(lpSrc));
    WideCharToMultiByte( CP_ACP, 0, lpSrc, -1, buffer, sizeof(buffer), NULL, NULL );
    buffer[sizeof(buffer)-1] = 0;
    return atol(buffer);
}

/*************************************************************************
 *	StrToIntExA			[SHLWAPI.@]
 */
BOOL WINAPI StrToIntExA( LPCSTR pszString, DWORD dwFlags, LPINT piRet)
{
	TRACE("%s %ld stub !\n", debugstr_a(pszString), dwFlags);
	piRet = (LPINT) StrToIntA(pszString);
	return TRUE;
}

/*************************************************************************
 *	StrToIntExW			[SHLWAPI.@]
 */
BOOL WINAPI StrToIntExW( LPCWSTR pszString, DWORD dwFlags, LPINT piRet)
{
	TRACE("%s %ld stub !\n", debugstr_w(pszString), dwFlags);
	piRet = (LPINT) StrToIntW(pszString);
	return TRUE;
}

/*************************************************************************
 *	StrDupA			[SHLWAPI.@]
 */
LPSTR WINAPI StrDupA (LPCSTR lpSrc)
{
	int len = strlen(lpSrc);
	LPSTR lpDest = (LPSTR) LocalAlloc(LMEM_FIXED, len+1);

	TRACE("%s\n", lpSrc);

	if (lpDest) strcpy(lpDest, lpSrc);
	return lpDest;
}

/*************************************************************************
 *	StrDupW			[SHLWAPI.@]
 */
LPWSTR WINAPI StrDupW (LPCWSTR lpSrc)
{
	int len = strlenW(lpSrc);
	LPWSTR lpDest = (LPWSTR) LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * (len+1));

	TRACE("%s\n", debugstr_w(lpSrc));

	if (lpDest) strcpyW(lpDest, lpSrc);
	return lpDest;
}

/*************************************************************************
 *	StrCSpnA		[SHLWAPI.@]
 */
int WINAPI StrCSpnA (LPCSTR lpStr, LPCSTR lpSet)
{
	int i,j, pos = strlen(lpStr);

	TRACE("(%p %s  %p %s)\n",
	   lpStr, debugstr_a(lpStr), lpSet, debugstr_a(lpSet));

	for (i=0; i < strlen(lpSet) ; i++ )
	{
	  for (j = 0; j < pos;j++)
	  {
	    if (lpStr[j] == lpSet[i])
	    {
	      pos = j;
	    }
	  }
	}
	TRACE("-- %u\n", pos);
	return pos;
}

/*************************************************************************
 *	StrCSpnW		[SHLWAPI.@]
 */
int WINAPI StrCSpnW (LPCWSTR lpStr, LPCWSTR lpSet)
{
	int i,j, pos = strlenW(lpStr);

	TRACE("(%p %s %p %s)\n",
	   lpStr, debugstr_w(lpStr), lpSet, debugstr_w(lpSet));

	for (i=0; i < strlenW(lpSet) ; i++ )
	{
	  for (j = 0; j < pos;j++)
	  {
	    if (lpStr[j] == lpSet[i])
	    {
	      pos = j;
	    }
	  }
	}
	TRACE("-- %u\n", pos);
	return pos;
}

/**************************************************************************
 * StrRChrA [SHLWAPI.@]
 *
 */
LPSTR WINAPI StrRChrA( LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch )
{
    LPCSTR lpGotIt = NULL;
    BOOL dbcs = IsDBCSLeadByte( LOBYTE(wMatch) );

    TRACE("(%p, %p, %x)\n", lpStart, lpEnd, wMatch);
    if (!lpStart && !lpEnd) return NULL;
    if (!lpEnd) lpEnd = lpStart + strlen(lpStart);

    for(; lpStart < lpEnd; lpStart = CharNextA(lpStart))
    {
        if (*lpStart != LOBYTE(wMatch)) continue;
        if (dbcs && lpStart[1] != HIBYTE(wMatch)) continue;
        lpGotIt = lpStart;
    }
    return (LPSTR)lpGotIt;
}


/**************************************************************************
 * StrRChrW [SHLWAPI.@]
 *
 */
LPWSTR WINAPI StrRChrW( LPCWSTR lpStart, LPCWSTR lpEnd, WORD wMatch)
{
    LPCWSTR lpGotIt = NULL;

    TRACE("(%p, %p, %x)\n", lpStart, lpEnd, wMatch);
    if (!lpStart && !lpEnd) return NULL;
    if (!lpEnd) lpEnd = lpStart + strlenW(lpStart);

    for(; lpStart < lpEnd; lpStart = CharNextW(lpStart))
        if (*lpStart == wMatch) lpGotIt = lpStart;

    return (LPWSTR)lpGotIt;
}


/**************************************************************************
 * StrRChrIA [SHLWAPI.@]
 *
 */
LPSTR WINAPI StrRChrIA( LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch )
{
    LPCSTR lpGotIt = NULL;
    BOOL dbcs = IsDBCSLeadByte( LOBYTE(wMatch) );

    TRACE("(%p, %p, %x)\n", lpStart, lpEnd, wMatch);
    if (!lpStart && !lpEnd) return NULL;
    if (!lpEnd) lpEnd = lpStart + strlen(lpStart);

    for(; lpStart < lpEnd; lpStart = CharNextA(lpStart))
    {
	if (dbcs) {
	    /*
	    if (_mbctoupper(*lpStart) == _mbctoupper(wMatch))
		lpGotIt = lpStart;
	    */
	    if (toupper(*lpStart) == toupper(wMatch)) lpGotIt = lpStart;
	} else {
	    if (toupper(*lpStart) == toupper(wMatch)) lpGotIt = lpStart;
	}
    }
    return (LPSTR)lpGotIt;
}


/**************************************************************************
 * StrRChrIW [SHLWAPI.@]
 *
 */
LPWSTR WINAPI StrRChrIW( LPCWSTR lpStart, LPCWSTR lpEnd, WORD wMatch)
{
    LPCWSTR lpGotIt = NULL;

    TRACE("(%p, %p, %x)\n", lpStart, lpEnd, wMatch);
    if (!lpStart && !lpEnd) return NULL;
    if (!lpEnd) lpEnd = lpStart + strlenW(lpStart);

    for(; lpStart < lpEnd; lpStart = CharNextW(lpStart))
        if (toupperW(*lpStart) == toupperW(wMatch)) lpGotIt = lpStart;

    return (LPWSTR)lpGotIt;
}


/*************************************************************************
 *	StrCatBuffA		[SHLWAPI.@]
 *
 * Appends back onto front, stopping when front is size-1 characters long.
 * Returns front.
 *
 */
LPSTR WINAPI StrCatBuffA(LPSTR front, LPCSTR back, INT size)
{
    LPSTR dst = front + strlen(front);
    LPCSTR src = back, end = front + size - 1;

    while(dst < end && *src)
        *dst++ = *src++;
    *dst = '\0';
    return front;
}

/*************************************************************************
 *	StrCatBuffW		[SHLWAPI.@]
 *
 * Appends back onto front, stopping when front is size-1 characters long.
 * Returns front.
 *
 */
LPWSTR WINAPI StrCatBuffW(LPWSTR front, LPCWSTR back, INT size)
{
    LPWSTR dst = front + strlenW(front);
    LPCWSTR src = back, end = front + size - 1;

    while(dst < end && *src)
        *dst++ = *src++;
    *dst = '\0';
    return front;
}

/*************************************************************************
 * StrRetToBufA					[SHLWAPI.@]
 *
 * converts a STRRET to a normal string
 *
 * NOTES
 *  the pidl is for STRRET OFFSET
 *
 * ***** NOTE *****
 *  This routine is identical to StrRetToStrNA in dlls/shell32/shellstring.c.
 *  It was duplicated there because not every version of Shlwapi.dll exports
 *  StrRetToBufA. If you change one routine, change them both. YOU HAVE BEEN
 *  WARNED.
 * ***** NOTE *****
 */
HRESULT WINAPI StrRetToBufA (LPSTRRET src, const ITEMIDLIST *pidl, LPSTR dest, DWORD len)
{
	TRACE("dest=%p len=0x%lx strret=%p pidl=%p stub\n",dest,len,src,pidl);

	switch (src->uType)
	{
	  case STRRET_WSTR:
	    WideCharToMultiByte(CP_ACP, 0, src->u.pOleStr, -1, (LPSTR)dest, len, NULL, NULL);
/*	    SHFree(src->u.pOleStr);  FIXME: is this right? */
	    break;

	  case STRRET_CSTRA:
	    lstrcpynA((LPSTR)dest, src->u.cStr, len);
	    break;

	  case STRRET_OFFSETA:
	    lstrcpynA((LPSTR)dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
	    break;

	  default:
	    FIXME("unknown type!\n");
	    if (len)
	    {
	      *(LPSTR)dest = '\0';
	    }
	    return(FALSE);
	}
	return S_OK;
}

/*************************************************************************
 * StrRetToBufW					[SHLWAPI.@]
 *
 * converts a STRRET to a normal string
 *
 * NOTES
 *  the pidl is for STRRET OFFSET
 *
 * ***** NOTE *****
 *  This routine is identical to StrRetToStrNW in dlls/shell32/shellstring.c.
 *  It was duplicated there because not every version of Shlwapi.dll exports
 *  StrRetToBufW. If you change one routine, change them both. YOU HAVE BEEN
 *  WARNED.
 * ***** NOTE *****
 */
HRESULT WINAPI StrRetToBufW (LPSTRRET src, const ITEMIDLIST *pidl, LPWSTR dest, DWORD len)
{
	TRACE("dest=%p len=0x%lx strret=%p pidl=%p stub\n",dest,len,src,pidl);

	switch (src->uType)
	{
	  case STRRET_WSTR:
	    lstrcpynW((LPWSTR)dest, src->u.pOleStr, len);
/*	    SHFree(src->u.pOleStr);  FIXME: is this right? */
	    break;

	  case STRRET_CSTRA:
              if (!MultiByteToWideChar( CP_ACP, 0, src->u.cStr, -1, dest, len ) && len)
                  dest[len-1] = 0;
	    break;

	  case STRRET_OFFSETA:
	    if (pidl)
	    {
              if (!MultiByteToWideChar( CP_ACP, 0, ((LPCSTR)&pidl->mkid)+src->u.uOffset, -1,
                                        dest, len ) && len)
                  dest[len-1] = 0;
	    }
	    break;

	  default:
	    FIXME("unknown type!\n");
	    if (len)
	    { *(LPSTR)dest = '\0';
	    }
	    return(FALSE);
	}
	return S_OK;
}

/*************************************************************************
 * StrFormatByteSizeA				[SHLWAPI.@]
 */
LPSTR WINAPI StrFormatByteSizeA ( DWORD dw, LPSTR pszBuf, UINT cchBuf )
{	char buf[64];
	TRACE("%lx %p %i\n", dw, pszBuf, cchBuf);
	if ( dw<1024L )
	{ sprintf (buf,"%ld bytes", dw);
	}
	else if ( dw<1048576L)
	{ sprintf (buf,"%3.1f KB", (FLOAT)dw/1024);
	}
	else if ( dw < 1073741824L)
	{ sprintf (buf,"%3.1f MB", (FLOAT)dw/1048576L);
	}
	else
	{ sprintf (buf,"%3.1f GB", (FLOAT)dw/1073741824L);
	}
	lstrcpynA (pszBuf, buf, cchBuf);
	return pszBuf;
}

/*************************************************************************
 * StrFormatByteSizeW				[SHLWAPI.@]
 */
LPWSTR WINAPI StrFormatByteSizeW ( DWORD dw, LPWSTR pszBuf, UINT cchBuf )
{
        char buf[64];
        StrFormatByteSizeA( dw, buf, sizeof(buf) );
        if (!MultiByteToWideChar( CP_ACP, 0, buf, -1, pszBuf, cchBuf ) && cchBuf)
            pszBuf[cchBuf-1] = 0;
        return pszBuf;
}

/*************************************************************************
 *      StrNCatA	[SHLWAPI.@]
 */
LPSTR WINAPI StrNCatA(LPSTR front, LPCSTR back, INT cchMax)
{
	TRACE("%s %s %i stub\n", debugstr_a(front),debugstr_a(back),cchMax);
	return (front);
}

/*************************************************************************
 *      StrNCatW	[SHLWAPI.@]
 */
LPWSTR WINAPI StrNCatW(LPWSTR front, LPCWSTR back, INT cchMax)
{
	TRACE("%s %s %i stub\n", debugstr_w(front),debugstr_w(back),cchMax);
	return (front);
}

/*************************************************************************
 *      StrTrimA	[SHLWAPI.@]
 */
BOOL WINAPI StrTrimA(LPSTR pszSource, LPCSTR pszTrimChars)
{
    BOOL trimmed = FALSE;
    LPSTR pSrc;
    LPCSTR pTrim;

    TRACE("('%s', '%s');\n", pszSource, pszTrimChars);
    for (pTrim = pszTrimChars; *pTrim; pTrim++)
    {
	 for (pSrc = pszSource; *pSrc; pSrc++)
	     if (*pSrc == *pTrim)
	     {
		 /* match -> remove this char.
		  * strlen(pSrc) equiv. to the correct strlen(pSrc+1)+1 */
		 memmove(pSrc, pSrc+1, strlen(pSrc));
		 trimmed = TRUE;
	     }
    }
    TRACE("<- '%s'\n", pszSource);
    return trimmed;
}
