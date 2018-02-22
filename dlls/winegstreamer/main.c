/* GStreamer Base Functions
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

#include <gst/gst.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"
#include "advpub.h"
#include "wine/debug.h"

#include "wine/unicode.h"
#include "gst_private.h"
#include "initguid.h"
#include "gst_guids.h"

static HINSTANCE hInst = NULL;

WINE_DEFAULT_DEBUG_CHANNEL(gstreamer);

static const WCHAR wGstreamer_Splitter[] =
{'G','S','t','r','e','a','m','e','r',' ','s','p','l','i','t','t','e','r',' ','f','i','l','t','e','r',0};
static const WCHAR wGstreamer_YUV2RGB[] =
{'G','S','t','r','e','a','m','e','r',' ','Y','U','V',' ','t','o',' ','R','G','B',' ','f','i','l','t','e','r',0};
static const WCHAR wGstreamer_YUV2ARGB[] =
{'G','S','t','r','e','a','m','e','r',' ','Y','U','V',' ','t','o',' ','A','R','G','B',' ','f','i','l','t','e','r',0};
static const WCHAR wGstreamer_Mp3[] =
{'G','S','t','r','e','a','m','e','r',' ','M','p','3',' ','f','i','l','t','e','r',0};
static const WCHAR wGstreamer_AudioConvert[] =
{'G','S','t','r','e','a','m','e','r',' ','A','u','d','i','o','C','o','n','v','e','r','t',' ','f','i','l','t','e','r',0};

static WCHAR wNull[] = {'\0'};

static const AMOVIESETUP_MEDIATYPE amfMTstream[] =
{   { &MEDIATYPE_Stream, &WINESUBTYPE_Gstreamer },
    { &MEDIATYPE_Stream, &MEDIASUBTYPE_NULL },
};

static const AMOVIESETUP_MEDIATYPE amfMTaudio[] =
{   { &MEDIATYPE_Audio, &MEDIASUBTYPE_NULL } };

static const AMOVIESETUP_MEDIATYPE amfMTvideo[] =
{   { &MEDIATYPE_Video, &MEDIASUBTYPE_NULL } };

static const AMOVIESETUP_PIN amfSplitPin[] =
{   {   wNull,
        FALSE, FALSE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        2,
        amfMTstream
    },
    {
        wNull,
        FALSE, TRUE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTaudio
    },
    {
        wNull,
        FALSE, TRUE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTvideo
    }
};

static const AMOVIESETUP_FILTER amfSplitter =
{   &CLSID_Gstreamer_Splitter,
    wGstreamer_Splitter,
    MERIT_PREFERRED,
    3,
    amfSplitPin
};

static const AMOVIESETUP_PIN amfYUVPin[] =
{   {   wNull,
        FALSE, FALSE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTvideo
    },
    {
        wNull,
        FALSE, TRUE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTvideo
    },
};

static const AMOVIESETUP_FILTER amfYUV2RGB =
{   &CLSID_Gstreamer_YUV2RGB,
    wGstreamer_YUV2RGB,
    MERIT_UNLIKELY,
    2,
    amfYUVPin
};

static const AMOVIESETUP_FILTER amfYUV2ARGB =
{   &CLSID_Gstreamer_YUV2ARGB,
    wGstreamer_YUV2ARGB,
    MERIT_UNLIKELY,
    2,
    amfYUVPin
};

AMOVIESETUP_PIN amfMp3Pin[] =
{   {   wNull,
        FALSE, FALSE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTaudio
    },
    {
        wNull,
        FALSE, TRUE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTaudio
    },
};

AMOVIESETUP_FILTER const amfMp3 =
{   &CLSID_Gstreamer_Mp3,
    wGstreamer_Mp3,
    MERIT_NORMAL,
    2,
    amfMp3Pin
};

AMOVIESETUP_PIN amfAudioConvertPin[] =
{   {   wNull,
        FALSE, FALSE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTaudio
    },
    {
        wNull,
        FALSE, TRUE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        1,
        amfMTaudio
    },
};

AMOVIESETUP_FILTER const amfAudioConvert =
{   &CLSID_Gstreamer_AudioConvert,
    wGstreamer_AudioConvert,
    MERIT_UNLIKELY,
    2,
    amfAudioConvertPin
};

FactoryTemplate const g_Templates[] = {
    {
        wGstreamer_Splitter,
        &CLSID_Gstreamer_Splitter,
        Gstreamer_Splitter_create,
        NULL,
        &amfSplitter,
    },
    {
        wGstreamer_YUV2RGB,
        &CLSID_Gstreamer_YUV2RGB,
        Gstreamer_YUV2RGB_create,
        NULL,
        &amfYUV2RGB,
    },
    {
        wGstreamer_YUV2ARGB,
        &CLSID_Gstreamer_YUV2ARGB,
        Gstreamer_YUV2ARGB_create,
        NULL,
        &amfYUV2ARGB,
    },
    {
        wGstreamer_Mp3,
        &CLSID_Gstreamer_Mp3,
        Gstreamer_Mp3_create,
        NULL,
        &amfMp3,
    },
    {
        wGstreamer_AudioConvert,
        &CLSID_Gstreamer_AudioConvert,
        Gstreamer_AudioConvert_create,
        NULL,
        &amfAudioConvert,
    },
};

const int g_cTemplates = sizeof(g_Templates) / sizeof (g_Templates[0]);

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
        hInst = hInstDLL;
    return STRMBASE_DllMain(hInstDLL, fdwReason, lpv);
}

/***********************************************************************
 *    DllCanUnloadNow
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return STRMBASE_DllCanUnloadNow();
}

/***********************************************************************
 *    DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return STRMBASE_DllGetClassObject( rclsid, riid, ppv );
}

/* GStreamer common functions */

void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt)
{
    if (!pmt)
        return;
    TRACE("\t%s\n\t%s\n\t...\n\t%s\n", debugstr_guid(&pmt->majortype), debugstr_guid(&pmt->subtype), debugstr_guid(&pmt->formattype));
}

DWORD Gstreamer_init(void)
{
    static int inited;

    if (!inited) {
        char argv0[] = "wine";
        char argv1[] = "--gst-disable-registry-fork";
        char **argv = HeapAlloc(GetProcessHeap(), 0, sizeof(char *)*3);
        int argc = 2;
        GError *err = NULL;

        TRACE("initializing\n");

        argv[0] = argv0;
        argv[1] = argv1;
        argv[2] = NULL;
        inited = gst_init_check(&argc, &argv, &err);
        HeapFree(GetProcessHeap(), 0, argv);
        if (err) {
            ERR("Failed to initialize gstreamer: %s\n", err->message);
            g_error_free(err);
        }
        if (inited) {
            HINSTANCE newhandle;
            /* Unloading glib is a bad idea.. it installs atexit handlers,
             * so never unload the dll after loading */
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                               (LPCWSTR)hInst, &newhandle);
            if (!newhandle)
                ERR("Could not pin module %p\n", hInst);

            start_dispatch_thread();
        }
    }
    return inited;
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
