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

#include "gst_private.h"
#include "rpcproxy.h"
#include "wine/debug.h"
#include "wine/unicode.h"

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

const int g_cTemplates = ARRAY_SIZE(g_Templates);

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
    HRESULT hr = STRMBASE_DllCanUnloadNow();

    if (hr == S_OK)
        hr = mfplat_can_unload_now();

    return hr;
}

/***********************************************************************
 *    DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;

    if (FAILED(hr = mfplat_get_class_object(rclsid, riid, ppv)))
        hr = STRMBASE_DllGetClassObject( rclsid, riid, ppv );

    return hr;
}

/* GStreamer common functions */

void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt)
{
    if (!pmt)
        return;
    TRACE("\t%s\n\t%s\n\t...\n\t%s\n", debugstr_guid(&pmt->majortype), debugstr_guid(&pmt->subtype), debugstr_guid(&pmt->formattype));
}

static BOOL CALLBACK init_gstreamer_proc(INIT_ONCE *once, void *param, void **ctx)
{
    BOOL *status = param;
    char argv0[] = "wine";
    char argv1[] = "--gst-disable-registry-fork";
    char *args[3];
    char **argv = args;
    int argc = 2;
    GError *err = NULL;

    TRACE("Initializing...\n");

    argv[0] = argv0;
    argv[1] = argv1;
    argv[2] = NULL;
    *status = gst_init_check(&argc, &argv, &err);
    if (*status)
    {
        HINSTANCE handle;

        TRACE("Initialized, version %s. Built with %d.%d.%d.\n", gst_version_string(),
                GST_VERSION_MAJOR, GST_VERSION_MINOR, GST_VERSION_MICRO);

        /* Unloading glib is a bad idea.. it installs atexit handlers,
         * so never unload the dll after loading */
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                (LPCWSTR)hInst, &handle);
        if (!handle)
            ERR("Failed to pin module %p.\n", hInst);

        start_dispatch_thread();
    }
    else if (err)
    {
        ERR("Failed to initialize gstreamer: %s\n", debugstr_a(err->message));
        g_error_free(err);
    }

    return TRUE;
}

BOOL init_gstreamer(void)
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    static BOOL status;

    InitOnceExecuteOnce(&once, init_gstreamer_proc, &status, NULL);

    return status;
}

/***********************************************************************
 *      DllRegisterServer
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = AMovieDllRegisterServer2(TRUE);
    if (SUCCEEDED(hr))
        hr = __wine_register_resources(hInst);
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
        hr = __wine_unregister_resources(hInst);
    return hr;
}
