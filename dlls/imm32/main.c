/*
 *	The entry point of IMM32.DLL.
 *
 *	Copyright 2000 Hidenori Takeshima
 */

#include "config.h"

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "immddk.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(imm);

#include "imm_private.h"

static DWORD			IMM32_dwProcessAttached = 0;
static HANDLE			IMM32_hHeap;
static DWORD			IMM32_dwTLSIndex;
static CRITICAL_SECTION		IMM32_csIMM;

static BOOL IMM32_InitProcessMem( void );
static void IMM32_CleanupProcessMem( void );
static void IMM32_InitThreadData( void );
static void IMM32_CleanupThreadData( void );


/***********************************************************************
 *		IMM32_DllMain
 */
BOOL WINAPI IMM32_DllMain(
	HINSTANCE hInstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved )
{
	switch ( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
		if ( IMM32_dwProcessAttached > 0 )
		{
			ERR( "cannot attach to two or more processes.\n" );
			return FALSE;
		}
		IMM32_dwProcessAttached ++;

		IMM32_InitProcessMem();
		IMM32_RegisterIMEWndClass( hInstDLL );
		break;
	case DLL_PROCESS_DETACH:
		IMM32_UnloadAllIMEs();
		IMM32_UnregisterIMEWndClass( hInstDLL );
		IMM32_CleanupProcessMem();

		IMM32_dwProcessAttached --;
		break;
	case DLL_THREAD_ATTACH:
		IMM32_InitThreadData();
		break;
	case DLL_THREAD_DETACH:
		IMM32_CleanupThreadData();
		break;
	}

	return TRUE;
}

static BOOL IMM32_InitProcessMem( void )
{
	IMM32_hHeap = (HANDLE)NULL;
	IMM32_dwTLSIndex = (DWORD)0xffffffff;

	IMM32_hHeap = HeapCreate( 0, 0x10000, 0 );
	if ( IMM32_hHeap == (HANDLE)NULL )
	{
		ERR( "cannot allocate heap for IMM32.\n" );
		return FALSE;
	}

	IMM32_dwTLSIndex = TlsAlloc();
	if ( IMM32_dwTLSIndex == (DWORD)0xffffffff )
	{
		ERR( "cannot allocate a TLS for IMM.\n" );
		return FALSE;
	}

	InitializeCriticalSection( &IMM32_csIMM );

	return TRUE;
}

static void IMM32_CleanupProcessMem( void )
{
	DeleteCriticalSection( &IMM32_csIMM );

	if ( IMM32_dwTLSIndex != (DWORD)0xffffffff )
	{
		TlsFree( IMM32_dwTLSIndex );
		IMM32_dwTLSIndex = (DWORD)0xffffffff;
	}

	if ( IMM32_hHeap != (HANDLE)NULL )
	{
		(void)HeapDestroy( IMM32_hHeap );
		IMM32_hHeap = (HANDLE)NULL;
	}
}

LPVOID IMM32_HeapAlloc( DWORD dwFlags, DWORD dwSize )
{
	return HeapAlloc( IMM32_hHeap, dwFlags, dwSize );
}

LPVOID IMM32_HeapReAlloc( DWORD dwFlags, LPVOID lpv, DWORD dwSize )
{
	return HeapReAlloc( IMM32_hHeap, dwFlags, lpv, dwSize );
}

void IMM32_HeapFree( LPVOID lpv )
{
	if ( lpv != NULL )
		HeapFree( IMM32_hHeap, 0, lpv );
}


static void IMM32_InitThreadData( void )
{
	TlsSetValue( IMM32_dwTLSIndex, NULL );
}

static void IMM32_CleanupThreadData( void )
{
	IMM32_THREADDATA*	pData;

	pData = (IMM32_THREADDATA*)TlsGetValue( IMM32_dwTLSIndex );
	if ( pData != NULL )
	{
		/* Destroy Thread-local Data. */
		if ( pData->hwndIME != (HWND)NULL )
			DestroyWindow( pData->hwndIME );
		if ( pData->hIMC != NULLIMC )
			ImmDestroyContext( pData->hIMC );

		IMM32_HeapFree( pData );
		TlsSetValue( IMM32_dwTLSIndex, NULL );
	}
}

IMM32_THREADDATA* IMM32_GetThreadData( void )
{
	IMM32_THREADDATA*	pData;

	pData = (IMM32_THREADDATA*)TlsGetValue( IMM32_dwTLSIndex );
	if ( pData != NULL )
		return pData;

	pData = (IMM32_THREADDATA*)
			IMM32_HeapAlloc( 0, sizeof(IMM32_THREADDATA) );
	if ( pData == NULL )
		return NULL;

	/* Initialize Thread-local Data. */
	pData->hwndIME = (HWND)NULL;
	pData->hIMC = NULLIMC;

	TlsSetValue( IMM32_dwTLSIndex, pData );

	return pData;
}

HIMC IMM32_GetDefaultContext( void )
{
	IMM32_THREADDATA*	pData;

	pData = IMM32_GetThreadData();
	if ( pData == NULL )
		return NULLIMC;
	if ( pData->hIMC == NULLIMC )
		pData->hIMC = ImmCreateContext();

	return pData->hIMC;
}

HWND IMM32_GetDefaultIMEWnd( void )
{
	IMM32_THREADDATA*	pData;

	pData = IMM32_GetThreadData();
	if ( pData == NULL )
		return NULLIMC;
	if ( pData->hwndIME == (HWND)NULL )
		pData->hwndIME = IMM32_CreateDefaultIMEWnd();

	return pData->hwndIME;
}


void IMM32_Lock( void )
{
	EnterCriticalSection( &IMM32_csIMM );
}

void IMM32_Unlock( void )
{
	LeaveCriticalSection( &IMM32_csIMM );
}
