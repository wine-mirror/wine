/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "advpub.h"
#include "ole2.h"
#include "dimm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msimtf);

static HINSTANCE msimtf_instance;

/******************************************************************
 *              DllMain (msimtf.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        msimtf_instance = hInstDLL;
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/******************************************************************
 *		DllGetClassObject (msimtf.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    FIXME("(%s %s %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 *              DllCanUnloadNow (msimtf.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("()\n");
    return S_FALSE;
}

#define INF_SET_CLSID(clsid)                  \
    do                                        \
    {                                         \
        static CHAR name[] = "CLSID_" #clsid; \
                                              \
        pse[i].pszName = name;                \
        clsids[i++] = &CLSID_ ## clsid;       \
    } while (0)

static HRESULT register_server(BOOL doregister)
{
    HRESULT hres;
    HMODULE hAdvpack;
    typeof(RegInstallA) *pRegInstall;
    STRTABLEA strtable;
    STRENTRYA pse[1];
    static CLSID const *clsids[34];
    int i = 0;

    static const WCHAR wszAdvpack[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    INF_SET_CLSID(CActiveIMM);

    for(i = 0; i < sizeof(pse)/sizeof(pse[0]); i++) {
        pse[i].pszValue = HeapAlloc(GetProcessHeap(), 0, 39);
        sprintf(pse[i].pszValue, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                clsids[i]->Data1, clsids[i]->Data2, clsids[i]->Data3, clsids[i]->Data4[0],
                clsids[i]->Data4[1], clsids[i]->Data4[2], clsids[i]->Data4[3], clsids[i]->Data4[4],
                clsids[i]->Data4[5], clsids[i]->Data4[6], clsids[i]->Data4[7]);
    }

    strtable.cEntries = sizeof(pse)/sizeof(pse[0]);
    strtable.pse = pse;

    hAdvpack = LoadLibraryW(wszAdvpack);
    pRegInstall = (typeof(RegInstallA)*)GetProcAddress(hAdvpack, "RegInstall");

    hres = pRegInstall(msimtf_instance, doregister ? "RegisterDll" : "UnregisterDll", &strtable);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++)
        HeapFree(GetProcessHeap(), 0, pse[i].pszValue);

    return hres;
}

#undef INF_SET_CLSID

/***********************************************************************
 *          DllRegisterServer (msimtf.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return register_server(TRUE);
}

/***********************************************************************
 *          DllUnregisterServer (msimtf.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return register_server(FALSE);
}
