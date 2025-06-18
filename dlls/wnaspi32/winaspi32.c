/*
 * Copyright 1997 Bruce Milner
 * Copyright 1998 Andreas Mohr
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

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "aspi.h"
#include "wnaspi32.h"
#include "winescsi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(aspi);


/*******************************************************************
 *     GetASPI32SupportInfo		[WNASPI32.1]
 *
 * Checks if the ASPI subsystem is initialized correctly.
 *
 * RETURNS
 *    HIWORD: 0.
 *    HIBYTE of LOWORD: status (SS_COMP or SS_FAILED_INIT)
 *    LOBYTE of LOWORD: # of host adapters.
 */
DWORD __cdecl GetASPI32SupportInfo(void)
{
    DWORD controllers = ASPI_GetNumControllers();

    if (!controllers)
	return SS_NO_ADAPTERS << 8;
    return (SS_COMP << 8) | controllers ;
}

/***********************************************************************
 *             SendASPI32Command (WNASPI32.2)
 */
DWORD __cdecl SendASPI32Command(LPSRB lpSRB)
{
    FIXME( "no longer supported, should be ported to DeviceIoControl\n" );
    return SS_INVALID_SRB;
}


/***********************************************************************
 *             GetASPI32DLLVersion   (WNASPI32.4)
 */
DWORD __cdecl GetASPI32DLLVersion(void)
{
	FIXME("Please add SCSI support for your operating system, returning 0\n");
        return 0;
}

/***********************************************************************
 *             GetASPI32Buffer   (WNASPI32.8)
 * Supposed to return a DMA capable large SCSI buffer.
 * Our implementation does not use those at all, all buffer stuff is
 * done in the kernel SG device layer. So we just heapalloc the buffer.
 */
BOOL __cdecl GetASPI32Buffer(PASPI32BUFF pab)
{
    pab->AB_BufPointer = HeapAlloc(GetProcessHeap(),
	    	pab->AB_ZeroFill?HEAP_ZERO_MEMORY:0,
		pab->AB_BufLen
    );
    if (!pab->AB_BufPointer) return FALSE;
    return TRUE;
}

/***********************************************************************
 *             FreeASPI32Buffer   (WNASPI32.14)
 */
BOOL __cdecl FreeASPI32Buffer(PASPI32BUFF pab)
{
    HeapFree(GetProcessHeap(),0,pab->AB_BufPointer);
    return TRUE;
}

/***********************************************************************
 *             TranslateASPI32Address   (WNASPI32.7)
 */
BOOL __cdecl TranslateASPI32Address(LPDWORD pdwPath, LPDWORD pdwDEVNODE)
{
    FIXME("(%p, %p), stub !\n", pdwPath, pdwDEVNODE);
    return TRUE;
}
