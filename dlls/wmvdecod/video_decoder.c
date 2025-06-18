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

#include "video_decoder.h"

#include "initguid.h"

DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_VC1S,MAKEFOURCC('V','C','1','S'));
DEFINE_GUID(MEDIASUBTYPE_WMV_Unknown, 0x7ce12ca9,0xbfbf,0x43d9,0x9d,0x00,0x82,0xb8,0xed,0x54,0x31,0x6b);

static HRESULT WINAPI h264_decoder_factory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **out)
{
    static const GUID CLSID_wg_h264_decoder = {0x1f1e273d,0x12c0,0x4b3a,{0x8e,0x9b,0x19,0x33,0xc2,0x49,0x8a,0xea}};
    return CoCreateInstance(&CLSID_wg_h264_decoder, outer, CLSCTX_INPROC_SERVER, riid, out);
}

static const IClassFactoryVtbl h264_decoder_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    h264_decoder_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory h264_decoder_factory = {&h264_decoder_factory_vtbl};

static HRESULT WINAPI wmv_decoder_factory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **out)
{
    static const GUID CLSID_wg_wmv_decoder = {0x62ee5ddb,0x4f52,0x48e2,{0x89,0x28,0x78,0x7b,0x02,0x53,0xa0,0xbc}};
    return CoCreateInstance(&CLSID_wg_wmv_decoder, outer, CLSCTX_INPROC_SERVER, riid, out);
}

static const IClassFactoryVtbl wmv_decoder_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    wmv_decoder_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory wmv_decoder_factory = {&wmv_decoder_factory_vtbl};
