/*
 *	Implementation of the 'IME window' class
 *
 *	Copyright 2000 Hidenori Takeshima
 *
 *
 * FIXME:
 *	- handle all messages.
 *	- handle all notifications.
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

static CHAR IMM32_szIMEClass[] = "IME";

typedef struct
{
	int dummy;
} IMM32_IMEWNDPARAM;


static BOOL IMM32_IsUIMessage( UINT nMsg );


static
LRESULT CALLBACK IMM32_IMEWndProc( HWND hwnd, UINT nMsg,
				   WPARAM wParam, LPARAM lParam )
{
	IMM32_IMEWNDPARAM* pParam =
		(IMM32_IMEWNDPARAM*)GetWindowLongA( hwnd, 0L );

	if ( nMsg == WM_CREATE )
	{
		pParam = (IMM32_IMEWNDPARAM*)IMM32_HeapAlloc(
				HEAP_ZERO_MEMORY, sizeof(IMM32_IMEWNDPARAM) );
		if ( pParam == NULL )
			return -1L;
		SetWindowLongA( hwnd, 0L, (LONG)pParam );

		/* FIXME - Initialize pParam. */

		return 0;
	}
	else if ( nMsg == WM_DESTROY )
	{
		/* FIXME - Uninitialize pParam. */

		IMM32_HeapFree( pParam );
		SetWindowLongA( hwnd, 0L, (LONG)NULL );
		return 0;
	}

	if ( pParam == NULL )
	{
		if ( IMM32_IsUIMessage( nMsg ) )
			return 0;
		return DefWindowProcA( hwnd, nMsg, wParam, lParam );
	}

	/* FIXME - handle all messages. */
	/* FIXME - handle all notifications. */
	if ( IMM32_IsUIMessage( nMsg ) )
		return 0;

	return DefWindowProcA( hwnd, nMsg, wParam, lParam );
}


/***********************************************************************
 *		IMM32_RegisterClass (internal)
 */
BOOL IMM32_RegisterIMEWndClass( HINSTANCE hInstDLL )
{
	WNDCLASSA	wc;

	/* SDK says the "IME" window class is a system global class. */
	wc.style		= CS_GLOBALCLASS;
	wc.lpfnWndProc		= IMM32_IMEWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= sizeof(LONG);
	wc.hInstance		= hInstDLL;
	wc.hIcon		= LoadIconA((HINSTANCE)NULL,IDI_APPLICATIONA);
	wc.hCursor		= LoadCursorA((HINSTANCE)NULL,IDC_IBEAMA);
	wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1); /* FIXME? */
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= IMM32_szIMEClass;
	if ( !RegisterClassA( &wc ) )
		return FALSE;

	return TRUE;
}

/***********************************************************************
 *		IMM32_UnregisterClass (internal)
 */
void IMM32_UnregisterIMEWndClass( HINSTANCE hInstDLL )
{
	(void)UnregisterClassA( IMM32_szIMEClass, hInstDLL );
}



static BOOL IMM32_IsUIMessage( UINT nMsg )
{
	switch ( nMsg )
	{
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_COMPOSITION:
	case WM_IME_SETCONTEXT:
	case WM_IME_NOTIFY:
	case WM_IME_COMPOSITIONFULL:
	case WM_IME_SELECT:
	case 0x287: /* What is this message? IMM32.DLL returns TRUE. */
		return TRUE;
	}

	return FALSE;
}


/***********************************************************************
 *		ImmIsUIMessageA (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageA(
	HWND hwndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TRACE("(0x%08x, %d, %d, %ld)\n",
	      hwndIME, msg, wParam, lParam);

	if ( !IMM32_IsUIMessage( msg ) )
		return FALSE;
	if ( hwndIME == (HWND)NULL )
		return TRUE;

	switch ( msg )
	{
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_COMPOSITION:
	case WM_IME_SETCONTEXT:
	case WM_IME_NOTIFY:
	case WM_IME_COMPOSITIONFULL:
	case WM_IME_SELECT:
		SendMessageA( hwndIME, msg, wParam, lParam );
		break;
	case 0x287: /* What is this message? */
		FIXME("(0x%08x, %d, %d, %ld) - unknown message 0x287.\n",
		      hwndIME, msg, wParam, lParam);
		SendMessageA( hwndIME, msg, wParam, lParam );
		break;
	}

	return TRUE;
}

/***********************************************************************
 *		ImmIsUIMessageW (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageW(
	HWND hwndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TRACE("(0x%08x, %d, %d, %ld)\n",
	      hwndIME, msg, wParam, lParam);

	if ( !IMM32_IsUIMessage( msg ) )
		return FALSE;
	if ( hwndIME == (HWND)NULL )
		return TRUE;

	switch ( msg )
	{
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_COMPOSITION:
	case WM_IME_SETCONTEXT:
	case WM_IME_NOTIFY:
	case WM_IME_COMPOSITIONFULL:
	case WM_IME_SELECT:
		SendMessageW( hwndIME, msg, wParam, lParam );
		break;
	case 0x287: /* What is this message? */
		FIXME("(0x%08x, %d, %d, %ld) - unknown message 0x287.\n",
		      hwndIME, msg, wParam, lParam);
		SendMessageW( hwndIME, msg, wParam, lParam );
		break;
	}

	return TRUE;
}
