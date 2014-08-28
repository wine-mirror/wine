/*
 * AUTHZ Implementation
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(authz);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        default:
            break;
    }

    return TRUE;
}

/***********************************************************************
 *              AuthzInitializeResourceManager (AUTHZ.@)
 */
BOOL WINAPI AuthzInitializeResourceManager(DWORD flags, LPVOID pfnAccessChecker,
    LPVOID pfnComputeDynGroups, LPVOID pfnFreeDynGroups,
    LPCWSTR managerName, LPVOID lpManagerHandle )
{
    FIXME("(0x%X,%p,%p,%p,%s,%p): stub\n", flags, pfnAccessChecker,
        pfnComputeDynGroups, pfnFreeDynGroups,
        debugstr_w(managerName), lpManagerHandle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *              AuthzInstallSecurityEventSource (AUTHZ.@)
 */
BOOL WINAPI AuthzInstallSecurityEventSource(DWORD dwFlags, LPVOID pRegistration)
{
    FIXME("(0x%X,%p): stub\n", dwFlags, pRegistration);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
