/*
 *	Helper functions for ANSI<->UNICODE string conversion
 *
 *	Copyright 2000 Hidenori Takeshima
 */

#include "config.h"

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"
#include "immddk.h"
#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(imm);

#include "imm_private.h"


INT IMM32_strlenAtoW( LPCSTR lpstr )
{
	INT	len;

	len = MultiByteToWideChar( CP_ACP, 0, lpstr, -1, NULL, 0 );
	return ( len > 0 ) ? (len-1) : 0;
}

INT IMM32_strlenWtoA( LPCWSTR lpwstr )
{
	INT	len;

	len = WideCharToMultiByte( CP_ACP, 0, lpwstr, -1,
				   NULL, 0, NULL, NULL );
	return ( len > 0 ) ? (len-1) : 0;
}

LPWSTR IMM32_strncpyAtoW( LPWSTR lpwstr, LPCSTR lpstr, INT wbuflen )
{
	INT	len;

	len = MultiByteToWideChar( CP_ACP, 0, lpstr, -1, lpwstr, wbuflen );
	if ( len == 0 )
		*lpwstr = 0;
	return lpwstr;
}

LPSTR IMM32_strncpyWtoA( LPSTR lpstr, LPCWSTR lpwstr, INT abuflen )
{
	INT	len;

	len = WideCharToMultiByte( CP_ACP, 0, lpwstr, -1,
				   lpstr, abuflen, NULL, NULL );
	if ( len == 0 )
		*lpstr = 0;
	return lpstr;
}

LPWSTR IMM32_strdupAtoW( LPCSTR lpstr )
{
	INT len;
	LPWSTR lpwstr = NULL;

	len = IMM32_strlenAtoW( lpstr );
	if ( len > 0 )
	{
		lpwstr = (LPWSTR)IMM32_HeapAlloc( 0, sizeof(WCHAR)*(len+1) );
		if ( lpwstr != NULL )
			(void)IMM32_strncpyAtoW( lpwstr, lpstr, len+1 );
	}

	return lpwstr;
}

LPSTR IMM32_strdupWtoA( LPCWSTR lpwstr )
{
	INT len;
	LPSTR lpstr = NULL;

	len = IMM32_strlenWtoA( lpwstr );
	if ( len > 0 )
	{
		lpstr = (LPSTR)IMM32_HeapAlloc( 0, sizeof(CHAR)*(len+1) );
		if ( lpstr != NULL )
			(void)IMM32_strncpyWtoA( lpstr, lpwstr, len+1 );
	}

	return lpstr;
}
