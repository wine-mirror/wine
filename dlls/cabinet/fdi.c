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
    return NULL;
}


/***********************************************************************
 *		FDICreate (CABINET.20)
 */
BOOL __cdecl FDIIsCabinet(
	HFDI            hfdi,
	INT_PTR         hf,
	PFDICABINETINFO pfdici)
{
    return FALSE;
}

/***********************************************************************
 *		FDICreate (CABINET.20)
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
    return FALSE;
}

/***********************************************************************
 *		FDICreate (CABINET.20)
 */
BOOL __cdecl FDIDestroy(HFDI hfdi)
{
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
    return FALSE;
}
