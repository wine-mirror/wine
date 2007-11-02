/*
 * Internet Messaging APIs
 *
 * Copyright 2006 Robert Shearman for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"

#include "inetcomm_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        if (!InternetTransport_RegisterClass(hinstDLL))
            return FALSE;
        break;
    case DLL_PROCESS_DETACH:
        InternetTransport_UnregisterClass(hinstDLL);
        break;
    default:
        break;
    }
    return TRUE;
}


/***********************************************************************
 *              DllGetClassObject (INETCOMM.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllCanUnloadNow (INETCOMM.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("\n");
    return S_FALSE;
}
