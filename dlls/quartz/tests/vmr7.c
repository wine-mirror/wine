/*
 * Video Mixing Renderer 7 unit tests
 *
 * Copyright 2018 Zebediah Figura
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

#define COBJMACROS
#include "dshow.h"
#include "d3d9.h"
#include "vmr9.h"
#include "wine/test.h"

static void test_filter_config(void)
{
    IVMRFilterConfig *config;
    DWORD count, mode;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMRMode_Windowed, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowed);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMRMode_Windowed, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowed);
    todo_wine ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);

    ref = IVMRFilterConfig_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowless);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(mode == VMRMode_Windowless, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowed);
    todo_wine ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);

    ref = IVMRFilterConfig_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Renderless);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(mode == VMRMode_Renderless, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowless);
    todo_wine ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);

    ref = IVMRFilterConfig_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_GetNumberOfStreams(config, &count);
    todo_wine ok(hr == VFW_E_VMR_NOT_IN_MIXER_MODE, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_SetNumberOfStreams(config, 3);
    if (hr != VFW_E_DDRAW_CAPS_NOT_SUITABLE)
    {
        todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IVMRFilterConfig_GetNumberOfStreams(config, &count);
        todo_wine {
            ok(hr == S_OK, "Got hr %#x.\n", hr);
            ok(count == 3, "Got count %u.\n", count);
        }

        hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(mode == VMRMode_Windowed, "Got mode %#x.\n", mode);

        /* Despite MSDN, you can still change the rendering mode after setting the
         * stream count. */
        hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowless);
        todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        todo_wine ok(mode == VMRMode_Windowless, "Got mode %#x.\n", mode);

        hr = IVMRFilterConfig_GetNumberOfStreams(config, &count);
        todo_wine {
            ok(hr == S_OK, "Got hr %#x.\n", hr);
            ok(count == 3, "Got count %u.\n", count);
        }
    }
    else
        skip("Mixing mode is not supported.\n");

    ref = IVMRFilterConfig_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

START_TEST(vmr7)
{
    CoInitialize(NULL);

    test_filter_config();

    CoUninitialize();
}
