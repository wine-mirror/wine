/*
 * Copyright (c) 2015 Andrew Eikum for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/debug.h"

#include "initguid.h"
#include "xaudio2.h"
#include "xaudio2fx.h"
#include "xapo.h"
#include "xapofx.h"

WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        break;
    }
    return TRUE;
}

HRESULT WINAPI XAudio2Create(IXAudio2 **ppxa2, UINT32 flags, XAUDIO2_PROCESSOR proc)
{
    HRESULT hr;
    IXAudio2 *xa2;
    IXAudio27 *xa27;

    /* create XAudio2 2.8 instance */
    hr = CoCreateInstance(&CLSID_XAudio2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXAudio2, (void**)&xa2);
    if(FAILED(hr))
        return hr;

    hr = IXAudio2_QueryInterface(xa2, &IID_IXAudio27, (void**)&xa27);
    if(FAILED(hr)){
        IXAudio2_Release(xa2);
        return hr;
    }

    hr = IXAudio27_Initialize(xa27, flags, proc);
    if(FAILED(hr)){
        IXAudio27_Release(xa27);
        IXAudio2_Release(xa2);
        return hr;
    }

    IXAudio27_Release(xa27);

    *ppxa2 = xa2;

    return S_OK;
}

HRESULT WINAPI CreateAudioVolumeMeter(IUnknown **out)
{
    return CoCreateInstance(&CLSID_AudioVolumeMeter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)out);
}

HRESULT WINAPI CreateAudioReverb(IUnknown **out)
{
    return CoCreateInstance(&CLSID_AudioReverb, NULL, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)out);
}

HRESULT CDECL CreateFX(REFCLSID clsid, IUnknown **out, void *initdata, UINT32 initdata_bytes)
{
    HRESULT hr;
    IUnknown *obj;
    const GUID *class;

    *out = NULL;
    class = clsid;

    if(IsEqualGUID(clsid, &CLSID_FXReverb27) ||
            IsEqualGUID(clsid, &CLSID_FXReverb))
        class = &CLSID_WINE_FXReverb28;

    hr = CoCreateInstance(class, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&obj);
    if(FAILED(hr)){
        WARN("CoCreateInstance failed: %08x\n", hr);
        return hr;
    }

    if(initdata && initdata_bytes > 0){
        IXAPO *xapo;

        hr = IUnknown_QueryInterface(obj, &IID_IXAPO, (void**)&xapo);
        if(SUCCEEDED(hr)){
            hr = IXAPO_Initialize(xapo, initdata, initdata_bytes);

            IXAPO_Release(xapo);

            if(FAILED(hr)){
                WARN("Initialize failed: %08x\n", hr);
                IUnknown_Release(obj);
                return hr;
            }
        }
    }

    *out = obj;

    return S_OK;
}
