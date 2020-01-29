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
static const WCHAR wave_parserW[] =
{'W','a','v','e',' ','P','a','r','s','e','r',0};
static const WCHAR avi_splitterW[] =
{'A','V','I',' ','S','p','l','i','t','t','e','r',0};
static const WCHAR mpeg_splitterW[] =
{'M','P','E','G','-','I',' ','S','t','r','e','a','m',' ','S','p','l','i','t','t','e','r',0};

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

static const AMOVIESETUP_MEDIATYPE wave_parser_sink_type_data[] =
{
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_WAVE},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_AU},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_AIFF},
};

static const AMOVIESETUP_MEDIATYPE wave_parser_source_type_data[] =
{
    {&MEDIATYPE_Audio, &GUID_NULL},
};

static const AMOVIESETUP_PIN wave_parser_pin_data[] =
{
    {
        NULL,
        FALSE, FALSE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        ARRAY_SIZE(wave_parser_sink_type_data),
        wave_parser_sink_type_data,
    },
    {
        NULL,
        FALSE, TRUE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        ARRAY_SIZE(wave_parser_source_type_data),
        wave_parser_source_type_data,
    },
};

static const AMOVIESETUP_FILTER wave_parser_filter_data =
{
    &CLSID_WAVEParser,
    wave_parserW,
    MERIT_UNLIKELY,
    ARRAY_SIZE(wave_parser_pin_data),
    wave_parser_pin_data,
};

static const AMOVIESETUP_MEDIATYPE avi_splitter_sink_type_data[] =
{
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_Avi},
};

static const AMOVIESETUP_PIN avi_splitter_pin_data[] =
{
    {
        NULL,
        FALSE, FALSE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        ARRAY_SIZE(avi_splitter_sink_type_data),
        avi_splitter_sink_type_data,
    },
    {
        NULL,
        FALSE, TRUE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        ARRAY_SIZE(amfMTvideo),
        amfMTvideo,
    },
};

static const AMOVIESETUP_FILTER avi_splitter_filter_data =
{
    &CLSID_AviSplitter,
    avi_splitterW,
    0x5ffff0,
    ARRAY_SIZE(avi_splitter_pin_data),
    avi_splitter_pin_data,
};

static const AMOVIESETUP_MEDIATYPE mpeg_splitter_sink_type_data[] =
{
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Audio},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Video},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1System},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1VideoCD},
};

static const AMOVIESETUP_MEDIATYPE mpeg_splitter_audio_type_data[] =
{
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet},
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload},
};

static const AMOVIESETUP_MEDIATYPE mpeg_splitter_video_type_data[] =
{
    {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet},
    {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload},
};

static const AMOVIESETUP_PIN mpeg_splitter_pin_data[] =
{
    {
        NULL,
        FALSE, FALSE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        ARRAY_SIZE(mpeg_splitter_sink_type_data),
        mpeg_splitter_sink_type_data,
    },
    {
        NULL,
        FALSE, TRUE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        ARRAY_SIZE(mpeg_splitter_audio_type_data),
        mpeg_splitter_audio_type_data,
    },
    {
        NULL,
        FALSE, TRUE, FALSE, FALSE,
        &GUID_NULL,
        NULL,
        ARRAY_SIZE(mpeg_splitter_video_type_data),
        mpeg_splitter_video_type_data,
    },
};

static const AMOVIESETUP_FILTER mpeg_splitter_filter_data =
{
    &CLSID_MPEG1Splitter,
    mpeg_splitterW,
    0x5ffff0,
    ARRAY_SIZE(mpeg_splitter_pin_data),
    mpeg_splitter_pin_data,
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
        wave_parserW,
        &CLSID_WAVEParser,
        wave_parser_create,
        NULL,
        &wave_parser_filter_data,
    },
    {
        avi_splitterW,
        &CLSID_AviSplitter,
        avi_splitter_create,
        NULL,
        &avi_splitter_filter_data,
    },
    {
        mpeg_splitterW,
        &CLSID_MPEG1Splitter,
        mpeg_splitter_create,
        NULL,
        &mpeg_splitter_filter_data,
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
