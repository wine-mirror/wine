/*
 *	Input Method Context
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

static HIMC IMM32_CreateIMC( HKL hkl );
static BOOL IMM32_DestroyIMC( HIMC hIMC );

IMM32_IMC* IMM32_LockIMC( HIMC hIMC )
{
	IMM32_IMC*	pIMC;

	if ( hIMC == NULLIMC )
	{
		SetLastError( ERROR_INVALID_HANDLE );
		return NULL;
	}

	pIMC = (IMM32_IMC*)IMM32_MoveableLock( (IMM32_MOVEABLEMEM*)hIMC );
	if ( !pIMC->fSelected )
	{
		(void)IMM32_MoveableUnlock( (IMM32_MOVEABLEMEM*)hIMC );
		SetLastError( ERROR_ACCESS_DENIED );
		return NULL;
	}

	return pIMC;
}

BOOL IMM32_UnlockIMC( HIMC hIMC )
{
	if ( hIMC == NULLIMC )
	{
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	return IMM32_MoveableUnlock( (IMM32_MOVEABLEMEM*)hIMC );
}

static HIMC IMM32_CreateIMC( HKL hkl )
{
	IMM32_MOVEABLEMEM*	hIMC;
	IMM32_IMC*		pIMC;
	LPCOMPOSITIONSTRING	lpCompStr;
	LPCANDIDATEINFO		lpCandInfo;
	LPGUIDELINE		lpGuideLine;

	hIMC = IMM32_MoveableAlloc( 0, sizeof( IMM32_IMC ) );
	if ( hIMC == NULL )
	{
		SetLastError( ERROR_OUTOFMEMORY );
		return NULLIMC;
	}

	pIMC = (IMM32_IMC*)IMM32_MoveableLock( hIMC );

	/* Initialize some members of IMC. */
	pIMC->context.hWnd = (HWND)NULL;
	pIMC->context.fOpen = FALSE;
	pIMC->context.hCompStr = (HIMCC)NULL;
	pIMC->context.hCandInfo = (HIMCC)NULL;
	pIMC->context.hGuideLine = (HIMCC)NULL;
	pIMC->context.hPrivate = (HIMCC)NULL;
	pIMC->context.dwNumMsgBuf = 0;
	pIMC->context.hMsgBuf = (HIMCC)NULL;
	pIMC->context.fdwInit = 0;
	pIMC->pkl = NULL;
	pIMC->fSelected = FALSE;

	/* hCompStr, hCandInfo, hGuideLine, hMsgBuf must be allocated. */
	pIMC->context.hCompStr = ImmCreateIMCC( sizeof(COMPOSITIONSTRING) );
	if ( pIMC->context.hCompStr == (HIMCC)NULL )
		goto out_of_memory;
	lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC( pIMC->context.hCompStr );
	if ( lpCompStr == NULL )
		goto out_of_memory;
	lpCompStr->dwSize = sizeof(COMPOSITIONSTRING);
	(void)ImmUnlockIMCC( pIMC->context.hCompStr );

	pIMC->context.hCandInfo = ImmCreateIMCC( sizeof(CANDIDATEINFO) );
	if ( pIMC->context.hCandInfo == (HIMCC)NULL )
		goto out_of_memory;
	lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC( pIMC->context.hCandInfo );
	if ( lpCandInfo == NULL )
		goto out_of_memory;
	lpCandInfo->dwSize = sizeof(CANDIDATEINFO);
	(void)ImmUnlockIMCC( pIMC->context.hCandInfo );

	pIMC->context.hGuideLine = ImmCreateIMCC( sizeof(GUIDELINE) );
	if ( pIMC->context.hGuideLine == (HIMCC)NULL )
		goto out_of_memory;
	lpGuideLine = (LPGUIDELINE)ImmLockIMCC( pIMC->context.hGuideLine );
	if ( lpGuideLine == NULL )
		goto out_of_memory;
	lpGuideLine->dwSize = sizeof(GUIDELINE);
	(void)ImmUnlockIMCC( pIMC->context.hGuideLine );

	pIMC->context.hMsgBuf = ImmCreateIMCC( 0 );
	if ( pIMC->context.hMsgBuf == (HIMCC)NULL )
		goto out_of_memory;

	pIMC->pkl = IMM32_GetIME( hkl );
	if ( pIMC->pkl != NULL )
	{
		/* The current HKL is IME.
		 * Initialize IME's private context.
		 */
		if ( pIMC->pkl->imeinfo.dwPrivateDataSize > 0 )
		{
			pIMC->context.hPrivate = ImmCreateIMCC(
				pIMC->pkl->imeinfo.dwPrivateDataSize );
			if ( pIMC->context.hPrivate == (HIMCC)NULL )
				goto out_of_memory;
		}

		pIMC->fSelected = TRUE;
		if ( !pIMC->pkl->handlers.pImeSelect( (HIMC)hIMC, TRUE ) )
		{
			pIMC->fSelected = FALSE;
			goto out_of_memory;
		}
	}

	(void)IMM32_MoveableUnlock( hIMC );

	return (HIMC)hIMC;

