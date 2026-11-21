/*
 * D3D11 + D3D12 graphics-device creation smoke test.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Run only against a configured Wine prefix with a real Vulkan device.
 */
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_2.h>
#include <stdio.h>

int main(void)
{
    IDXGIFactory1 *factory = NULL;
    IDXGIAdapter1 *adapter = NULL;
    ID3D11Device *d3d11 = NULL;
    ID3D11DeviceContext *context = NULL;
    ID3D12Device *d3d12 = NULL;
    D3D_FEATURE_LEVEL level = 0;
    HRESULT hr_dxgi, hr_11, hr_12;
    int fail = 0;
    printf("AILinux Wine graphics probe (%u-bit)\n", (unsigned)(sizeof(void *) * 8));

    hr_dxgi = CreateDXGIFactory1(&IID_IDXGIFactory1, (void**)&factory);
    printf("DXGI CreateDXGIFactory1: HRESULT=0x%08lx\n", (unsigned long)(ULONG)hr_dxgi);
    if (SUCCEEDED(hr_dxgi) && factory)
    {
        if (SUCCEEDED(IDXGIFactory1_EnumAdapters1(factory, 0, &adapter)) && adapter)
        {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(IDXGIAdapter1_GetDesc1(adapter, &desc)))
                printf("DXGI primary adapter vendor=%04x device=%04x VRAM=%llu MiB\n",
                        desc.VendorId, desc.DeviceId,
                        (unsigned long long)(desc.DedicatedVideoMemory / (1024 * 1024)));
            IDXGIAdapter1_Release(adapter);
        }
        IDXGIFactory1_Release(factory);
    }
    else ++fail;

    hr_11 = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
                              NULL, 0, D3D11_SDK_VERSION, &d3d11, &level, &context);
    printf("D3D11 hardware device: HRESULT=0x%08lx level=0x%04x\n",
           (unsigned long)(ULONG)hr_11, (unsigned)level);
    if (context) ID3D11DeviceContext_Release(context);
    if (d3d11) ID3D11Device_Release(d3d11);
    if (FAILED(hr_11)) ++fail;

    hr_12 = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0,
                               &IID_ID3D12Device, (void**)&d3d12);
    printf("D3D12 hardware device: HRESULT=0x%08lx\n", (unsigned long)(ULONG)hr_12);
    if (d3d12) ID3D12Device_Release(d3d12);
    if (FAILED(hr_12)) ++fail;

    printf("SUMMARY failures=%d\n", fail);
    return fail ? 1 : 0;
}
