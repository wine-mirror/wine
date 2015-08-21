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

#include <windows.h>

#define COBJMACROS
#include "wine/test.h"
#include "initguid.h"
#include "xaudio2.h"

static BOOL xaudio27;

static HRESULT (WINAPI *pXAudio2Create)(IXAudio2 **, UINT32, XAUDIO2_PROCESSOR) = NULL;

static void test_DeviceDetails(IXAudio27 *xa)
{
    HRESULT hr;
    XAUDIO2_DEVICE_DETAILS dd;
    UINT32 count, i;

    hr = IXAudio27_GetDeviceCount(xa, &count);
    ok(hr == S_OK, "GetDeviceCount failed: %08x\n", hr);

    if(count == 0)
        return;

    for(i = 0; i < count; ++i){
        hr = IXAudio27_GetDeviceDetails(xa, i, &dd);
        ok(hr == S_OK, "GetDeviceDetails failed: %08x\n", hr);

        if(i == 0)
            ok(dd.Role == GlobalDefaultDevice, "Got wrong role for index 0: 0x%x\n", dd.Role);
        else
            ok(dd.Role == NotDefaultDevice, "Got wrong role for index %u: 0x%x\n", i, dd.Role);
    }
}

START_TEST(xaudio2)
{
    HRESULT hr;
    IXAudio27 *xa27 = NULL;
    IXAudio2 *xa = NULL;
    HANDLE xa28dll;

    CoInitialize(NULL);

    xa28dll = LoadLibraryA("xaudio2_8.dll");
    if(xa28dll)
        pXAudio2Create = (void*)GetProcAddress(xa28dll, "XAudio2Create");

    /* XAudio 2.7 (Jun 2010 DirectX) */
    hr = CoCreateInstance(&CLSID_XAudio2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXAudio27, (void**)&xa27);
    if(hr == S_OK){
        xaudio27 = TRUE;

        hr = IXAudio27_Initialize(xa27, 0, XAUDIO2_ANY_PROCESSOR);
        ok(hr == S_OK, "Initialize failed: %08x\n", hr);

        test_DeviceDetails(xa27);

        IXAudio27_Release(xa27);
    }else
        win_skip("XAudio 2.7 not available\n");

    /* XAudio 2.8 (Win8+) */
    if(pXAudio2Create){
        xaudio27 = FALSE;

        hr = pXAudio2Create(&xa, 0, XAUDIO2_DEFAULT_PROCESSOR);
        ok(hr == S_OK, "XAudio2Create failed: %08x\n", hr);

        IXAudio2_Release(xa);
    }else
        win_skip("XAudio 2.8 not available\n");

    CoUninitialize();
}