out_of_memory:
	(void)IMM32_DestroyIMC( (HIMC)hIMC );
	SetLastError( ERROR_OUTOFMEMORY );
	return NULLIMC;
}

static BOOL IMM32_DestroyIMC( HIMC hIMC )
{
	IMM32_IMC*		pIMC;

	if ( hIMC == NULLIMC )
	{
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	pIMC = (IMM32_IMC*)IMM32_MoveableLock( (IMM32_MOVEABLEMEM*)hIMC );

	if ( pIMC->context.hWnd != (HWND)NULL )
	{
		FIXME( "please release lock of the context.hWnd!" );
	}

	if ( pIMC->fSelected )
	{
		(void)pIMC->pkl->handlers.pImeSelect( hIMC, FALSE );
		pIMC->fSelected = FALSE;
	}

	if ( pIMC->context.hCompStr != (HIMCC)NULL )
		(void)ImmDestroyIMCC(pIMC->context.hCompStr);
	if ( pIMC->context.hCandInfo != (HIMCC)NULL )
		(void)ImmDestroyIMCC(pIMC->context.hCandInfo);
	if ( pIMC->context.hGuideLine != (HIMCC)NULL )
		(void)ImmDestroyIMCC(pIMC->context.hGuideLine);
	if ( pIMC->context.hPrivate != (HIMCC)NULL )
		(void)ImmDestroyIMCC(pIMC->context.hPrivate);
	if ( pIMC->context.hMsgBuf != (HIMCC)NULL )
		(void)ImmDestroyIMCC(pIMC->context.hMsgBuf);

	IMM32_MoveableFree( (IMM32_MOVEABLEMEM*)hIMC );

	return TRUE;
}





/***********************************************************************
 *		ImmCreateContext (IMM32.@)
 */
HIMC WINAPI ImmCreateContext( void )
{
	TRACE("()\n");

	return IMM32_CreateIMC( GetKeyboardLayout(0) );
}

/***********************************************************************
 *		ImmDestroyContext (IMM32.@)
 */
BOOL WINAPI ImmDestroyContext( HIMC hIMC )
{
	TRACE("(0x%08x)\n",hIMC);

	return IMM32_DestroyIMC( hIMC );
}

/***********************************************************************
 *		ImmLockIMC (IMM32.@)
 */
LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC hIMC)
{
	IMM32_IMC*	pIMC;

	TRACE("(0x%08x)\n", (unsigned)hIMC);

	pIMC = IMM32_LockIMC( hIMC );
	if ( pIMC == NULL )
		return NULL;
	return &(pIMC->context);
}

/***********************************************************************
 *		ImmUnlockIMC (IMM32.@)
 */
BOOL WINAPI ImmUnlockIMC(HIMC hIMC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMC);

	return IMM32_UnlockIMC( hIMC );
}

/***********************************************************************
 *		ImmGetIMCLockCount (IMM32.@)
 */
DWORD WINAPI ImmGetIMCLockCount(HIMC hIMC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMC);

	if ( hIMC == NULLIMC )
	{
		SetLastError( ERROR_INVALID_HANDLE );
		return 0;
	}

	return IMM32_MoveableGetLockCount( (IMM32_MOVEABLEMEM*)hIMC );
}


