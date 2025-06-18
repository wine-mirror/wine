/*
 * Copyright 2023 Ziqing Hui for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static HRESULT WINAPI mp3_sink_factory_CreateMediaSink(IMFSinkClassFactory *iface, IMFByteStream *bytestream,
        IMFMediaType *video_type, IMFMediaType *audio_type, IMFMediaSink **out)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static const IMFSinkClassFactoryVtbl mp3_sink_factory_vtbl =
{
    sink_class_factory_QueryInterface,
    sink_class_factory_AddRef,
    sink_class_factory_Release,
    mp3_sink_factory_CreateMediaSink,
};

static HRESULT WINAPI mp3_sink_class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **out)
{
    static const GUID CLSID_wg_mp3_sink_factory = {0x1f302877,0xaaab,0x40a3,{0xb9,0xe0,0x9f,0x48,0xda,0xf3,0x5b,0xc8}};
    return CoCreateInstance(&CLSID_wg_mp3_sink_factory, outer, CLSCTX_INPROC_SERVER, riid, out);
}

static const IClassFactoryVtbl mp3_sink_class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    mp3_sink_class_factory_CreateInstance,
    class_factory_LockServer,
};

struct sink_class_factory mp3_sink_factory =
{
    {&mp3_sink_class_factory_vtbl},
    {&mp3_sink_factory_vtbl},
};

IClassFactory *mp3_sink_class_factory = &mp3_sink_factory.IClassFactory_iface;

static HRESULT WINAPI mpeg4_sink_factory_CreateMediaSink(IMFSinkClassFactory *iface, IMFByteStream *bytestream,
        IMFMediaType *video_type, IMFMediaType *audio_type, IMFMediaSink **out)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static const IMFSinkClassFactoryVtbl mpeg4_sink_factory_vtbl =
{
    sink_class_factory_QueryInterface,
    sink_class_factory_AddRef,
    sink_class_factory_Release,
    mpeg4_sink_factory_CreateMediaSink,
};

static HRESULT WINAPI mpeg4_sink_class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **out)
{
    static const GUID CLSID_wg_mpeg4_sink_factory = {0x5d5407d9,0xc6ca,0x4770,{0xa7,0xcc,0x27,0xc0,0xcb,0x8a,0x76,0x27}};
    return CoCreateInstance(&CLSID_wg_mpeg4_sink_factory, outer, CLSCTX_INPROC_SERVER, riid, out);
}

static const IClassFactoryVtbl mpeg4_sink_class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    mpeg4_sink_class_factory_CreateInstance,
    class_factory_LockServer,
};

struct sink_class_factory mpeg4_sink_factory =
{
    {&mpeg4_sink_class_factory_vtbl},
    {&mpeg4_sink_factory_vtbl},
};

IClassFactory *mpeg4_sink_class_factory = &mpeg4_sink_factory.IClassFactory_iface;
