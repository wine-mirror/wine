/*
 * Video Mixing Renderer 9 unit tests
 *
 * Copyright 2019 Zebediah Figura
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
    IVMRFilterConfig9 *config;
    DWORD count, mode;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig9, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMRMode_Windowed, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowed);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMR9Mode_Windowed, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowed);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);

    ref = IVMRFilterConfig9_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig9, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowless);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMR9Mode_Windowless, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowed);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);

    ref = IVMRFilterConfig9_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig9, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Renderless);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMR9Mode_Renderless, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowless);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);

    ref = IVMRFilterConfig9_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig9, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig9_GetNumberOfStreams(config, &count);
    todo_wine ok(hr == VFW_E_VMR_NOT_IN_MIXER_MODE, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig9_SetNumberOfStreams(config, 3);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig9_GetNumberOfStreams(config, &count);
    todo_wine {
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(count == 3, "Got count %u.\n", count);
    }

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMR9Mode_Windowed, "Got mode %#x.\n", mode);

    /* Despite MSDN, you can still change the rendering mode after setting the
     * stream count. */
    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowless);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMR9Mode_Windowless, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig9_GetNumberOfStreams(config, &count);
    todo_wine {
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(count == 3, "Got count %u.\n", count);
    }

    ref = IVMRFilterConfig9_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

START_TEST(vmr9)
{
    IBaseFilter *filter;
    HRESULT hr;

    CoInitialize(NULL);

    if (FAILED(hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL,
            CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)&filter)))
    {
        skip("Failed to create VMR9, hr %#x.\n", hr);
        return;
    }
    IBaseFilter_Release(filter);

    test_filter_config();

    CoUninitialize();
}
