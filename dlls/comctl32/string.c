/*
 * String manipulation functions
 *
 * Copyright 1998 Eric Kohl
 *           1998 Juergen Schmied <j.schmied@metronet.de>
 *           2000 Eric Kohl for CodeWeavers
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
 *
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h> /* atoi */

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"

#include "wine/unicode.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

/**************************************************************************
 * StrChrA [COMCTL32.350]
 *
 * Find a given character in a string.
 *
 * PARAMS
 *  lpszStr [I] String to search in.
 *  ch      [I] Character to search for.
 *
 * RETURNS
 *  Success: A pointer to the first occurrence of ch in lpszStr, or NULL if
 *           not found.
 *  Failure: NULL, if any arguments are invalid.
 */
LPSTR WINAPI StrChrA (LPCSTR lpszStr, CHAR ch)
{
    return strchr (lpszStr, ch);
}


/**************************************************************************
 * StrStrIA [COMCTL32.355]
 *
 * Find a substring within a string, ignoring case.
 *
 * PARAMS
 *  lpStr1    [I] String to search in
 *  lpStr2    [I] String to look for
 *
 * RETURNS
 *  The start of lpszSearch within lpszStr, or NULL if not found.
 */
LPSTR WINAPI StrStrIA (LPCSTR lpStr1, LPCSTR lpStr2)
{
    INT len1, len2, i;
    CHAR  first;

    if (*lpStr2 == 0)
	return ((LPSTR)lpStr1);
    len1 = 0;
    while (lpStr1[len1] != 0) ++len1;
    len2 = 0;
    while (lpStr2[len2] != 0) ++len2;
    if (len2 == 0)
	return ((LPSTR)(lpStr1 + len1));
    first = tolower (*lpStr2);
    while (len1 >= len2) {
	if (tolower(*lpStr1) == first) {
	    for (i = 1; i < len2; ++i)
		if (tolower (lpStr1[i]) != tolower(lpStr2[i]))
		    break;
	    if (i >= len2)
		return ((LPSTR)lpStr1);
        }
	++lpStr1; --len1;
    }
    return (NULL);
}

/**************************************************************************
 * StrToIntA [COMCTL32.357]
 *
 * Read a signed integer from a string.
 *
 * PARAMS
 *  lpszStr [I] String to read integer from
 *
 * RETURNS
 *   The signed integer value represented by the string, or 0 if no integer is
 *   present.
 */
INT WINAPI StrToIntA (LPSTR lpszStr)
{
    return atoi(lpszStr);
}

/**************************************************************************
 * StrStrIW [COMCTL32.363]
 *
 * See StrStrIA.
 */
LPWSTR WINAPI StrStrIW (LPCWSTR lpStr1, LPCWSTR lpStr2)
{
    INT len1, len2, i;
    WCHAR  first;

    if (*lpStr2 == 0)
	return ((LPWSTR)lpStr1);
    len1 = 0;
    while (lpStr1[len1] != 0) ++len1;
    len2 = 0;
    while (lpStr2[len2] != 0) ++len2;
    if (len2 == 0)
	return ((LPWSTR)(lpStr1 + len1));
    first = tolowerW (*lpStr2);
    while (len1 >= len2) {
	if (tolowerW (*lpStr1) == first) {
	    for (i = 1; i < len2; ++i)
		if (tolowerW (lpStr1[i]) != tolowerW(lpStr2[i]))
		    break;
	    if (i >= len2)
		return ((LPWSTR)lpStr1);
        }
	++lpStr1; --len1;
    }
    return (NULL);
}

/**************************************************************************
 * StrToIntW [COMCTL32.365]
 *
 * See StrToIntA.
 */
INT WINAPI StrToIntW (LPWSTR lpString)
{
    return atoiW(lpString);
}

/**************************************************************************
 * StrCSpnA [COMCTL32.356]
 *
 * Find the length of the start of a string that does not contain certain
 * characters.
 *
 * PARAMS
 *  lpszStr   [I] String to search
 *  lpszMatch [I] Characters that cannot be in the substring
 *
 * RETURNS
 *  The length of the part of lpszStr containing only chars not in lpszMatch,
 *  or 0 if any parameter is invalid.
 */
