/*
 * Copyright (C) Hidenori TAKESHIMA
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
 *	FIXME - stub
 */

#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "vfw.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avicap32);


/***********************************************************************
 *
 *		capCreateCaptureWindowA (AVICAP32.@)
 *
 */
HWND WINAPI capCreateCaptureWindowA( LPCSTR pszTitle, DWORD dwStyle, int x, int y, int width, int height, HWND hwndParent, int nID )
{
	FIXME( "(%s,%lx,%d,%d,%d,%d,%x,%d)\n",debugstr_a(pszTitle),dwStyle,x,y,width,height,hwndParent,nID );

	return (HWND)NULL;
}

/***********************************************************************
 *
 *		capCreateCaptureWindowW (AVICAP32.@)
 *
 */
HWND WINAPI capCreateCaptureWindowW( LPCWSTR pwszTitle, DWORD dwStyle, int x, int y, int width, int height, HWND hwndParent, int nID )
{
	FIXME( "(%s,%lx,%d,%d,%d,%d,%x,%d)\n",debugstr_w(pwszTitle),dwStyle,x,y,width,height,hwndParent,nID );

	return (HWND)NULL;
}

/***********************************************************************
 *
 *		capGetDriverDescriptionA (AVICAP32.@)
 *
 */
BOOL WINAPI capGetDriverDescriptionA( UINT uDriverIndex, LPSTR pszName, int cbName, LPSTR pszVersion, int cbVersion )
{
	FIXME( "(%u,%p,%d,%p,%d)\n",uDriverIndex,pszName,cbName,pszVersion,cbVersion );

	return FALSE;
}

/***********************************************************************
 *
 *		capGetDriverDescriptionW (AVICAP32.@)
 *
 */
BOOL WINAPI capGetDriverDescriptionW( UINT uDriverIndex, LPWSTR pwszName, int cbName, LPWSTR pwszVersion, int cbVersion )
{
	FIXME( "(%u,%p,%d,%p,%d)\n",uDriverIndex,pwszName,cbName,pwszVersion,cbVersion );

	return FALSE;
}

