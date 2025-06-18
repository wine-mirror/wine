/*
 * Copyright 1994 Erik Bos
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "wine/windef16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(stress);

/***********************************************************************
 *		AllocDiskSpace (STRESS.10)
 */
INT16 WINAPI AllocDiskSpace(LONG lLeft, UINT16 uDrive)
{
	FIXME("(%d, %ld) - stub\n",
		       uDrive, lLeft);

	return 1;
}

/***********************************************************************
 *		AllocFileHandles (STRESS.6)
 */
INT16 WINAPI AllocFileHandles(INT16 Left)
{
	TRACE("(%d) - stub\n", Left);

	if (Left < 0)
		return -1;
	else
		return 1;
}

/***********************************************************************
 *		AllocGDIMem (STRESS.14)
 */
BOOL16 WINAPI AllocGDIMem(UINT16 uLeft)
{
	TRACE("(%d) - stub\n", uLeft);
        return TRUE;
}

/***********************************************************************
 *		AllocMem (STRESS.2)
 */
BOOL16 WINAPI AllocMem(DWORD dwLeft)
{
	FIXME("(%ld) - stub\n", dwLeft);
        return TRUE;
}

/***********************************************************************
 *		AllocUserMem (STRESS.12)
 */
BOOL16 WINAPI AllocUserMem(UINT16 uContig)
{
	TRACE("AllocUserMem %d\n", uContig);
        return TRUE;
}

/***********************************************************************
 *		FreeAllMem (STRESS.3)
 */
void WINAPI FreeAllMem(void)
{
	TRACE("FreeAllMem\n");
}

/***********************************************************************
 *		FreeAllGDIMem (STRESS.15)
 */
void WINAPI FreeAllGDIMem(void)
{
	TRACE("FreeAllGDIMem\n");
}

/***********************************************************************
 *		FreeAllUserMem (STRESS.13)
 */
void WINAPI FreeAllUserMem(void)
{
	TRACE("FreeAllUserMem\n");
}

/***********************************************************************
 *		GetFreeFileHandles (STRESS.8)
 */
INT16 WINAPI GetFreeFileHandles(void)
{
	TRACE("GetFreeFileHandles\n");
        return 256;  /* can't have more than 256 16-bit handles */
}

/***********************************************************************
 *		UnAllocDiskSpace (STRESS.11)
 */
void WINAPI UnAllocDiskSpace(UINT16 drive)
{
	TRACE("UnAllocDiskSpace %d\n", drive);
}

/***********************************************************************
 *		UnAllocFileHandles (STRESS.7)
 */
void WINAPI UnAllocFileHandles(void)
{
	TRACE("GetFreeAllUserMem\n");
}
