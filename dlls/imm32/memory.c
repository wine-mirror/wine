/*
 *	This file implements 'moveable' memory block.
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

#define	IMM32_MOVEABLEMEM_LOCK_MAX	((DWORD)0xffffffff)

struct IMM32_tagMOVEABLEMEM
{
	DWORD				dwLockCount;
	DWORD				dwSize;
	LPVOID				lpvMem;
};

IMM32_MOVEABLEMEM* IMM32_MoveableAlloc( DWORD dwHeapFlags, DWORD dwHeapSize )
{
	IMM32_MOVEABLEMEM*	lpMoveable;

	lpMoveable = (IMM32_MOVEABLEMEM*)
		IMM32_HeapAlloc( 0, sizeof( IMM32_MOVEABLEMEM ) );
	if ( lpMoveable != NULL )
	{
		lpMoveable->dwLockCount = 0;
		lpMoveable->dwSize = dwHeapSize;
		lpMoveable->lpvMem = NULL;

		if ( dwHeapSize > 0 )
		{
			lpMoveable->lpvMem =
				IMM32_HeapAlloc( dwHeapFlags, dwHeapSize );
			if ( lpMoveable->lpvMem == NULL )
			{
				IMM32_HeapFree( lpMoveable );
				lpMoveable = NULL;
			}
		}
	}

	return lpMoveable;
}

void IMM32_MoveableFree( IMM32_MOVEABLEMEM* lpMoveable )
{
	IMM32_HeapFree( lpMoveable->lpvMem );
	IMM32_HeapFree( lpMoveable );
}

BOOL IMM32_MoveableReAlloc( IMM32_MOVEABLEMEM* lpMoveable,
			    DWORD dwHeapFlags, DWORD dwHeapSize )
{
	LPVOID	lpv;

	if ( dwHeapSize > 0 )
	{
		if ( lpMoveable->dwLockCount > 0 )
			dwHeapFlags |= HEAP_REALLOC_IN_PLACE_ONLY;
		lpv = IMM32_HeapReAlloc( dwHeapFlags,
					 lpMoveable->lpvMem, dwHeapSize );
		if ( lpv == NULL )
			return FALSE;
	}
	else
	{
		IMM32_HeapFree( lpMoveable->lpvMem );
		lpv = NULL;
	}

	lpMoveable->dwSize = dwHeapSize;
	lpMoveable->lpvMem = lpv;

	return TRUE;
}

LPVOID IMM32_MoveableLock( IMM32_MOVEABLEMEM* lpMoveable )
{
	if ( lpMoveable->dwLockCount == IMM32_MOVEABLEMEM_LOCK_MAX )
	{
		ERR( "lock count is 0xffffffff\n" );
	}
	else
	{
		lpMoveable->dwLockCount ++;
	}

	return lpMoveable->lpvMem;
}

BOOL IMM32_MoveableUnlock( IMM32_MOVEABLEMEM* lpMoveable )
{
	if ( lpMoveable->dwLockCount == 0 )
		return FALSE;

	if ( --lpMoveable->dwLockCount > 0 )
		return TRUE;

	return FALSE;
}

DWORD IMM32_MoveableGetLockCount( IMM32_MOVEABLEMEM* lpMoveable )
{
	return lpMoveable->dwLockCount;
}

DWORD IMM32_MoveableGetSize( IMM32_MOVEABLEMEM* lpMoveable )
{
	return lpMoveable->dwSize;
}



/***********************************************************************
 *		ImmCreateIMCC (IMM32.@)
 *
 * Create IMCC(IMC Component).
 */
HIMCC WINAPI ImmCreateIMCC(DWORD dwSize)
{
	IMM32_MOVEABLEMEM* lpMoveable;

	TRACE("(%lu)\n", dwSize);

	lpMoveable = IMM32_MoveableAlloc( HEAP_ZERO_MEMORY, dwSize );
	if ( lpMoveable == NULL )
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return (HIMCC)NULL;
	}

	return (HIMCC)lpMoveable;
}

/***********************************************************************
 *		ImmDestroyIMCC (IMM32.@)
 *
 * Destroy IMCC(IMC Component).
 */
HIMCC WINAPI ImmDestroyIMCC(HIMCC hIMCC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMCC);

	IMM32_MoveableFree( (IMM32_MOVEABLEMEM*)hIMCC );
	return (HIMCC)NULL;
}

/***********************************************************************
 *		ImmLockIMCC (IMM32.@)
 */
LPVOID WINAPI ImmLockIMCC(HIMCC hIMCC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMCC);

	return IMM32_MoveableLock( (IMM32_MOVEABLEMEM*)hIMCC );
}

/***********************************************************************
 *		ImmUnlockIMCC (IMM32.@)
 */
BOOL WINAPI ImmUnlockIMCC(HIMCC hIMCC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMCC);

	return IMM32_MoveableUnlock( (IMM32_MOVEABLEMEM*)hIMCC );
}

/***********************************************************************
 *		ImmGetIMCCLockCount (IMM32.@)
 */
DWORD WINAPI ImmGetIMCCLockCount(HIMCC hIMCC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMCC);

	return IMM32_MoveableGetLockCount( (IMM32_MOVEABLEMEM*)hIMCC );
}

/***********************************************************************
 *		ImmReSizeIMCC (IMM32.@)
 */
HIMCC WINAPI ImmReSizeIMCC(HIMCC hIMCC, DWORD dwSize)
{
	TRACE("(0x%08x,%lu)\n", (unsigned)hIMCC, dwSize);

	if ( !IMM32_MoveableReAlloc( (IMM32_MOVEABLEMEM*)hIMCC,
				     HEAP_ZERO_MEMORY, dwSize ) )
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return (HIMCC)NULL;
	}

	return hIMCC;
}

/***********************************************************************
 *		ImmGetIMCCSize (IMM32.@)
 */
DWORD WINAPI ImmGetIMCCSize(HIMCC hIMCC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMCC);

	return IMM32_MoveableGetSize( (IMM32_MOVEABLEMEM*)hIMCC );
}


