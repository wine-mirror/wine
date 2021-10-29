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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#define __NO_CAPIUTILS__

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define __user
#ifdef HAVE_LINUX_CAPI_H
# include <linux/capi.h>
#endif
#ifdef HAVE_CAPI20_H
# include <capi20.h>
#endif
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(capi);

/*===========================================================================*\
\*===========================================================================*/

DWORD WINAPI wrapCAPI_REGISTER (DWORD MessageBufferSize, DWORD maxLogicalConnection, DWORD maxBDataBlocks, DWORD maxBDataLen, DWORD *pApplID) {
    unsigned aid = 0;
    DWORD fret;

    fret = capi20_register (maxLogicalConnection, maxBDataBlocks, maxBDataLen, &aid);
    *pApplID   = aid;
    TRACE ( "(%x) -> %x\n", *pApplID, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI wrapCAPI_RELEASE (DWORD ApplID) {
    DWORD fret;

    fret = capi20_release (ApplID);
    TRACE ("(%x) -> %x\n", ApplID, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI wrapCAPI_PUT_MESSAGE (DWORD ApplID, PVOID pCAPIMessage) {
    DWORD fret;

    fret = capi20_put_message (ApplID, pCAPIMessage);
    TRACE ("(%x) -> %x\n", ApplID, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI wrapCAPI_GET_MESSAGE (DWORD ApplID, PVOID *ppCAPIMessage) {
    DWORD fret;

    fret = capi20_get_message (ApplID, (unsigned char **)ppCAPIMessage);
    TRACE ("(%x) -> %x\n", ApplID, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI wrapCAPI_WAIT_FOR_SIGNAL (DWORD ApplID) {
    TRACE ("(%x)\n", ApplID);

    return capi20_waitformessage (ApplID, NULL);
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI wrapCAPI_GET_MANUFACTURER (char *SzBuffer) {
    DWORD fret;

    fret = (capi20_get_manufacturer (0, (unsigned char *) SzBuffer) != 0) ? 0 : 0x1108;
    if (!strncmp (SzBuffer, "AVM", 3)) {
        strcpy (SzBuffer, "AVM-GmbH");
    }
    TRACE ("(%s) -> %x\n", SzBuffer, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI wrapCAPI_GET_VERSION (DWORD *pCAPIMajor, DWORD *pCAPIMinor, DWORD *pManufacturerMajor, DWORD *pManufacturerMinor) {
    unsigned char version[4 * sizeof (unsigned)];
    DWORD fret;

    fret = (capi20_get_version (0, version) != 0) ? 0 : 0x1108;
    *pCAPIMajor         = *(unsigned *)(version + 0 * sizeof (unsigned));
    *pCAPIMinor         = *(unsigned *)(version + 1 * sizeof (unsigned));
    *pManufacturerMajor = *(unsigned *)(version + 2 * sizeof (unsigned));
    *pManufacturerMinor = *(unsigned *)(version + 3 * sizeof (unsigned));
    TRACE ("(%x.%x,%x.%x) -> %x\n", *pCAPIMajor, *pCAPIMinor, *pManufacturerMajor,
             *pManufacturerMinor, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI wrapCAPI_GET_SERIAL_NUMBER (char *SzBuffer) {
    DWORD fret;

    fret = (capi20_get_serial_number (0, (unsigned char*) SzBuffer) != 0) ? 0 : 0x1108;
    TRACE ("(%s) -> %x\n", SzBuffer, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI wrapCAPI_GET_PROFILE (PVOID SzBuffer, DWORD CtlrNr) {
    DWORD fret;

    fret = capi20_get_profile (CtlrNr, SzBuffer);
    TRACE ("(%x,%x) -> %x\n", CtlrNr, *(unsigned short *)SzBuffer, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI wrapCAPI_INSTALLED (void) {
    DWORD fret;

    fret = capi20_isinstalled();
    TRACE ("() -> %x\n", fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI wrapCAPI_MANUFACTURER (DWORD Class, DWORD Function, DWORD Ctlr, PVOID pParams, DWORD ParamsLen) {
    FIXME ("(), not supported!\n");
    return 0x1109;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
