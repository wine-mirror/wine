/*
 * cap20wxx.c
 *
 * Copyright 2002-2003 AVM Computersysteme Vertriebs GmbH
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
 *
 */

#ifndef _cap20wxx_h
#define _cap20wxx_h
/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
#include <windef.h>
/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/

DWORD APIENTRY wrapCAPI_REGISTER          (DWORD MessageBufferSize, DWORD maxLogicalConnection, DWORD maxBDataBlocks, DWORD maxBDataLen, DWORD *pApplID);
DWORD APIENTRY wrapCAPI_RELEASE           (DWORD ApplID);
DWORD APIENTRY wrapCAPI_PUT_MESSAGE       (DWORD ApplID, PVOID pCAPIMessage);
DWORD APIENTRY wrapCAPI_GET_MESSAGE       (DWORD ApplID, PVOID *ppCAPIMessage);
DWORD APIENTRY wrapCAPI_WAIT_FOR_SIGNAL   (DWORD ApplID);
DWORD APIENTRY wrapCAPI_GET_MANUFACTURER  (char *SzBuffer);
DWORD APIENTRY wrapCAPI_GET_VERSION       (DWORD *pCAPIMajor, DWORD *pCAPIMinor, DWORD *pManufacturerMajor, DWORD *pManufacturerMinor);
DWORD APIENTRY wrapCAPI_GET_SERIAL_NUMBER (char *SzBuffer);
DWORD APIENTRY wrapCAPI_GET_PROFILE       (PVOID SzBuffer, DWORD CtrlNr);
DWORD APIENTRY wrapCAPI_INSTALLED         (void);
DWORD APIENTRY wrapCAPI_MANUFACTURER      (DWORD Class, DWORD Function, DWORD Ctlr, PVOID pParams, DWORD ParamsLen);

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
#endif
