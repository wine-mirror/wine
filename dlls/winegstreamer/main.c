/* Gstreamer Base Functions
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappbuffer.h>

#include "initguid.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"
#include "advpub.h"
#include "wine/debug.h"

#include "wine/unicode.h"
#include "gst_private.h"
#include "gst_guids.h"

static HINSTANCE hInst = NULL;

WINE_DEFAULT_DEBUG_CHANNEL(gstreamer);

FactoryTemplate const g_Templates[] = {
};

const int g_cTemplates = 0;

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
        hInst = hInstDLL;
    return STRMBASE_DllMain(hInstDLL, fdwReason, lpv);
}

/* GStreamer common functions */

void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt)
{
    if (!pmt)
        return;
    TRACE("\t%s\n\t%s\n\t...\n\t%s\n", debugstr_guid(&pmt->majortype), debugstr_guid(&pmt->subtype), debugstr_guid(&pmt->formattype));
}

#define INF_SET_ID(id)            \
    do                            \
    {                             \
        static CHAR name[] = #id; \
                                  \
        pse[i].pszName = name;    \
        clsids[i++] = &id;        \
    } while (0)

#define INF_SET_CLSID(clsid) INF_SET_ID(CLSID_ ## clsid)

static HRESULT register_server(BOOL do_register)
{
    HRESULT hres;
    HMODULE hAdvpack;
    HRESULT (WINAPI *pRegInstall)(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable);
    STRTABLEA strtable;
    STRENTRYA pse[3];
    static CLSID const *clsids[3];
    unsigned int i = 0;

    static const WCHAR wszAdvpack[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    TRACE("(%x)\n", do_register);

    INF_SET_CLSID(AsyncReader);
    INF_SET_ID(MEDIATYPE_Stream);
    INF_SET_ID(WINESUBTYPE_Gstreamer);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++) {
        pse[i].pszValue = HeapAlloc(GetProcessHeap(),0,39);
        sprintf(pse[i].pszValue, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                clsids[i]->Data1, clsids[i]->Data2, clsids[i]->Data3, clsids[i]->Data4[0],
                clsids[i]->Data4[1], clsids[i]->Data4[2], clsids[i]->Data4[3], clsids[i]->Data4[4],
                clsids[i]->Data4[5], clsids[i]->Data4[6], clsids[i]->Data4[7]);
    }

    strtable.cEntries = sizeof(pse)/sizeof(pse[0]);
    strtable.pse = pse;

    hAdvpack = LoadLibraryW(wszAdvpack);
    pRegInstall = (void *)GetProcAddress(hAdvpack, "RegInstall");

    hres = pRegInstall(hInst, do_register ? "RegisterDll" : "UnregisterDll", &strtable);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++)
        HeapFree(GetProcessHeap(),0,pse[i].pszValue);

    if(FAILED(hres)) {
        ERR("RegInstall failed: %08x\n", hres);
        return hres;
    }

    return hres;
}

#undef INF_SET_CLSID
#undef INF_SET_ID

/***********************************************************************
 *      DllRegisterServer
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = AMovieDllRegisterServer2(TRUE);
    if (SUCCEEDED(hr))
        hr = register_server(TRUE);
    return hr;
}

/***********************************************************************
 *      DllUnregisterServer
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = AMovieDllRegisterServer2(FALSE);
    if (SUCCEEDED(hr))
        hr = register_server(FALSE);
    return hr;
}
