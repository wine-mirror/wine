/*
 * File Decompression Interface
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
#include "fdi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

/***********************************************************************
 *		FDICreate (CABINET.20)
 */
HFDI __cdecl FDICreate(
	PFNALLOC pfnalloc,
	PFNFREE  pfnfree,
	PFNOPEN  pfnopen,
	PFNREAD  pfnread,
	PFNWRITE pfnwrite,
	PFNCLOSE pfnclose,
	PFNSEEK  pfnseek,
	int      cpuType,
	PERF     perf)
{
    FIXME("(%p, %p, %p, %p, %p, %p, %p, %d, %p): stub\n", 
	  pfnalloc, pfnfree, pfnopen, pfnread, pfnwrite, pfnclose, pfnseek,
	  cpuType, perf);

    perf->erfOper = FDIERROR_NONE;
    perf->erfType = 0;
    perf->fError = TRUE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}


/***********************************************************************
 *		FDIIsCabinet (CABINET.21)
 */
BOOL __cdecl FDIIsCabinet(
	HFDI            hfdi,
	INT_PTR         hf,
	PFDICABINETINFO pfdici)
{
    FIXME("(%p, %d, %p): stub\n", hfdi, hf, pfdici);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		FDICopy (CABINET.22)
 */
BOOL __cdecl FDICopy(
	HFDI           hfdi,
	char          *pszCabinet,
	char          *pszCabPath,
	int            flags,
	PFNFDINOTIFY   pfnfdin,
	PFNFDIDECRYPT  pfnfdid,
	void          *pvUser)
{
    FIXME("(%p, %p, %p, %d, %p, %p, %p): stub\n",
	  hfdi, pszCabinet, pszCabPath, flags, pfnfdin, pfnfdid, pvUser);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		FDIDestroy (CABINET.23)
 */
BOOL __cdecl FDIDestroy(HFDI hfdi)
{
    FIXME("(%p): stub\n", hfdi);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		FDICreate (CABINET.20)
 */
BOOL __cdecl FDITruncateCabinet(
	HFDI    hfdi,
	char   *pszCabinetName,
	USHORT  iFolderToDelete)
{
    FIXME("(%p, %p, %hu): stub\n", hfdi, pszCabinetName, iFolderToDelete);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}
