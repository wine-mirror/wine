/*
 * Copyright 2001 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "winbase.h"
#include "winnls.h"
#include "mmsystem.h"
#include "winerror.h"
#include "vfw.h"
#include "wine/debug.h"
#include "avifile_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);


/****************************************************************************
 * string APIs (internal) - Copied from wine/dlls/imm32/string.c
 */

INT AVIFILE_strlenAtoW( LPCSTR lpstr )
{
	INT	len;

	len = MultiByteToWideChar( CP_ACP, 0, lpstr, -1, NULL, 0 );
	return ( len > 0 ) ? (len-1) : 0;
}

INT AVIFILE_strlenWtoA( LPCWSTR lpwstr )
{
	INT	len;

	len = WideCharToMultiByte( CP_ACP, 0, lpwstr, -1,
				   NULL, 0, NULL, NULL );
	return ( len > 0 ) ? (len-1) : 0;
}

LPWSTR AVIFILE_strncpyAtoW( LPWSTR lpwstr, LPCSTR lpstr, INT wbuflen )
{
	INT	len;

	len = MultiByteToWideChar( CP_ACP, 0, lpstr, -1, lpwstr, wbuflen );
	if ( len == 0 )
		*lpwstr = 0;
	return lpwstr;
}

LPSTR AVIFILE_strncpyWtoA( LPSTR lpstr, LPCWSTR lpwstr, INT abuflen )
{
	INT	len;

	len = WideCharToMultiByte( CP_ACP, 0, lpwstr, -1,
				   lpstr, abuflen, NULL, NULL );
	if ( len == 0 )
		*lpstr = 0;
	return lpstr;
}

LPWSTR AVIFILE_strdupAtoW( LPCSTR lpstr )
{
	INT len;
	LPWSTR lpwstr = NULL;

	len = AVIFILE_strlenAtoW( lpstr );
	if ( len > 0 )
	{
		lpwstr = (LPWSTR)HeapAlloc( AVIFILE_data.hHeap, 0, sizeof(WCHAR)*(len+1) );
		if ( lpwstr != NULL )
			(void)AVIFILE_strncpyAtoW( lpwstr, lpstr, len+1 );
	}

	return lpwstr;
}

LPSTR AVIFILE_strdupWtoA( LPCWSTR lpwstr )
{
	INT len;
	LPSTR lpstr = NULL;

	len = AVIFILE_strlenWtoA( lpwstr );
	if ( len > 0 )
	{
		lpstr = (LPSTR)HeapAlloc( AVIFILE_data.hHeap, 0, sizeof(CHAR)*(len+1) );
		if ( lpstr != NULL )
			(void)AVIFILE_strncpyWtoA( lpstr, lpwstr, len+1 );
	}

	return lpstr;
}



