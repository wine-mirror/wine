#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h> 

#include "winnls.h"
#include "winerror.h"
#include "debugtools.h"
#include "winversion.h"
#include "heap.h"

#include "shellapi.h"
#include "wine/undocshell.h"
#include "wine/unicode.h"

DEFAULT_DEBUG_CHANNEL(shell);

/************************* STRRET functions ****************************/

/*************************************************************************
 * StrRetToStrN					[SHELL32.96]
 * 
 * converts a STRRET to a normal string
 *
 * NOTES
 *  the pidl is for STRRET OFFSET
 */
HRESULT WINAPI StrRetToStrNA (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{
	TRACE("dest=0x%p len=0x%lx strret=0x%p pidl=%p stub\n",dest,len,src,pidl);

	switch (src->uType)
	{
	  case STRRET_WSTR:
	    WideCharToMultiByte(CP_ACP, 0, src->u.pOleStr, -1, (LPSTR)dest, len, NULL, NULL);
	    SHFree(src->u.pOleStr);
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

HRESULT WINAPI StrRetToStrNW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{
	TRACE("dest=0x%p len=0x%lx strret=0x%p pidl=%p stub\n",dest,len,src,pidl);

	switch (src->uType)
	{
	  case STRRET_WSTR:
	    lstrcpynW((LPWSTR)dest, src->u.pOleStr, len);
	    SHFree(src->u.pOleStr);
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
HRESULT WINAPI StrRetToStrNAW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{
	if(VERSION_OsIsUnicode())
	  return StrRetToStrNW (dest, len, src, pidl);
	return StrRetToStrNA (dest, len, src, pidl);
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
	return StrRetToStrNA(dest, len, src, pidl);
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
	return StrRetToStrNW(dest, len, src, pidl);
}

/************************* string functions ****************************/

/*************************************************************************
 * StrChrA					[SHLWAPI]
 */
LPSTR WINAPI StrChrA (LPCSTR str, INT c)
{
	TRACE("%s %i\n", str,c);
	return strchr(str, c);
}

/*************************************************************************
 * StrChrW					[NT 4.0:SHELL32.651]
 *
 */
LPWSTR WINAPI StrChrW (LPWSTR str, WCHAR x )
{
	TRACE("%s 0x%04x\n",debugstr_w(str),x);
	return strchrW(str, x);
}

/*************************************************************************
 * StrCmpNA					[NT 4.0:SHELL32.*]
 */
INT WINAPI StrCmpNA ( LPCSTR str1, LPCSTR str2, INT len)
{
	TRACE("%s %s %i stub\n", str1,str2,len);
	return strncmp(str1, str2, len);
}

/*************************************************************************
 * StrCmpNW					[NT 4.0:SHELL32.*]
 */
INT WINAPI StrCmpNW ( LPCWSTR wstr1, LPCWSTR wstr2, INT len)
{
	TRACE("%s %s %i stub\n", debugstr_w(wstr1),debugstr_w(wstr2),len);
	return strncmpW(wstr1, wstr2, len);
}

/*************************************************************************
 * StrCmpNIA					[NT 4.0:SHELL32.*]
 */
int WINAPI StrCmpNIA ( LPCSTR str1, LPCSTR str2, int len)
{
	TRACE("%s %s %i stub\n", str1,str2,len);
	return strncasecmp(str1, str2, len);
}

/*************************************************************************
 * StrCmpNIW					[NT 4.0:SHELL32.*]
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

/************************* OLESTR functions ****************************/

/************************************************************************
 *	StrToOleStr			[SHELL32.163]
 *
 */
int WINAPI StrToOleStrA (LPWSTR lpWideCharStr, LPCSTR lpMultiByteString)
{
	TRACE("(%p, %p %s)\n",
	lpWideCharStr, lpMultiByteString, debugstr_a(lpMultiByteString));

	return MultiByteToWideChar(0, 0, lpMultiByteString, -1, lpWideCharStr, MAX_PATH);

}
int WINAPI StrToOleStrW (LPWSTR lpWideCharStr, LPCWSTR lpWString)
{
	TRACE("(%p, %p %s)\n",
	lpWideCharStr, lpWString, debugstr_w(lpWString));

	if (lstrcpyW (lpWideCharStr, lpWString ))
	{ return lstrlenW (lpWideCharStr);
	}
	return 0;
}

BOOL WINAPI StrToOleStrAW (LPWSTR lpWideCharStr, LPCVOID lpString)
{
	if (VERSION_OsIsUnicode())
	  return StrToOleStrW (lpWideCharStr, lpString);
	return StrToOleStrA (lpWideCharStr, lpString);
}

/*************************************************************************
 * StrToOleStrN					[SHELL32.79]
 *  lpMulti, nMulti, nWide [IN]
 *  lpWide [OUT]
 */
BOOL WINAPI StrToOleStrNA (LPWSTR lpWide, INT nWide, LPCSTR lpStrA, INT nStr) 
{
	TRACE("(%p, %x, %s, %x)\n", lpWide, nWide, debugstr_an(lpStrA,nStr), nStr);
	return MultiByteToWideChar (0, 0, lpStrA, nStr, lpWide, nWide);
}
BOOL WINAPI StrToOleStrNW (LPWSTR lpWide, INT nWide, LPCWSTR lpStrW, INT nStr) 
{
	TRACE("(%p, %x, %s, %x)\n", lpWide, nWide, debugstr_wn(lpStrW, nStr), nStr);

	if (lstrcpynW (lpWide, lpStrW, nWide))
	{ return lstrlenW (lpWide);
	}
	return 0;
}

BOOL WINAPI StrToOleStrNAW (LPWSTR lpWide, INT nWide, LPCVOID lpStr, INT nStr) 
{
	if (VERSION_OsIsUnicode())
	  return StrToOleStrNW (lpWide, nWide, lpStr, nStr);
	return StrToOleStrNA (lpWide, nWide, lpStr, nStr);
}

/*************************************************************************
 * OleStrToStrN					[SHELL32.78]
 */
BOOL WINAPI OleStrToStrNA (LPSTR lpStr, INT nStr, LPCWSTR lpOle, INT nOle) 
{
	TRACE("(%p, %x, %s, %x)\n", lpStr, nStr, debugstr_wn(lpOle,nOle), nOle);
	return WideCharToMultiByte (0, 0, lpOle, nOle, lpStr, nStr, NULL, NULL);
}

BOOL WINAPI OleStrToStrNW (LPWSTR lpwStr, INT nwStr, LPCWSTR lpOle, INT nOle) 
{
	TRACE("(%p, %x, %s, %x)\n", lpwStr, nwStr, debugstr_wn(lpOle,nOle), nOle);

	if (lstrcpynW ( lpwStr, lpOle, nwStr))
	{ return lstrlenW (lpwStr);
	}
	return 0;
}

BOOL WINAPI OleStrToStrNAW (LPVOID lpOut, INT nOut, LPCVOID lpIn, INT nIn) 
{
	if (VERSION_OsIsUnicode())
	  return OleStrToStrNW (lpOut, nOut, lpIn, nIn);
	return OleStrToStrNA (lpOut, nOut, lpIn, nIn);
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

