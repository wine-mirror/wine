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
        return StrRetToBufA( src, pidl, dest, len );
}

HRESULT WINAPI StrRetToStrNW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{
        return StrRetToBufW( src, pidl, dest, len );
}

HRESULT WINAPI StrRetToStrNAW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{
	if(VERSION_OsIsUnicode())
	  return StrRetToStrNW (dest, len, src, pidl);
	return StrRetToStrNA (dest, len, src, pidl);
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
