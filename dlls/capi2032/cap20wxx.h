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

/*---------------------------------------------------------------------------*\
 * Modul : cap20wxx.h created at Thu 06-Jul-2000 19:58:17 by RStolz
 *
 * Id: cap20wxx.h 1.3 2002/12/04 09:43:22Z rstolz Exp 
 *
 * Log: cap20wxx.h 
 * Revision 1.3  2002/12/04 09:43:22Z  rstolz
 * - RStolz: CCDI interface removed
 * Revision 1.2  2002/08/27 09:32:15Z  rstolz
 * - RStolz: new incarnation AVMCDDI.dll supports PNP notifications
 * Revision 1.1  2000/09/07 11:02:40Z  DFriedel
 * Initial revision
 *
 * CAPI 2.0-Schnittstelle fÅr Windows-Programme 32 und 64 Bit
\*---------------------------------------------------------------------------*/
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
