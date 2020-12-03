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

#include "gst_private.h"
#include "rpcproxy.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "initguid.h"
#include "gst_guids.h"

static HINSTANCE winegstreamer_instance;
LONG object_locks;

WINE_DEFAULT_DEBUG_CHANNEL(gstreamer);

static const WCHAR wGstreamer_Splitter[] =
{'G','S','t','r','e','a','m','e','r',' ','s','p','l','i','t','t','e','r',' ','f','i','l','t','e','r',0};
static const WCHAR wave_parserW[] =
{'W','a','v','e',' ','P','a','r','s','e','r',0};
static const WCHAR avi_splitterW[] =
{'A','V','I',' ','S','p','l','i','t','t','e','r',0};
static const WCHAR mpeg_splitterW[] =
{'M','P','E','G','-','I',' ','S','t','r','e','a','m',' ','S','p','l','i','t','t','e','r',0};

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        winegstreamer_instance = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE(".\n");

    return object_locks ? S_FALSE : S_OK;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
};

static inline struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        *out = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    IUnknown *unk;
    HRESULT hr;

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    if (outer && !IsEqualGUID(iid, &IID_IUnknown))
        return E_NOINTERFACE;

    *out = NULL;
    if (SUCCEEDED(hr = factory->create_instance(outer, &unk)))
    {
        hr = IUnknown_QueryInterface(unk, iid, out);
        IUnknown_Release(unk);
    }
    return hr;
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("iface %p, lock %d.\n", iface, lock);

    if (lock)
        InterlockedIncrement(&object_locks);
    else
        InterlockedDecrement(&object_locks);
    return S_OK;
}

static const IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer,
};

static struct class_factory avi_splitter_cf = {{&class_factory_vtbl}, avi_splitter_create};
static struct class_factory gstdemux_cf = {{&class_factory_vtbl}, gstdemux_create};
static struct class_factory mpeg_splitter_cf = {{&class_factory_vtbl}, mpeg_splitter_create};
static struct class_factory wave_parser_cf = {{&class_factory_vtbl}, wave_parser_create};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    struct class_factory *factory;
    HRESULT hr;

    TRACE("clsid %s, iid %s, out %p.\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    if (!init_gstreamer())
        return CLASS_E_CLASSNOTAVAILABLE;

    if (SUCCEEDED(hr = mfplat_get_class_object(clsid, iid, out)))
        return hr;

    if (IsEqualGUID(clsid, &CLSID_AviSplitter))
        factory = &avi_splitter_cf;
    else if (IsEqualGUID(clsid, &CLSID_Gstreamer_Splitter))
        factory = &gstdemux_cf;
    else if (IsEqualGUID(clsid, &CLSID_MPEG1Splitter))
        factory = &mpeg_splitter_cf;
    else if (IsEqualGUID(clsid, &CLSID_WAVEParser))
        factory = &wave_parser_cf;
    else
    {
        FIXME("%s not implemented, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(clsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return IClassFactory_QueryInterface(&factory->IClassFactory_iface, iid, out);
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
                (LPCWSTR)winegstreamer_instance, &handle);
        if (!handle)
            ERR("Failed to pin module %p.\n", winegstreamer_instance);

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

static const REGPINTYPES reg_audio_mt = {&MEDIATYPE_Audio, &GUID_NULL};
static const REGPINTYPES reg_stream_mt = {&MEDIATYPE_Stream, &GUID_NULL};
static const REGPINTYPES reg_video_mt = {&MEDIATYPE_Video, &GUID_NULL};

static const REGPINTYPES reg_avi_splitter_sink_mt = {&MEDIATYPE_Stream, &MEDIASUBTYPE_Avi};

static const REGFILTERPINS2 reg_avi_splitter_pins[2] =
{
    {
        .nMediaTypes = 1,
        .lpMediaType = &reg_avi_splitter_sink_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_video_mt,
    },
};

static const REGFILTER2 reg_avi_splitter =
{
    .dwVersion = 2,
    .dwMerit = MERIT_NORMAL,
    .u.s2.cPins2 = 2,
    .u.s2.rgPins2 = reg_avi_splitter_pins,
};

static const REGPINTYPES reg_mpeg_splitter_sink_mts[4] =
{
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Audio},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Video},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1System},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1VideoCD},
};

static const REGPINTYPES reg_mpeg_splitter_audio_mts[2] =
{
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet},
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload},
};

static const REGPINTYPES reg_mpeg_splitter_video_mts[2] =
{
    {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet},
    {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload},
};

static const REGFILTERPINS2 reg_mpeg_splitter_pins[3] =
{
    {
        .nMediaTypes = 4,
        .lpMediaType = reg_mpeg_splitter_sink_mts,
    },
    {
        .dwFlags = REG_PINFLAG_B_ZERO | REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 2,
        .lpMediaType = reg_mpeg_splitter_audio_mts,
    },
    {
        .dwFlags = REG_PINFLAG_B_ZERO | REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 2,
        .lpMediaType = reg_mpeg_splitter_video_mts,
    },
};

static const REGFILTER2 reg_mpeg_splitter =
{
    .dwVersion = 2,
    .dwMerit = MERIT_NORMAL,
    .u.s2.cPins2 = 3,
    .u.s2.rgPins2 = reg_mpeg_splitter_pins,
};

static const REGPINTYPES reg_wave_parser_sink_mts[3] =
{
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_WAVE},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_AU},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_AIFF},
};

static const REGFILTERPINS2 reg_wave_parser_pins[2] =
{
    {
        .nMediaTypes = 3,
        .lpMediaType = reg_wave_parser_sink_mts,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_audio_mt,
    },
};

static const REGFILTER2 reg_wave_parser =
{
    .dwVersion = 2,
    .dwMerit = MERIT_UNLIKELY,
    .u.s2.cPins2 = 2,
    .u.s2.rgPins2 = reg_wave_parser_pins,
};

static const REGFILTERPINS2 reg_gstdemux_pins[3] =
{
    {
        .nMediaTypes = 1,
        .lpMediaType = &reg_stream_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_audio_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_video_mt,
    },
};

static const REGFILTER2 reg_gstdemux =
{
    .dwVersion = 2,
    .dwMerit = MERIT_PREFERRED,
    .u.s2.cPins2 = 3,
    .u.s2.rgPins2 = reg_gstdemux_pins,
};

HRESULT WINAPI DllRegisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE(".\n");

    if (FAILED(hr = __wine_register_resources(winegstreamer_instance)))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    IFilterMapper2_RegisterFilter(mapper, &CLSID_AviSplitter, avi_splitterW, NULL, NULL, NULL, &reg_avi_splitter);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_Gstreamer_Splitter,
            wGstreamer_Splitter, NULL, NULL, NULL, &reg_gstdemux);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_MPEG1Splitter, mpeg_splitterW, NULL, NULL, NULL, &reg_mpeg_splitter);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_WAVEParser, wave_parserW, NULL, NULL, NULL, &reg_wave_parser);

    IFilterMapper2_Release(mapper);

    return mfplat_DllRegisterServer();
}

HRESULT WINAPI DllUnregisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE(".\n");

    if (FAILED(hr = __wine_unregister_resources(winegstreamer_instance)))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_AviSplitter);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_Gstreamer_Splitter);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_MPEG1Splitter);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_WAVEParser);

    IFilterMapper2_Release(mapper);
    return S_OK;
}
