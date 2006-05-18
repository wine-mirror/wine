/*
 * Copyright 2000 Corel Corporation
 * Copyright 2006 Marcus Meissner
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

#ifndef _TWAIN32_H
#define _TWAIN32_H

#ifndef __WINE_CONFIG_H
# error You must include config.h first
#endif

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "twain.h"

/* internal information about an active data source */
typedef struct tagActiveDS
{
    struct tagActiveDS	*next;			/* next active DS */
    TW_IDENTITY		identity;		/* identity */
    HMODULE		hmod;
    DSENTRYPROC		dsEntry;
} activeDS;

TW_UINT16 DSM_initialized;      /* whether Source Manager is initialized */
TW_UINT16 DSM_currentState;     /* current state of Source Manager */
TW_UINT16 DSM_twCC;             /* current condition code of Source Manager */
TW_UINT32 DSM_sourceId;         /* source id generator */
TW_UINT16 DSM_currentDevice;    /* keep track of device during enumeration */
HINSTANCE DSM_instance;

activeDS *activeSources;	/* list of active data sources */

/* Device Source Manager Control handlers */
extern TW_UINT16 TWAIN_ControlGroupHandler (
	pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
    TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
extern TW_UINT16 TWAIN_ImageGroupHandler (
	pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
	TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
extern TW_UINT16 TWAIN_AudioGroupHandler (
	pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
	TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
extern TW_UINT16 TWAIN_SourceManagerHandler (
	pTW_IDENTITY pOrigin, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);

/* Implementation of operation triplets (From Application to Source Manager) */
extern TW_UINT16 TWAIN_CloseDS
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_IdentityGetDefault
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_IdentityGetFirst
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_IdentityGetNext
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_OpenDS
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_UserSelect
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_CloseDSM
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_OpenDSM
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_GetDSMStatus
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);

#endif
