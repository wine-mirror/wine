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

#include <stdio.h>
#include <sys/types.h>

#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(capi);

#define CAPI_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )


BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( instance );
        return !__wine_init_unix_call();
    }
    return TRUE;
}

/*===========================================================================* \
\*===========================================================================*/

DWORD WINAPI CAPI_REGISTER (DWORD MessageBufferSize, DWORD maxLogicalConnection, DWORD maxBDataBlocks, DWORD maxBDataLen, DWORD *pApplID) {
    struct register_params params = { MessageBufferSize, maxLogicalConnection,
                                      maxBDataBlocks, maxBDataLen, pApplID };
    DWORD fret;

    fret = CAPI_CALL( register, &params );
    TRACE ( "(%lx) -> %lx\n", *pApplID, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI CAPI_RELEASE (DWORD ApplID) {
    struct release_params params = { ApplID };
    DWORD fret;

    fret = CAPI_CALL( release, &params );
    TRACE ("(%lx) -> %lx\n", ApplID, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI CAPI_PUT_MESSAGE (DWORD ApplID, PVOID pCAPIMessage) {
    struct put_message_params params = { ApplID, pCAPIMessage };
    DWORD fret;

    fret = CAPI_CALL( put_message, &params );
    TRACE ("(%lx) -> %lx\n", ApplID, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI CAPI_GET_MESSAGE (DWORD ApplID, PVOID *ppCAPIMessage) {
    struct get_message_params params = { ApplID, ppCAPIMessage };
    DWORD fret;

    fret = CAPI_CALL( get_message, &params );
    TRACE ("(%lx) -> %lx\n", ApplID, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI CAPI_WAIT_FOR_SIGNAL (DWORD ApplID) {
    struct waitformessage_params params = { ApplID };
    TRACE ("(%lx)\n", ApplID);

    return CAPI_CALL( waitformessage, &params );
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI CAPI_GET_MANUFACTURER (char *SzBuffer) {
    struct get_manufacturer_params params = { SzBuffer };
    DWORD fret;

    fret = CAPI_CALL( get_manufacturer, &params );
    if (!strncmp (SzBuffer, "AVM", 3)) {
        strcpy (SzBuffer, "AVM-GmbH");
    }
    TRACE ("(%s) -> %lx\n", SzBuffer, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI CAPI_GET_VERSION (DWORD *pCAPIMajor, DWORD *pCAPIMinor, DWORD *pManufacturerMajor, DWORD *pManufacturerMinor) {
    struct get_version_params params = { pCAPIMajor, pCAPIMinor, pManufacturerMajor, pManufacturerMinor };
    DWORD fret;

    fret = CAPI_CALL( get_version, &params );
    TRACE ("(%lx.%lx,%lx.%lx) -> %lx\n", *pCAPIMajor, *pCAPIMinor, *pManufacturerMajor,
             *pManufacturerMinor, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI CAPI_GET_SERIAL_NUMBER (char *SzBuffer) {
    struct get_serial_number_params params = { SzBuffer };
    DWORD fret;

    fret = CAPI_CALL( get_serial_number, &params );
    TRACE ("(%s) -> %lx\n", SzBuffer, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI CAPI_GET_PROFILE (PVOID SzBuffer, DWORD CtlrNr) {
    struct get_profile_params params = { SzBuffer, CtlrNr };
    DWORD fret;

    fret = CAPI_CALL( get_profile, &params );
    TRACE ("(%lx,%x) -> %lx\n", CtlrNr, *(unsigned short *)SzBuffer, fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI CAPI_INSTALLED (void) {
    DWORD fret;

    fret = CAPI_CALL( isinstalled, NULL );
    TRACE ("() -> %lx\n", fret);
    return fret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
DWORD WINAPI CAPI_MANUFACTURER (DWORD Class, DWORD Function, DWORD Ctlr, PVOID pParams, DWORD ParamsLen) {
    FIXME ("(), not supported!\n");
    return 0x1109;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/
