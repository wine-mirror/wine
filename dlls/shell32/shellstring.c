/*
 * Copyright 2000 Juergen Schmied
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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"

#include "shlobj.h"
#include "shlwapi.h"
#include "shell32_main.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

DWORD WINAPI CheckEscapesW(WCHAR *string, DWORD len);

/*************************************************************************
 * StrRetToStrN    [SHELL32.96]
 *
 * converts a STRRET to a normal string
 *
 * NOTES
 *  the pidl is for STRRET OFFSET
 */
BOOL WINAPI StrRetToStrNAW(LPVOID dest, DWORD len, LPSTRRET src, const ITEMIDLIST *pidl)
{
    HRESULT res;

    if(SHELL_OsIsUnicode())
        res = StrRetToBufW(src, pidl, dest, len);
    else
        res = StrRetToBufA(src, pidl, dest, len);

    return res == S_OK || res == E_NOT_SUFFICIENT_BUFFER || src->uType > STRRET_CSTR;
}

/************************* OLESTR functions ****************************/

/************************************************************************
 *	StrToOleStr			[SHELL32.163]
 *
 */
static int StrToOleStrA (LPWSTR lpWideCharStr, LPCSTR lpMultiByteString)
{
	TRACE("(%p, %p %s)\n",
	lpWideCharStr, lpMultiByteString, debugstr_a(lpMultiByteString));

	return MultiByteToWideChar(CP_ACP, 0, lpMultiByteString, -1, lpWideCharStr, MAX_PATH);

}
static int StrToOleStrW (LPWSTR lpWideCharStr, LPCWSTR lpWString)
{
	TRACE("(%p, %p %s)\n",
	lpWideCharStr, lpWString, debugstr_w(lpWString));

	lstrcpyW (lpWideCharStr, lpWString );
	return lstrlenW(lpWideCharStr);
}

BOOL WINAPI StrToOleStrAW (LPWSTR lpWideCharStr, LPCVOID lpString)
{
	if (SHELL_OsIsUnicode())
	  return StrToOleStrW (lpWideCharStr, lpString);
	return StrToOleStrA (lpWideCharStr, lpString);
}

/*************************************************************************
 * StrToOleStrN					[SHELL32.79]
 *  lpMulti, nMulti, nWide [IN]
 *  lpWide [OUT]
 */
static BOOL StrToOleStrNA (LPWSTR lpWide, INT nWide, LPCSTR lpStrA, INT nStr)
{
	TRACE("(%p, %x, %s, %x)\n", lpWide, nWide, debugstr_an(lpStrA,nStr), nStr);
	return MultiByteToWideChar (CP_ACP, 0, lpStrA, nStr, lpWide, nWide);
}
static BOOL StrToOleStrNW (LPWSTR lpWide, INT nWide, LPCWSTR lpStrW, INT nStr)
{
	TRACE("(%p, %x, %s, %x)\n", lpWide, nWide, debugstr_wn(lpStrW, nStr), nStr);

	if (lstrcpynW (lpWide, lpStrW, nWide))
	{ return lstrlenW (lpWide);
	}
	return FALSE;
}

BOOL WINAPI StrToOleStrNAW (LPWSTR lpWide, INT nWide, LPCVOID lpStr, INT nStr)
{
	if (SHELL_OsIsUnicode())
	  return StrToOleStrNW (lpWide, nWide, lpStr, nStr);
	return StrToOleStrNA (lpWide, nWide, lpStr, nStr);
}

/*************************************************************************
 * OleStrToStrN					[SHELL32.78]
 */
static BOOL OleStrToStrNA (LPSTR lpStr, INT nStr, LPCWSTR lpOle, INT nOle)
{
	TRACE("(%p, %x, %s, %x)\n", lpStr, nStr, debugstr_wn(lpOle,nOle), nOle);
	return WideCharToMultiByte (CP_ACP, 0, lpOle, nOle, lpStr, nStr, NULL, NULL);
}

static BOOL OleStrToStrNW (LPWSTR lpwStr, INT nwStr, LPCWSTR lpOle, INT nOle)
{
	TRACE("(%p, %x, %s, %x)\n", lpwStr, nwStr, debugstr_wn(lpOle,nOle), nOle);

	if (lstrcpynW ( lpwStr, lpOle, nwStr))
	{ return lstrlenW (lpwStr);
	}
        return FALSE;
}

BOOL WINAPI OleStrToStrNAW (LPVOID lpOut, INT nOut, LPCVOID lpIn, INT nIn)
{
	if (SHELL_OsIsUnicode())
	  return OleStrToStrNW (lpOut, nOut, lpIn, nIn);
	return OleStrToStrNA (lpOut, nOut, lpIn, nIn);
}


/*************************************************************************
 * CheckEscapesA             [SHELL32.@]
 *
 * Checks a string for special characters which are not allowed in a path
 * and encloses it in quotes if that is the case.
 *
 * PARAMS
 *  string     [I/O] string to check and on return eventually quoted
 *  len        [I]   length of string
 *
 * RETURNS
 *  length of actual string
 *
 * NOTES
 *  Not really sure if this function returns actually a value at all. 
 */
DWORD WINAPI CheckEscapesA(
	LPSTR	string,         /* [I/O]   string to check ??*/
	DWORD	len)            /* [I]      is 0 */
{
	LPWSTR wString;
	DWORD ret = 0;

	TRACE("(%s %ld)\n", debugstr_a(string), len);
	wString = LocalAlloc(LPTR, len * sizeof(WCHAR));
	if (wString)
	{
	  MultiByteToWideChar(CP_ACP, 0, string, len, wString, len);
	  ret = CheckEscapesW(wString, len);
	  WideCharToMultiByte(CP_ACP, 0, wString, len, string, len, NULL, NULL);
	  LocalFree(wString);
	}
	return ret;
}

/*************************************************************************
 * CheckEscapesW             [SHELL32.@]
 *
 * See CheckEscapesA.
 */
DWORD WINAPI CheckEscapesW(
	LPWSTR	string,
	DWORD	len)
{
	DWORD size = lstrlenW(string);
	LPWSTR s, d;

	TRACE("%s, %lu.\n", debugstr_w(string), len);

	if (StrPBrkW(string, L" \",;^") && size + 2 <= len)
	{
	  s = &string[size - 1];
	  d = &string[size + 2];
	  *d-- = 0;
	  *d-- = '"';
	  for (;d > string;)
	    *d-- = *s--;
	  *d = '"';
	  return size + 2;
	}
	return size;
}
