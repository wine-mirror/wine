/*
 *    INSENG Implementation
 *
 * Copyright 2006 Mike McCormack
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
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "initguid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inseng);

DEFINE_GUID(CLSID_DLManager, 0xBFC880F1,0x7484,0x11D0,0x83,0x09,0x00,0xAA,0x00,0xB6,0x01,0x5C);
DEFINE_GUID(CLSID_ActiveSetupEng, 0x6e449686,0xc509,0x11cf,0xaa,0xfa,0x00,0xaa,0x00,0xb6,0x01,0x5c );

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("\n");
    return S_FALSE;
}

static HRESULT register_clsid(LPCGUID guid)
{
    static const WCHAR clsid[] =
        {'C','L','S','I','D','\\',0};
    static const WCHAR ips[] =
        {'\\','I','n','p','r','o','c','S','e','r','v','e','r','3','2',0};
    static const WCHAR inseng[] =
        {'i','n','s','e','n','g','.','d','l','l',0};
    static const WCHAR threading_model[] =
        {'T','h','r','e','a','d','i','n','g','M','o','d','e','l',0};
    static const WCHAR apartment[] =
        {'A','p','a','r','t','m','e','n','t',0};
    WCHAR path[80];
    HKEY key = NULL;
    LONG r;

    lstrcpyW(path, clsid);
    StringFromGUID2(guid, &path[6], 80);
    lstrcatW(path, ips);
    r = RegCreateKeyW(HKEY_CLASSES_ROOT, path, &key);
    if (r != ERROR_SUCCESS)
        return E_FAIL;

    RegSetValueExW(key, NULL, 0, REG_SZ, (LPBYTE)inseng, sizeof inseng);
    RegSetValueExW(key, threading_model, 0, REG_SZ, (LPBYTE)apartment, sizeof apartment);
    RegCloseKey(key);

    return S_OK;
}

HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT r;

    r = register_clsid(&CLSID_ActiveSetupEng);
    if (SUCCEEDED(r))
        r = register_clsid(&CLSID_DLManager);

    return r;
}

BOOL WINAPI CheckTrustEx( LPVOID a, LPVOID b, LPVOID c, LPVOID d, LPVOID e )
{
    FIXME("%p %p %p %p %p\n", a, b, c, d, e );
    return TRUE;
}
