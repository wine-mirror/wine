/*
 * Copyright 2017 Zebediah Figura for Codeweavers
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
#include "objbase.h"
#include "rpcproxy.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsquery);

static HINSTANCE instance;

/***********************************************************************
 *		DllMain
 */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    TRACE("(%p, %u, %p)\n", inst, reason, reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            instance = inst;
            DisableThreadLibraryCalls(inst);
            break;
    }

    return TRUE;
}

/***********************************************************************
 *		DllCanUnloadNow (DSQUERY.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *		DllGetClassObject (DSQUERY.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **out)
{
    TRACE("rclsid %s, riid %s, out %p\n", debugstr_guid(rclsid), debugstr_guid(riid), out);

    FIXME("%s: no class found\n", debugstr_guid(rclsid));
    *out = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllRegisterServer (DSQUERY.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (DSQUERY.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
