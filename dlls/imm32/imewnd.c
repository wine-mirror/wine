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

#define	IMM32_CONVERSION_BUFSIZE		200

static CHAR IMM32_szIMEClass[] = "IME";
static CHAR IMM32_szIMEWindowName[] = "Default IME";

typedef struct
{
	HWND	hwndSelf;
	HWND	hwndActive;
	DWORD	dwBufUsed;
	union
	{
		CHAR	A[IMM32_CONVERSION_BUFSIZE];
		WCHAR	W[IMM32_CONVERSION_BUFSIZE];
	}		buf;
} IMM32_IMEWNDPARAM;


static BOOL IMM32_IsUIMessage( UINT nMsg );

static
LRESULT IMM32_IMEWnd_WM_KEYDOWN( IMM32_IMEWNDPARAM* pParam,
				 WPARAM wParam, LPARAM lParam )
{
	BYTE			bKeyState[ 256 ];
	DWORD			dwTransBufSize;
	UINT			nNumOfMsg;
	LRESULT			lr;
	HIMC			hIMC;
	IMM32_IMC*		pIMC;
	const IMM32_IMEKL*	pkl;

	if ( pParam->hwndActive == (HWND)NULL )
		return 0;

	/* get context -> get pkl. */
	hIMC = ImmGetContext( pParam->hwndActive );
	if ( hIMC == NULLIMC )
		return 0;
	pIMC = IMM32_LockIMC( hIMC );
	if ( pIMC == NULL )
	{
		ImmReleaseContext( pParam->hwndActive, hIMC );
		return 0;
	}
	pkl = pIMC->pkl;

	GetKeyboardState( bKeyState );
	if ( !pkl->handlers.pImeProcessKey
		( hIMC, wParam, lParam, bKeyState ) )
	{
		lr = SendMessageA( pParam->hwndActive, WM_IME_KEYDOWN,
				   wParam, lParam );
		goto end;
	}

	dwTransBufSize = 0;
	nNumOfMsg = pkl->handlers.pImeToAsciiEx
		( wParam, (lParam>>16)&0xff, 
		  bKeyState, &dwTransBufSize,
		  0, /* FIXME!!! - 1 if a menu is active */
		  hIMC );

	/* FIXME - process generated messages */
	/* I cannot use ImmGenerateMessage() since
	 * the IME window must handle generated messages. */

	/* NOTE - I must check pkl->fUnicode. */
	FIXME( "%d messages generated.\n", nNumOfMsg );

	lr = 0;
end:
	IMM32_UnlockIMC( hIMC );
	ImmReleaseContext( pParam->hwndActive, hIMC );

	return lr;
}

static
LRESULT IMM32_IMEWnd_WM_KEYUP( IMM32_IMEWNDPARAM* pParam,
			       WPARAM wParam, LPARAM lParam )
{
	BYTE			bKeyState[ 256 ];
	LRESULT			lr;
	HIMC			hIMC;
	IMM32_IMC*		pIMC;
	const IMM32_IMEKL*	pkl;

	if ( pParam->hwndActive == (HWND)NULL )
		return 0;

	/* get context -> get pkl. */
	hIMC = ImmGetContext( pParam->hwndActive );
	if ( hIMC == NULLIMC )
		return 0;
	pIMC = IMM32_LockIMC( hIMC );
	if ( pIMC == NULL )
	{
		ImmReleaseContext( pParam->hwndActive, hIMC );
		return 0;
	}
	pkl = pIMC->pkl;

	GetKeyboardState( bKeyState );
	if ( !pkl->handlers.pImeProcessKey
		( hIMC, wParam, lParam, bKeyState ) )
	{
		lr = SendMessageA( pParam->hwndActive, WM_IME_KEYUP,
				   wParam, lParam );
		goto end;
	}

	lr = 0;
end:
	IMM32_UnlockIMC( hIMC );
	ImmReleaseContext( pParam->hwndActive, hIMC );

	return lr;
}


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

		/* Initialize pParam. */
		pParam->hwndSelf = hwnd;
		pParam->hwndActive = (HWND)NULL;
		pParam->dwBufUsed = 0;

		return 0;
	}
	else if ( nMsg == WM_DESTROY )
	{
		/* Uninitialize pParam. */

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
	switch ( nMsg )
	{
	case WM_KEYDOWN:
		return IMM32_IMEWnd_WM_KEYDOWN( pParam, wParam, lParam );
	case WM_KEYUP:
		return IMM32_IMEWnd_WM_KEYUP( pParam, wParam, lParam );
	case WM_IME_KEYDOWN:
		ERR( "Why WM_IME_KEYDOWN is generated?\n" );
		return 0;
	case WM_IME_KEYUP:
		ERR( "Why WM_IME_KEYUP is generated?\n" );
		return 0;
	case WM_IME_CHAR:
		FIXME( "ignore WM_IME_CHAR - wParam %08x, lParam %08lx.\n",
		       wParam, lParam );
		return 0;
	case WM_CHAR:
		/* TranslateMessage don't support IME HKL. - FIXME? */
		FIXME( "ignore WM_CHAR - wParam %08x, lParam %08lx.\n",
		       wParam, lParam );
		return 0;
	case WM_IME_CONTROL:
	case WM_IME_REQUEST:
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_COMPOSITION:
	case WM_IME_SETCONTEXT:
	case WM_IME_NOTIFY:
	case WM_IME_COMPOSITIONFULL:
	case WM_IME_SELECT:
	case 0x287: /* What is this message? IMM32.DLL returns TRUE. */
		FIXME( "handle message %08x\n", nMsg );
		return 0;
	}

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
	wc.hIcon		= (HICON)NULL;
	wc.hCursor		= LoadCursorA((HINSTANCE)NULL,IDC_ARROWA);
	wc.hbrBackground	= (HBRUSH)NULL;
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

/***********************************************************************
 *		IMM32_CreateDefaultIMEWnd (internal)
 */

HWND IMM32_CreateDefaultIMEWnd( void )
{
	return CreateWindowExA( 0L,
				IMM32_szIMEClass,
				IMM32_szIMEWindowName,
				WS_POPUP | WS_CLIPSIBLINGS | WS_OVERLAPPED,
				0, 0, 0, 0,
				(HWND)NULL,
				(HMENU)NULL,
				(HINSTANCE)GetModuleHandleA(NULL),
				NULL );
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
