#include <ctype.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>

#include "winerror.h"
#include "wine/undocshell.h"
#include "wine/unicode.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 * StrChrA					[SHLWAPI]
 */
LPSTR WINAPI StrChrA (LPCSTR str, INT c)
{
	TRACE("%s %i\n", str,c);
	return strchr(str, c);
}

/*************************************************************************
 * StrChrW					[SHLWAPI]
 *
 */
LPWSTR WINAPI StrChrW (LPWSTR str, WCHAR x )
{
	TRACE("%s 0x%04x\n",debugstr_w(str),x);
	return strchrW(str, x);
}

/*************************************************************************
 * StrCmpNA					[SHLWAPI]
 */
INT WINAPI StrCmpNA ( LPCSTR str1, LPCSTR str2, INT len)
{
	TRACE("%s %s %i stub\n", str1,str2,len);
	return strncmp(str1, str2, len);
}

/*************************************************************************
 * StrCmpNW					[SHLWAPI]
 */
INT WINAPI StrCmpNW ( LPCWSTR wstr1, LPCWSTR wstr2, INT len)
{
	TRACE("%s %s %i stub\n", debugstr_w(wstr1),debugstr_w(wstr2),len);
	return strncmpW(wstr1, wstr2, len);
}

/*************************************************************************
 * StrCmpNIA					[SHLWAPI]
 */
int WINAPI StrCmpNIA ( LPCSTR str1, LPCSTR str2, int len)
{
	TRACE("%s %s %i stub\n", str1,str2,len);
	return strncasecmp(str1, str2, len);
}

/*************************************************************************
 * StrCmpNIW					[SHLWAPI]
 */
int WINAPI StrCmpNIW ( LPCWSTR wstr1, LPCWSTR wstr2, int len)
{
	TRACE("%s %s %i stub\n", debugstr_w(wstr1),debugstr_w(wstr2),len);
	return strncmpiW(wstr1, wstr2, len);
}

/*************************************************************************
 * StrStrA					[SHLWAPI]
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
 * StrStrW					[SHLWAPI]
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
 * StrStrIA					[SHLWAPI]
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
 * StrStrIW					[SHLWAPI]
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
 *	StrToIntA			[SHLWAPI]
 */
int WINAPI StrToIntA(LPCSTR lpSrc)
{
	TRACE("%s\n", lpSrc);
	return atol(lpSrc);
}

/*************************************************************************
 *	StrToIntW			[SHLWAPI]
 */
int WINAPI StrToIntW(LPCWSTR lpSrc)
{
	int ret;
	LPSTR lpStr =  HEAP_strdupWtoA(GetProcessHeap(),0,lpSrc);

	TRACE("%s\n", debugstr_w(lpSrc));

	ret = atol(lpStr);
	HeapFree(GetProcessHeap(),0,lpStr);
	return ret;
}

/*************************************************************************
 *	StrDupA			[SHLWAPI]
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
 *	StrDupW			[SHLWAPI]
 */
LPWSTR WINAPI StrDupW (LPCWSTR lpSrc)
{
	int len = lstrlenW(lpSrc);
	LPWSTR lpDest = (LPWSTR) LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * (len+1));
	
	TRACE("%s\n", debugstr_w(lpSrc));

	if (lpDest) lstrcpyW(lpDest, lpSrc);
	return lpDest;
}

/*************************************************************************
 *	StrCSpnA		[SHLWAPI]
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
 *	StrCSpnW		[SHLWAPI]
 */
int WINAPI StrCSpnW (LPCWSTR lpStr, LPCWSTR lpSet)
{
	int i,j, pos = lstrlenW(lpStr);

	TRACE("(%p %s %p %s)\n",
	   lpStr, debugstr_w(lpStr), lpSet, debugstr_w(lpSet));

	for (i=0; i < lstrlenW(lpSet) ; i++ )
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
 *	StrCatBuffA		[SHLWAPI]
 *
 * Appends back onto front, stopping when front is size-1 characters long.
 * Returns front.
 *
 */
LPSTR WINAPI StrCatBuffA(LPSTR front, LPCSTR back, INT size)
{
    LPSTR dst = front + lstrlenA(front);
    LPCSTR src = back, end = front + size - 1;

    while(dst < end && *src)
        *dst++ = *src++;
    *dst = '\0';
    return front;
}

/*************************************************************************
 *	StrCatBuffW		[SHLWAPI]
 *
 * Appends back onto front, stopping when front is size-1 characters long.
 * Returns front.
 *
 */
LPWSTR WINAPI StrCatBuffW(LPWSTR front, LPCWSTR back, INT size)
{
    LPWSTR dst = front + lstrlenW(front);
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
 */
HRESULT WINAPI StrRetToBufA (LPSTRRET src, LPITEMIDLIST pidl, LPSTR dest, DWORD len)
{
	TRACE("dest=0x%p len=0x%lx strret=0x%p pidl=%p stub\n",dest,len,src,pidl);

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
 */
HRESULT WINAPI StrRetToBufW (LPSTRRET src, LPITEMIDLIST pidl, LPWSTR dest, DWORD len)
{
	TRACE("dest=0x%p len=0x%lx strret=0x%p pidl=%p stub\n",dest,len,src,pidl);

	switch (src->uType)
	{
	  case STRRET_WSTR:
	    lstrcpynW((LPWSTR)dest, src->u.pOleStr, len);
/*	    SHFree(src->u.pOleStr);  FIXME: is this right? */
	    break;

	  case STRRET_CSTRA:
	    lstrcpynAtoW((LPWSTR)dest, src->u.cStr, len);
	    break;

	  case STRRET_OFFSETA:
	    if (pidl)
	    {
	      lstrcpynAtoW((LPWSTR)dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
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
 * StrFormatByteSizeA				[SHLWAPI]
 */
LPSTR WINAPI StrFormatByteSizeA ( DWORD dw, LPSTR pszBuf, UINT cchBuf )
{	char buf[64];
	TRACE("%lx %p %i\n", dw, pszBuf, cchBuf);
	if ( dw<1024L )
	{ sprintf (buf,"%3.1f bytes", (FLOAT)dw);
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
 * StrFormatByteSizeW				[SHLWAPI]
 */
LPWSTR WINAPI StrFormatByteSizeW ( DWORD dw, LPWSTR pszBuf, UINT cchBuf )
{	char buf[64];
	TRACE("%lx %p %i\n", dw, pszBuf, cchBuf);
	if ( dw<1024L )
	{ sprintf (buf,"%3.1f bytes", (FLOAT)dw);
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
	lstrcpynAtoW (pszBuf, buf, cchBuf);
	return pszBuf;	
}

/*************************************************************************
 *      wnsprintfA	[SHLWAPI]
 */
int WINAPIV wnsprintfA(LPSTR lpOut, int cchLimitIn, LPCSTR lpFmt, ...)
{
    va_list valist;
    INT res;

    va_start( valist, lpFmt );
    res = wvsnprintfA( lpOut, cchLimitIn, lpFmt, valist );
    va_end( valist );
    return res;
}

/*************************************************************************
 *      wnsprintfW	[SHLWAPI]
 */
int WINAPIV wnsprintfW(LPWSTR lpOut, int cchLimitIn, LPCWSTR lpFmt, ...)
{
    va_list valist;
    INT res;

    va_start( valist, lpFmt );
    res = wvsnprintfW( lpOut, cchLimitIn, lpFmt, valist );
    va_end( valist );
    return res;
}
