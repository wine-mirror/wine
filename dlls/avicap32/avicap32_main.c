/*
 * Copyright 2002 Dmitry Timoshkov for Codeweavers
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

#define COM_NO_WINDOWS_H
#include "vfw.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avicap);


/***********************************************************************
 *             capCreateCaptureWindowW   (AVICAP32.@)
 */
HWND VFWAPI capCreateCaptureWindowW(LPCWSTR lpszWindowName, DWORD dwStyle, INT x,
                                    INT y, INT nWidth, INT nHeight, HWND hWnd,
                                    INT nID)
{
    FIXME("%s, %08lx, %08x, %08x, %08x, %08x, %p, %08x\n",
           debugstr_w(lpszWindowName), dwStyle,
           x, y, nWidth, nHeight, hWnd, nID);
    return 0;
}

/***********************************************************************
 *             capCreateCaptureWindowA   (AVICAP32.@)
 */
HWND VFWAPI capCreateCaptureWindowA(LPCSTR lpszWindowName, DWORD dwStyle, INT x,
                                    INT y, INT nWidth, INT nHeight, HWND hWnd,
                                    INT nID)
{   UNICODE_STRING nameW;
    HWND retW;

    if (lpszWindowName) RtlCreateUnicodeStringFromAsciiz(&nameW, lpszWindowName);
    else nameW.Buffer = NULL;

    retW = capCreateCaptureWindowW(nameW.Buffer, dwStyle, x, y, nWidth, nHeight,
                                   hWnd, nID);
    RtlFreeUnicodeString(&nameW);

    return retW;
}

/***********************************************************************
 *             capGetDriverDescriptionA   (AVICAP32.@)
 */
BOOL VFWAPI capGetDriverDescriptionA(WORD wDriverIndex, LPSTR lpszName,
				     INT cbName, LPSTR lpszVer, INT cbVer)
{
    FIXME("(%d, %p, %d, %p, %d) stub!\n", wDriverIndex, lpszName, cbName,
	  lpszVer, cbVer);
    return FALSE;
}

/***********************************************************************
 *             capGetDriverDescriptionW   (AVICAP32.@)
 */
BOOL VFWAPI capGetDriverDescriptionW(WORD wDriverIndex, LPWSTR lpszName,
				     INT cbName, LPWSTR lpszVer, INT cbVer)
{
    FIXME("(%d, %p, %d, %p, %d) stub!\n", wDriverIndex, lpszName, cbName,
	  lpszVer, cbVer);
    return FALSE;
}
