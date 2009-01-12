/*
 * HTTPAPI implementation
 *
 * Copyright 2009 Austin English
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "http.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(httpapi);

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID lpv )
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/***********************************************************************
 *        HttpInitialize       (HTTPAPI.@)
 *
 * Initializes HTTP Server API engine
 *
 * PARAMS
 *   version  [ I] HTTP API version which caller will use
 *   flags    [ I] initialization options which specify parts of API what will be used
 *   reserved [IO] reserved, must be NULL
 *
 * RETURNS
 *   NO_ERROR if function succeeds, or error code if function fails
 *
 */
ULONG WINAPI HttpInitialize( HTTPAPI_VERSION version, ULONG flags, PVOID reserved )
{
    FIXME( "({%d,%d}, 0x%x, %p): stub!\n", version.HttpApiMajorVersion,
           version.HttpApiMinorVersion, flags, reserved );
    return NO_ERROR;
}

/***********************************************************************
 *        HttpTerminate       (HTTPAPI.@)
 *
 * Cleans up HTTP Server API engine resources allocated by HttpInitialize
 *
 * PARAMS
 *   flags    [ I] options which specify parts of API what should be released
 *   reserved [IO] reserved, must be NULL
 *
 * RETURNS
 *   NO_ERROR if function succeeds, or error code if function fails
 *
 */
ULONG WINAPI HttpTerminate( ULONG flags, PVOID reserved )
{
    FIXME( "(0x%x, %p): stub!\n", flags, reserved );
    return NO_ERROR;
}
