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

#include <stddef.h>
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "d3d9.h"
#include "mfapi.h"
#include "mfidl.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmo);

#include "initguid.h"

DEFINE_MEDIATYPE_GUID(MFVideoFormat_ABGR32, D3DFMT_A8B8G8R8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_P208,MAKEFOURCC('P','2','0','8'));

static HRESULT WINAPI video_processor_factory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **out)
{
    static const GUID CLSID_wg_video_processor = {0xd527607f,0x89cb,0x4e94,{0x95,0x71,0xbc,0xfe,0x62,0x17,0x56,0x13}};
    return CoCreateInstance(&CLSID_wg_video_processor, outer, CLSCTX_INPROC_SERVER, riid, out);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    *out = IsEqualGUID(riid, &IID_IClassFactory) || IsEqualGUID(riid, &IID_IUnknown) ? iface : NULL;
    return *out ? S_OK : E_NOINTERFACE;
}
static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}
static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}
static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    return S_OK;
}

static const IClassFactoryVtbl video_processor_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    video_processor_factory_CreateInstance,
    class_factory_LockServer,
};

static IClassFactory video_processor_factory = {&video_processor_factory_vtbl};

/***********************************************************************
 *              DllGetClassObject (msvproc.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    if (IsEqualGUID(clsid, &CLSID_VideoProcessorMFT))
        return IClassFactory_QueryInterface(&video_processor_factory, riid, out);

    *out = NULL;
    FIXME("Unknown clsid %s.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllRegisterServer (msvproc.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    MFT_REGISTER_TYPE_INFO video_processor_mft_inputs[] =
    {
        {MFMediaType_Video, MFVideoFormat_IYUV},
        {MFMediaType_Video, MFVideoFormat_YV12},
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
        {MFMediaType_Video, MFVideoFormat_ARGB32},
        {MFMediaType_Video, MFVideoFormat_RGB32},
        {MFMediaType_Video, MFVideoFormat_NV11},
        {MFMediaType_Video, MFVideoFormat_AYUV},
        {MFMediaType_Video, MFVideoFormat_UYVY},
        {MFMediaType_Video, MFVideoFormat_P208},
        {MFMediaType_Video, MFVideoFormat_RGB24},
        {MFMediaType_Video, MFVideoFormat_RGB555},
        {MFMediaType_Video, MFVideoFormat_RGB565},
        {MFMediaType_Video, MFVideoFormat_RGB8},
        {MFMediaType_Video, MFVideoFormat_I420},
        {MFMediaType_Video, MFVideoFormat_Y216},
        {MFMediaType_Video, MFVideoFormat_v410},
        {MFMediaType_Video, MFVideoFormat_Y41P},
        {MFMediaType_Video, MFVideoFormat_Y41T},
        {MFMediaType_Video, MFVideoFormat_Y42T},
        {MFMediaType_Video, MFVideoFormat_YVYU},
        {MFMediaType_Video, MFVideoFormat_420O},
    };
    MFT_REGISTER_TYPE_INFO video_processor_mft_outputs[] =
    {
        {MFMediaType_Video, MFVideoFormat_IYUV},
        {MFMediaType_Video, MFVideoFormat_YV12},
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
        {MFMediaType_Video, MFVideoFormat_ARGB32},
        {MFMediaType_Video, MFVideoFormat_RGB32},
        {MFMediaType_Video, MFVideoFormat_NV11},
        {MFMediaType_Video, MFVideoFormat_AYUV},
        {MFMediaType_Video, MFVideoFormat_UYVY},
        {MFMediaType_Video, MFVideoFormat_P208},
        {MFMediaType_Video, MFVideoFormat_RGB24},
        {MFMediaType_Video, MFVideoFormat_RGB555},
        {MFMediaType_Video, MFVideoFormat_RGB565},
        {MFMediaType_Video, MFVideoFormat_RGB8},
        {MFMediaType_Video, MFVideoFormat_I420},
        {MFMediaType_Video, MFVideoFormat_Y216},
        {MFMediaType_Video, MFVideoFormat_v410},
        {MFMediaType_Video, MFVideoFormat_Y41P},
        {MFMediaType_Video, MFVideoFormat_Y41T},
        {MFMediaType_Video, MFVideoFormat_Y42T},
        {MFMediaType_Video, MFVideoFormat_YVYU},
    };

    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = __wine_register_resources()))
        return hr;
    if (FAILED(hr = MFTRegister(CLSID_VideoProcessorMFT, MFT_CATEGORY_VIDEO_PROCESSOR,
            (WCHAR *)L"Microsoft Video Processor MFT", MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(video_processor_mft_inputs), video_processor_mft_inputs,
            ARRAY_SIZE(video_processor_mft_outputs), video_processor_mft_outputs, NULL)))
        return hr;

    return S_OK;
}

/***********************************************************************
 *              DllUnregisterServer (msvproc.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = __wine_unregister_resources()))
        return hr;
    if (FAILED(hr = MFTUnregister(CLSID_VideoProcessorMFT)))
        return hr;

    return S_OK;
}
