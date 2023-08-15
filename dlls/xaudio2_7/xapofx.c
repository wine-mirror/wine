/*
 * Copyright (c) 2015 Andrew Eikum for CodeWeavers
 * Copyright (c) 2018 Ethan Lee for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#ifdef XAPOFX1_VER
#include "initguid.h"
#endif /* XAPOFX1_VER */
#include "xaudio_private.h"
#include "xapofx.h"

#include "wine/debug.h"

#if XAUDIO2_VER >= 8 || defined XAPOFX1_VER
WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);
#endif

#if XAUDIO2_VER >= 8
HRESULT CDECL CreateFX(REFCLSID clsid, IUnknown **out, void *initdata, UINT32 initdata_bytes)
{
    HRESULT hr;
    IUnknown *obj;
    const GUID *class = NULL;
    IClassFactory *cf;

    *out = NULL;

    if(IsEqualGUID(clsid, &CLSID_FXReverb27) ||
            IsEqualGUID(clsid, &CLSID_FXReverb))
        class = &CLSID_FXReverb;
    else if(IsEqualGUID(clsid, &CLSID_FXEQ27) ||
            IsEqualGUID(clsid, &CLSID_FXEQ))
        class = &CLSID_FXEQ;
    else if(IsEqualGUID(clsid, &CLSID_FXEcho27) ||
            IsEqualGUID(clsid, &CLSID_FXEcho))
        class = &CLSID_FXEcho;
    else if(IsEqualGUID(clsid, &CLSID_FXMasteringLimiter27) ||
            IsEqualGUID(clsid, &CLSID_FXMasteringLimiter))
        class = &CLSID_FXMasteringLimiter;

    if(class){
        hr = make_xapo_factory(class, &IID_IClassFactory, (void**)&cf);
        if(FAILED(hr))
            return hr;

        hr = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (void**)&obj);
        IClassFactory_Release(cf);
        if(FAILED(hr))
            return hr;
    }else{
        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&obj);
        if(FAILED(hr)){
            WARN("CoCreateInstance failed: %08lx\n", hr);
            return hr;
        }
    }

    if(initdata && initdata_bytes > 0){
        IXAPO *xapo;

        hr = IUnknown_QueryInterface(obj, &IID_IXAPO, (void**)&xapo);
        if(SUCCEEDED(hr)){
            hr = IXAPO_Initialize(xapo, initdata, initdata_bytes);

            IXAPO_Release(xapo);

            if(FAILED(hr)){
                WARN("Initialize failed: %08lx\n", hr);
                IUnknown_Release(obj);
                return hr;
            }
        }
    }

    *out = obj;

    return S_OK;
}
#endif /* XAUDIO2_VER >= 8 */

#ifdef XAPOFX1_VER
HRESULT CDECL CreateFX(REFCLSID clsid, IUnknown **out)
{
    HRESULT hr;
    IUnknown *obj;
    const GUID *class = NULL;
    IClassFactory *cf;

    TRACE("%s %p\n", debugstr_guid(clsid), out);

    *out = NULL;

    if(IsEqualGUID(clsid, &CLSID_FXReverb27) ||
            IsEqualGUID(clsid, &CLSID_FXReverb))
        class = &CLSID_FXReverb;
    else if(IsEqualGUID(clsid, &CLSID_FXEQ27) ||
            IsEqualGUID(clsid, &CLSID_FXEQ))
        class = &CLSID_FXEQ;
    else if(IsEqualGUID(clsid, &CLSID_FXEcho27) ||
            IsEqualGUID(clsid, &CLSID_FXEcho))
        class = &CLSID_FXEcho;
    else if(IsEqualGUID(clsid, &CLSID_FXMasteringLimiter27) ||
            IsEqualGUID(clsid, &CLSID_FXMasteringLimiter))
        class = &CLSID_FXMasteringLimiter;

    if(class){
        hr = make_xapo_factory(class, &IID_IClassFactory, (void**)&cf);
        if(FAILED(hr))
            return hr;

        hr = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (void**)&obj);
        IClassFactory_Release(cf);
        if(FAILED(hr))
            return hr;
    }else{
        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&obj);
        if(FAILED(hr)){
            WARN("CoCreateInstance failed: %08lx\n", hr);
            return hr;
        }
    }

    *out = obj;

    return S_OK;
}
#endif /* XAPOFX1_VER */
