/*
 * Tablet Context
 *
 * Copyright 2002 Patrik Stridvall
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

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "wintab.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintab32);

/***********************************************************************
 *		WTInfoA (WINTAB32.20)
 */
UINT WINAPI WTInfoA(UINT wCategory, UINT nIndex, LPVOID lpOutput)
{
    FIXME("(%u, %u, %p): stub\n", wCategory, nIndex, lpOutput);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}

/***********************************************************************
 *		WTInfoW (WINTAB32.1020)
 */
UINT WINAPI WTInfoW(UINT wCategory, UINT nIndex, LPVOID lpOutput)
{
    FIXME("(%u, %u, %p): stub\n", wCategory, nIndex, lpOutput);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}

/***********************************************************************
 *		WTOpenA (WINTAB32.21)
 */
HCTX WINAPI WTOpenA(HWND hWnd, LPLOGCONTEXTA lpLogCtx, BOOL fEnable)
{
    FIXME("(%p, %p, %u): stub\n", hWnd, lpLogCtx, fEnable);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}

/***********************************************************************
 *		WTOpenW (WINTAB32.1021)
 */
HCTX WINAPI WTOpenW(HWND hWnd, LPLOGCONTEXTW lpLogCtx, BOOL fEnable)
{
    FIXME("(%p, %p, %u): stub\n", hWnd, lpLogCtx, fEnable);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}

/***********************************************************************
 *		WTClose (WINTAB32.22)
 */
BOOL WINAPI WTClose(HCTX hCtx)
{
    FIXME("(%p): stub\n", hCtx);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return TRUE;
}

/***********************************************************************
 *		WTPacketsGet (WINTAB32.23)
 */
int WINAPI WTPacketsGet(HCTX hCtx, int cMaxPkts, LPVOID lpPkts)
{
    FIXME("(%p, %d, %p): stub\n", hCtx, cMaxPkts, lpPkts);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}

/***********************************************************************
 *		WTPacket (WINTAB32.24)
 */
BOOL WINAPI WTPacket(HCTX hCtx, UINT wSerial, LPVOID lpPkt)
{
    FIXME("(%p, %d, %p): stub\n", hCtx, wSerial, lpPkt);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTEnable (WINTAB32.40)
 */
BOOL WINAPI WTEnable(HCTX hCtx, BOOL fEnable)
{
    FIXME("(%p, %u): stub\n", hCtx, fEnable);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTOverlap (WINTAB32.41)
 */
BOOL WINAPI WTOverlap(HCTX hCtx, BOOL fToTop)
{
    FIXME("(%p, %u): stub\n", hCtx, fToTop);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTConfig (WINTAB32.61)
 */
BOOL WINAPI WTConfig(HCTX hCtx, HWND hWnd)
{
    FIXME("(%p, %p): stub\n", hCtx, hWnd);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTGetA (WINTAB32.61)
 */
BOOL WINAPI WTGetA(HCTX hCtx, LPLOGCONTEXTA lpLogCtx)
{
    FIXME("(%p, %p): stub\n", hCtx, lpLogCtx);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTGetW (WINTAB32.1061)
 */
BOOL WINAPI WTGetW(HCTX hCtx, LPLOGCONTEXTW lpLogCtx)
{
    FIXME("(%p, %p): stub\n", hCtx, lpLogCtx);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTSetA (WINTAB32.62)
 */
BOOL WINAPI WTSetA(HCTX hCtx, LPLOGCONTEXTA lpLogCtx)
{
    FIXME("(%p, %p): stub\n", hCtx, lpLogCtx);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTSetW (WINTAB32.1062)
 */
BOOL WINAPI WTSetW(HCTX hCtx, LPLOGCONTEXTW lpLogCtx)
{
    FIXME("(%p, %p): stub\n", hCtx, lpLogCtx);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTExtGet (WINTAB32.63)
 */
BOOL WINAPI WTExtGet(HCTX hCtx, UINT wExt, LPVOID lpData)
{
    FIXME("(%p, %u, %p): stub\n", hCtx, wExt, lpData);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTExtSet (WINTAB32.64)
 */
BOOL WINAPI WTExtSet(HCTX hCtx, UINT wExt, LPVOID lpData)
{
    FIXME("(%p, %u, %p): stub\n", hCtx, wExt, lpData);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTSave (WINTAB32.65)
 */
BOOL WINAPI WTSave(HCTX hCtx, LPVOID lpSaveInfo)
{
    FIXME("(%p, %p): stub\n", hCtx, lpSaveInfo);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTRestore (WINTAB32.66)
 */
HCTX WINAPI WTRestore(HWND hWnd, LPVOID lpSaveInfo, BOOL fEnable)
{
    FIXME("(%p, %p, %u): stub\n", hWnd, lpSaveInfo, fEnable);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}

/***********************************************************************
 *		WTPacketsPeek (WINTAB32.80)
 */
int WINAPI WTPacketsPeek(HCTX hCtx, int cMaxPkts, LPVOID lpPkts)
{
    FIXME("(%p, %d, %p): stub\n", hCtx, cMaxPkts, lpPkts);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}

/***********************************************************************
 *		WTDataGet (WINTAB32.81)
 */
int WINAPI WTDataGet(HCTX hCtx, UINT wBegin, UINT wEnd,
		     int cMaxPkts, LPVOID lpPkts, LPINT lpNPkts)
{
    FIXME("(%p, %u, %u, %d, %p, %p): stub\n",
	  hCtx, wBegin, wEnd, cMaxPkts, lpPkts, lpNPkts);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}

/***********************************************************************
 *		WTDataPeek (WINTAB32.82)
 */
int WINAPI WTDataPeek(HCTX hCtx, UINT wBegin, UINT wEnd,
		      int cMaxPkts, LPVOID lpPkts, LPINT lpNPkts)
{
    FIXME("(%p, %u, %u, %d, %p, %p): stub\n",
	  hCtx, wBegin, wEnd, cMaxPkts, lpPkts, lpNPkts);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}

/***********************************************************************
 *		WTQueuePacketsEx (WINTAB32.200)
 */
BOOL WINAPI WTQueuePacketsEx(HCTX hCtx, UINT *lpOld, UINT *lpNew)
{
    FIXME("(%p, %p, %p): stub\n", hCtx, lpOld, lpNew);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return TRUE;
}

/***********************************************************************
 *		WTQueueSizeGet (WINTAB32.84)
 */
int WINAPI WTQueueSizeGet(HCTX hCtx)
{
    FIXME("(%p): stub\n", hCtx);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}

/***********************************************************************
 *		WTQueueSizeSet (WINTAB32.85)
 */
BOOL WINAPI WTQueueSizeSet(HCTX hCtx, int nPkts)
{
    FIXME("(%p, %d): stub\n", hCtx, nPkts);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}