INT WINAPI StrCSpnA( LPCSTR lpStr, LPCSTR lpSet)
{
  return strcspn(lpStr, lpSet);
}

/**************************************************************************
 * StrChrW [COMCTL32.358]
 *
 * See StrChrA.
 */
LPWSTR WINAPI StrChrW( LPCWSTR lpStart, WORD wMatch)
{
  return strchrW(lpStart, wMatch);
}

/**************************************************************************
 * StrCmpNA [COMCTL32.352]
 *
 * Compare two strings, up to a maximum length.
 *
 * PARAMS
 *  lpszStr  [I] First string to compare
 *  lpszComp [I] Second string to compare
 *  iLen     [I] Maximum number of chars to compare.
 *
 * RETURNS
 *  An integer less than, equal to or greater than 0, indicating that
 *  lpszStr is less than, the same, or greater than lpszComp.
 */
INT WINAPI StrCmpNA( LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
  return strncmp(lpStr1, lpStr2, nChar);
}

/**************************************************************************
 * StrCmpNIA [COMCTL32.353]
 *
 */
INT WINAPI StrCmpNIA( LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
  return strncasecmp(lpStr1, lpStr2, nChar);
}

/**************************************************************************
 * StrCmpNW [COMCTL32.360]
 *
 */
INT WINAPI StrCmpNW( LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
  return strncmpW(lpStr1, lpStr2, nChar);
}

/**************************************************************************
 * StrCmpNIW [COMCTL32.361]
 *
 */
INT WINAPI StrCmpNIW( LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
  FIXME("(%s, %s, %i): stub\n", debugstr_w(lpStr1), debugstr_w(lpStr2), nChar);
  return 0;
}

/**************************************************************************
 * StrRChrA [COMCTL32.351]
 *
 */
LPSTR WINAPI StrRChrA( LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch )
{
    LPCSTR lpGotIt = NULL;
    BOOL dbcs = IsDBCSLeadByte( LOBYTE(wMatch) );

    TRACE("(%p, %p, %x)\n", lpStart, lpEnd, wMatch);

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
 * StrRChrW [COMCTL32.359]
 *
 */
LPWSTR WINAPI StrRChrW( LPCWSTR lpStart, LPCWSTR lpEnd, WORD wMatch)
{
    LPCWSTR lpGotIt = NULL;

    TRACE("(%p, %p, %x)\n", lpStart, lpEnd, wMatch);
    if (!lpEnd) lpEnd = lpStart + strlenW(lpStart);

    for(; lpStart < lpEnd; lpStart = CharNextW(lpStart))
        if (*lpStart == wMatch) lpGotIt = lpStart;

    return (LPWSTR)lpGotIt;
}


/**************************************************************************
 * StrStrA [COMCTL32.354]
 *
 */
LPSTR WINAPI StrStrA( LPCSTR lpFirst, LPCSTR lpSrch)
{
  return strstr(lpFirst, lpSrch);
}

/**************************************************************************
 * StrStrW [COMCTL32.362]
 *
 */
LPWSTR WINAPI StrStrW( LPCWSTR lpFirst, LPCWSTR lpSrch)
{
  return strstrW(lpFirst, lpSrch);
}

/**************************************************************************
 * StrSpnW [COMCTL32.364]
 *
 */
INT WINAPI StrSpnW( LPWSTR lpStr, LPWSTR lpSet)
{
  LPWSTR lpLoop = lpStr;

  /* validate ptr */
  if ((lpStr == 0) || (lpSet == 0)) return 0;

/* while(*lpLoop) { if lpLoop++; } */

  for(; (*lpLoop != 0); lpLoop++)
    if( strchrW(lpSet, *(WORD*)lpLoop))
      return (INT)(lpLoop-lpStr);

  return (INT)(lpLoop-lpStr);
}
