/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#include "mfsrcsnk_private.h"

#include "wine/debug.h"
#include "wine/winedmo.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static HRESULT byte_stream_plugin_create(IUnknown *outer, REFIID riid, void **out)
{
    FIXME("outer %p, riid %s, out %p stub!.\n", outer, debugstr_guid(riid), out);
    *out = NULL;
    return E_NOTIMPL;
}

static BOOL use_gst_byte_stream_handler(void)
{
    BOOL result;
    DWORD size = sizeof(result);

    /* @@ Wine registry key: HKCU\Software\Wine\MediaFoundation */
    if (!RegGetValueW( HKEY_CURRENT_USER, L"Software\\Wine\\MediaFoundation", L"DisableGstByteStreamHandler",
                       RRF_RT_REG_DWORD, NULL, &result, &size ))
        return !result;

    return TRUE;
}

static HRESULT WINAPI asf_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("video/x-ms-asf")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl asf_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    asf_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory asf_byte_stream_plugin_factory = {&asf_byte_stream_plugin_factory_vtbl};

static HRESULT WINAPI avi_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("video/avi")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl avi_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    avi_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory avi_byte_stream_plugin_factory = {&avi_byte_stream_plugin_factory_vtbl};

static HRESULT WINAPI mpeg4_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("video/mp4")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl mpeg4_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    mpeg4_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory mpeg4_byte_stream_plugin_factory = {&mpeg4_byte_stream_plugin_factory_vtbl};

static HRESULT WINAPI wav_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("audio/wav")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl wav_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    wav_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory wav_byte_stream_plugin_factory = {&wav_byte_stream_plugin_factory_vtbl};
