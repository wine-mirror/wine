/*
 * COM+ Services
 *
 * Copyright 2013 Alistair Leslie-Hughes
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(comsvcs);

static HINSTANCE COMSVCS_hInstance;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID lpv)
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;    /* prefer native version */
    case DLL_PROCESS_ATTACH:
        COMSVCS_hInstance = hinst;
        DisableThreadLibraryCalls(hinst);
        break;
    }
    return TRUE;
}

/******************************************************************
 * DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    FIXME("(%s,%s,%p) stub\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 * DllCanUnloadNow
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *		DllRegisterServer (comsvcs.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( COMSVCS_hInstance );
}

/***********************************************************************
 *		DllUnregisterServer (comsvcs.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( COMSVCS_hInstance );
}
