/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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

#include <assert.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#define COBJMACROS
#include "initguid.h"
#include "dxgi1_6.h"
#include "d3d11.h"
#include "d3d12.h"
#include "winternl.h"
#include "ddk/d3dkmthk.h"
#include "wine/heap.h"
#include "wine/test.h"

enum frame_latency
{
    DEFAULT_FRAME_LATENCY =  3,
    MAX_FRAME_LATENCY     = 16,
};

static DEVMODEW registry_mode;

static HRESULT (WINAPI *pCreateDXGIFactory1)(REFIID iid, void **factory);
static HRESULT (WINAPI *pCreateDXGIFactory2)(UINT flags, REFIID iid, void **factory);

static NTSTATUS (WINAPI *pD3DKMTCheckVidPnExclusiveOwnership)(const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc);
static NTSTATUS (WINAPI *pD3DKMTCloseAdapter)(const D3DKMT_CLOSEADAPTER *desc);
static NTSTATUS (WINAPI *pD3DKMTOpenAdapterFromGdiDisplayName)(D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *desc);

static PFN_D3D12_CREATE_DEVICE pD3D12CreateDevice;
static PFN_D3D12_GET_DEBUG_INTERFACE pD3D12GetDebugInterface;

static unsigned int use_adapter_idx;
static BOOL use_warp_adapter;
static BOOL use_mt = TRUE;

static struct test_entry
{
    void (*test)(void);
} *mt_tests;
size_t mt_tests_size, mt_test_count;

static void queue_test(void (*test)(void))
{
    if (mt_test_count >= mt_tests_size)
    {
        mt_tests_size = max(16, mt_tests_size * 2);
        mt_tests = heap_realloc(mt_tests, mt_tests_size * sizeof(*mt_tests));
    }
    mt_tests[mt_test_count++].test = test;
}

static DWORD WINAPI thread_func(void *ctx)
{
    LONG *i = ctx, j;

    while (*i < mt_test_count)
    {
        j = *i;
        if (InterlockedCompareExchange(i, j + 1, j) == j)
            mt_tests[j].test();
    }

    return 0;
}

static void run_queued_tests(void)
{
    unsigned int thread_count, i;
    HANDLE *threads;
    SYSTEM_INFO si;
    LONG test_idx;

    if (!use_mt)
    {
        for (i = 0; i < mt_test_count; ++i)
        {
            mt_tests[i].test();
        }

        return;
    }

    GetSystemInfo(&si);
    thread_count = si.dwNumberOfProcessors;
    threads = heap_calloc(thread_count, sizeof(*threads));
    for (i = 0, test_idx = 0; i < thread_count; ++i)
    {
        threads[i] = CreateThread(NULL, 0, thread_func, &test_idx, 0, NULL);
        ok(!!threads[i], "Failed to create thread %u.\n", i);
    }
    WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
    for (i = 0; i < thread_count; ++i)
    {
        CloseHandle(threads[i]);
    }
    heap_free(threads);
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c, d) check_interface_(__LINE__, a, b, c, d)
static HRESULT check_interface_(unsigned int line, void *iface, REFIID iid,
        BOOL supported, BOOL is_broken)
{
    HRESULT hr, expected_hr, broken_hr;
    IUnknown *unknown = iface, *out;

    if (supported)
    {
        expected_hr = S_OK;
        broken_hr = E_NOINTERFACE;
    }
    else
    {
        expected_hr = E_NOINTERFACE;
        broken_hr = S_OK;
    }

    out = (IUnknown *)0xdeadbeef;
    hr = IUnknown_QueryInterface(unknown, iid, (void **)&out);
    ok_(__FILE__, line)(hr == expected_hr || broken(is_broken && hr == broken_hr),
            "Got hr %#x, expected %#x.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(out);
    else
        ok_(__FILE__, line)(!out, "Got unexpected pointer %p.\n", out);
    return hr;
}

static BOOL is_flip_model(DXGI_SWAP_EFFECT swap_effect)
{
    return swap_effect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
            || swap_effect == DXGI_SWAP_EFFECT_FLIP_DISCARD;
}

static unsigned int check_multisample_quality_levels(IDXGIDevice *dxgi_device,
        DXGI_FORMAT format, unsigned int sample_count)
{
    ID3D10Device *device;
    unsigned int levels;
    HRESULT hr;

    hr = IDXGIDevice_QueryInterface(dxgi_device, &IID_ID3D10Device, (void **)&device);
    ok(hr == S_OK, "Failed to query ID3D10Device, hr %#x.\n", hr);
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, format, sample_count, &levels);
    ok(hr == S_OK, "Failed to check multisample quality levels, hr %#x.\n", hr);
    ID3D10Device_Release(device);

    return levels;
}

#define MODE_DESC_IGNORE_RESOLUTION        0x00000001u
#define MODE_DESC_IGNORE_REFRESH_RATE      0x00000002u
#define MODE_DESC_IGNORE_FORMAT            0x00000004u
#define MODE_DESC_IGNORE_SCANLINE_ORDERING 0x00000008u
#define MODE_DESC_IGNORE_SCALING           0x00000010u

#define MODE_DESC_CHECK_RESOLUTION         (~MODE_DESC_IGNORE_RESOLUTION)
#define MODE_DESC_CHECK_FORMAT             (~MODE_DESC_IGNORE_FORMAT)

#define check_mode_desc(a, b, c) check_mode_desc_(__LINE__, a, b, c)
static void check_mode_desc_(unsigned int line, const DXGI_MODE_DESC *desc,
        const DXGI_MODE_DESC *expected_desc, unsigned int ignore_flags)
{
    if (!(ignore_flags & MODE_DESC_IGNORE_RESOLUTION))
    {
        ok_(__FILE__, line)(desc->Width == expected_desc->Width
                && desc->Height == expected_desc->Height,
                "Got resolution %ux%u, expected %ux%u.\n",
                desc->Width, desc->Height, expected_desc->Width, expected_desc->Height);
    }
    if (!(ignore_flags & MODE_DESC_IGNORE_REFRESH_RATE))
    {
        ok_(__FILE__, line)(desc->RefreshRate.Numerator == expected_desc->RefreshRate.Numerator
                && desc->RefreshRate.Denominator == expected_desc->RefreshRate.Denominator,
                "Got refresh rate %u / %u, expected %u / %u.\n",
                desc->RefreshRate.Numerator, desc->RefreshRate.Denominator,
                expected_desc->RefreshRate.Denominator, expected_desc->RefreshRate.Denominator);
    }
    if (!(ignore_flags & MODE_DESC_IGNORE_FORMAT))
    {
        ok_(__FILE__, line)(desc->Format == expected_desc->Format,
                "Got format %#x, expected %#x.\n", desc->Format, expected_desc->Format);
    }
    if (!(ignore_flags & MODE_DESC_IGNORE_SCANLINE_ORDERING))
    {
        ok_(__FILE__, line)(desc->ScanlineOrdering == expected_desc->ScanlineOrdering,
                "Got scanline ordering %#x, expected %#x.\n",
                desc->ScanlineOrdering, expected_desc->ScanlineOrdering);
    }
    if (!(ignore_flags & MODE_DESC_IGNORE_SCALING))
    {
        ok_(__FILE__, line)(desc->Scaling == expected_desc->Scaling,
                "Got scaling %#x, expected %#x.\n",
                desc->Scaling, expected_desc->Scaling);
    }
}

static BOOL equal_luid(LUID a, LUID b)
{
    return a.LowPart == b.LowPart && a.HighPart == b.HighPart;
}

#define check_adapter_desc(a, b) check_adapter_desc_(__LINE__, a, b)
static void check_adapter_desc_(unsigned int line, const DXGI_ADAPTER_DESC *desc,
        const struct DXGI_ADAPTER_DESC *expected_desc)
{
    ok_(__FILE__, line)(!lstrcmpW(desc->Description, expected_desc->Description),
            "Got description %s, expected %s.\n",
            wine_dbgstr_w(desc->Description), wine_dbgstr_w(expected_desc->Description));
    ok_(__FILE__, line)(desc->VendorId == expected_desc->VendorId,
            "Got vendor id %04x, expected %04x.\n",
            desc->VendorId, expected_desc->VendorId);
    ok_(__FILE__, line)(desc->DeviceId == expected_desc->DeviceId,
            "Got device id %04x, expected %04x.\n",
            desc->DeviceId, expected_desc->DeviceId);
    ok_(__FILE__, line)(desc->SubSysId == expected_desc->SubSysId,
            "Got subsys id %04x, expected %04x.\n",
            desc->SubSysId, expected_desc->SubSysId);
    ok_(__FILE__, line)(desc->Revision == expected_desc->Revision,
            "Got revision %02x, expected %02x.\n",
            desc->Revision, expected_desc->Revision);
    ok_(__FILE__, line)(desc->DedicatedVideoMemory == expected_desc->DedicatedVideoMemory,
            "Got dedicated video memory %lu, expected %lu.\n",
            desc->DedicatedVideoMemory, expected_desc->DedicatedVideoMemory);
    ok_(__FILE__, line)(desc->DedicatedSystemMemory == expected_desc->DedicatedSystemMemory,
            "Got dedicated system memory %lu, expected %lu.\n",
            desc->DedicatedSystemMemory, expected_desc->DedicatedSystemMemory);
    ok_(__FILE__, line)(desc->SharedSystemMemory == expected_desc->SharedSystemMemory,
            "Got shared system memory %lu, expected %lu.\n",
            desc->SharedSystemMemory, expected_desc->SharedSystemMemory);
    ok_(__FILE__, line)(equal_luid(desc->AdapterLuid, expected_desc->AdapterLuid),
            "Got LUID %08x:%08x, expected %08x:%08x.\n",
            desc->AdapterLuid.HighPart, desc->AdapterLuid.LowPart,
            expected_desc->AdapterLuid.HighPart, expected_desc->AdapterLuid.LowPart);
}

#define check_output_desc(a, b) check_output_desc_(__LINE__, a, b)
static void check_output_desc_(unsigned int line, const DXGI_OUTPUT_DESC *desc,
        const struct DXGI_OUTPUT_DESC *expected_desc)
{
    ok_(__FILE__, line)(!lstrcmpW(desc->DeviceName, expected_desc->DeviceName),
            "Got unexpected device name %s, expected %s.\n",
            wine_dbgstr_w(desc->DeviceName), wine_dbgstr_w(expected_desc->DeviceName));
    ok_(__FILE__, line)(EqualRect(&desc->DesktopCoordinates, &expected_desc->DesktopCoordinates),
            "Got unexpected desktop coordinates %s, expected %s.\n",
            wine_dbgstr_rect(&desc->DesktopCoordinates),
            wine_dbgstr_rect(&expected_desc->DesktopCoordinates));
}

#define check_output_equal(a, b) check_output_equal_(__LINE__, a, b)
static void check_output_equal_(unsigned int line, IDXGIOutput *output1, IDXGIOutput *output2)
{
    DXGI_OUTPUT_DESC desc1, desc2;
    HRESULT hr;

    hr = IDXGIOutput_GetDesc(output1, &desc1);
    ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);
    hr = IDXGIOutput_GetDesc(output2, &desc2);
    ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);
    check_output_desc_(line, &desc1, &desc2);
}

static BOOL output_belongs_to_adapter(IDXGIOutput *output, IDXGIAdapter *adapter)
{
    DXGI_OUTPUT_DESC output_desc, desc;
    unsigned int output_idx;
    IDXGIOutput *o;
    HRESULT hr;

    hr = IDXGIOutput_GetDesc(output, &output_desc);
    ok(SUCCEEDED(hr), "Failed to get output desc, hr %#x.\n", hr);

    for (output_idx = 0; IDXGIAdapter_EnumOutputs(adapter, output_idx, &o) != DXGI_ERROR_NOT_FOUND; ++output_idx)
    {
        hr = IDXGIOutput_GetDesc(o, &desc);
        ok(SUCCEEDED(hr), "Failed to get output desc, hr %#x.\n", hr);
        IDXGIOutput_Release(o);

        if (!lstrcmpW(desc.DeviceName, output_desc.DeviceName)
                && EqualRect(&desc.DesktopCoordinates, &output_desc.DesktopCoordinates))
            return TRUE;
    }

    return FALSE;
}

struct fullscreen_state
{
    RECT window_rect;
    RECT client_rect;
    HMONITOR monitor;
    RECT monitor_rect;
};

struct swapchain_fullscreen_state
{
    struct fullscreen_state fullscreen_state;
    BOOL fullscreen;
    IDXGIOutput *target;
};

#define capture_fullscreen_state(a, b) capture_fullscreen_state_(__LINE__, a, b)
static void capture_fullscreen_state_(unsigned int line, struct fullscreen_state *state, HWND window)
{
    MONITORINFOEXW monitor_info;
    BOOL ret;

    ret = GetWindowRect(window, &state->window_rect);
    ok_(__FILE__, line)(ret, "GetWindowRect failed.\n");
    ret = GetClientRect(window, &state->client_rect);
    ok_(__FILE__, line)(ret, "GetClientRect failed.\n");

    state->monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
    ok_(__FILE__, line)(!!state->monitor, "Failed to get monitor from window.\n");

    monitor_info.cbSize = sizeof(monitor_info);
    ret = GetMonitorInfoW(state->monitor, (MONITORINFO *)&monitor_info);
    ok_(__FILE__, line)(ret, "Failed to get monitor info.\n");
    state->monitor_rect = monitor_info.rcMonitor;
}

#define check_fullscreen_state(a, b) check_fullscreen_state_(__LINE__, a, b)
static void check_fullscreen_state_(unsigned int line, const struct fullscreen_state *state,
        const struct fullscreen_state *expected_state)
{
    ok_(__FILE__, line)(EqualRect(&state->window_rect, &expected_state->window_rect),
            "Got window rect %s, expected %s.\n",
            wine_dbgstr_rect(&state->window_rect), wine_dbgstr_rect(&expected_state->window_rect));
    ok_(__FILE__, line)(EqualRect(&state->client_rect, &expected_state->client_rect),
            "Got client rect %s, expected %s.\n",
            wine_dbgstr_rect(&state->client_rect), wine_dbgstr_rect(&expected_state->client_rect));
    ok_(__FILE__, line)(state->monitor == expected_state->monitor,
            "Got monitor %p, expected %p.\n",
            state->monitor, expected_state->monitor);
    ok_(__FILE__, line)(EqualRect(&state->monitor_rect, &expected_state->monitor_rect),
            "Got monitor rect %s, expected %s.\n",
            wine_dbgstr_rect(&state->monitor_rect), wine_dbgstr_rect(&expected_state->monitor_rect));
}

#define check_window_fullscreen_state(a, b) check_window_fullscreen_state_(__LINE__, a, b)
static void check_window_fullscreen_state_(unsigned int line, HWND window,
        const struct fullscreen_state *expected_state)
{
    struct fullscreen_state current_state;
    capture_fullscreen_state_(line, &current_state, window);
    check_fullscreen_state_(line, &current_state, expected_state);
}

#define check_swapchain_fullscreen_state(a, b) check_swapchain_fullscreen_state_(__LINE__, a, b)
static void check_swapchain_fullscreen_state_(unsigned int line, IDXGISwapChain *swapchain,
        const struct swapchain_fullscreen_state *expected_state)
{
    IDXGIOutput *containing_output, *target;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    BOOL fullscreen;
    HRESULT hr;

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get swapchain desc, hr %#x.\n", hr);
    check_window_fullscreen_state_(line, swapchain_desc.OutputWindow, &expected_state->fullscreen_state);

    ok_(__FILE__, line)(swapchain_desc.Windowed == !expected_state->fullscreen,
            "Got windowed %#x, expected %#x.\n",
            swapchain_desc.Windowed, !expected_state->fullscreen);

    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &target);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get fullscreen state, hr %#x.\n", hr);
    ok_(__FILE__, line)(fullscreen == expected_state->fullscreen, "Got fullscreen %#x, expected %#x.\n",
            fullscreen, expected_state->fullscreen);

    if (!swapchain_desc.Windowed && expected_state->fullscreen)
    {
        IDXGIAdapter *adapter;

        hr = IDXGISwapChain_GetContainingOutput(swapchain, &containing_output);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get containing output, hr %#x.\n", hr);

        hr = IDXGIOutput_GetParent(containing_output, &IID_IDXGIAdapter, (void **)&adapter);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get parent, hr %#x.\n", hr);

        check_output_equal_(line, target, expected_state->target);
        ok_(__FILE__, line)(target == containing_output, "Got target %p, expected %p.\n",
                target, containing_output);
        ok_(__FILE__, line)(output_belongs_to_adapter(target, adapter),
                "Output %p doesn't belong to adapter %p.\n",
                target, adapter);

        IDXGIOutput_Release(target);
        IDXGIOutput_Release(containing_output);
        IDXGIAdapter_Release(adapter);
    }
    else
    {
        ok_(__FILE__, line)(!target, "Got unexpected target %p.\n", target);
    }
}

#define compute_expected_swapchain_fullscreen_state_after_fullscreen_change(a, b, c, d, e, f) \
        compute_expected_swapchain_fullscreen_state_after_fullscreen_change_(__LINE__, a, b, c, d, e, f)
static void compute_expected_swapchain_fullscreen_state_after_fullscreen_change_(unsigned int line,
        struct swapchain_fullscreen_state *state, const DXGI_SWAP_CHAIN_DESC *swapchain_desc,
        const RECT *old_monitor_rect, unsigned int new_width, unsigned int new_height, IDXGIOutput *target)
{
    if (!new_width && !new_height)
    {
        RECT client_rect;
        GetClientRect(swapchain_desc->OutputWindow, &client_rect);
        new_width = client_rect.right - client_rect.left;
        new_height = client_rect.bottom - client_rect.top;
    }

    if (target)
    {
        DXGI_MODE_DESC mode_desc = swapchain_desc->BufferDesc;
        HRESULT hr;

        mode_desc.Width = new_width;
        mode_desc.Height = new_height;
        hr = IDXGIOutput_FindClosestMatchingMode(target, &mode_desc, &mode_desc, NULL);
        ok_(__FILE__, line)(SUCCEEDED(hr), "FindClosestMatchingMode failed, hr %#x.\n", hr);
        new_width = mode_desc.Width;
        new_height = mode_desc.Height;
    }

    state->fullscreen = TRUE;
    if (swapchain_desc->Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
    {
        unsigned int new_x = (old_monitor_rect->left >= 0)
                ? old_monitor_rect->left : old_monitor_rect->right - new_width;
        unsigned new_y = (old_monitor_rect->top >= 0)
                ? old_monitor_rect->top : old_monitor_rect->bottom - new_height;
        RECT new_monitor_rect = {0, 0, new_width, new_height};
        OffsetRect(&new_monitor_rect, new_x, new_y);

        SetRect(&state->fullscreen_state.client_rect, 0, 0, new_width, new_height);
        state->fullscreen_state.monitor_rect = new_monitor_rect;
        state->fullscreen_state.window_rect = new_monitor_rect;

        if (target)
            state->target = target;
    }
    else
    {
        state->fullscreen_state.window_rect = *old_monitor_rect;
        SetRect(&state->fullscreen_state.client_rect, 0, 0,
                old_monitor_rect->right - old_monitor_rect->left,
                old_monitor_rect->bottom - old_monitor_rect->top);
    }
}

#define wait_fullscreen_state(a, b, c) wait_fullscreen_state_(__LINE__, a, b, c)
static void wait_fullscreen_state_(unsigned int line, IDXGISwapChain *swapchain, BOOL expected, BOOL todo)
{
    static const unsigned int wait_timeout = 2000;
    static const unsigned int wait_step = 100;
    unsigned int total_time = 0;
    HRESULT hr;
    BOOL state;

    while (total_time < wait_timeout)
    {
        state = !expected;
        if (FAILED(hr = IDXGISwapChain_GetFullscreenState(swapchain, &state, NULL)))
            break;
        if (state == expected)
            break;
        Sleep(wait_step);
        total_time += wait_step;
    }
    ok_(__FILE__, line)(hr == S_OK, "Failed to get fullscreen state, hr %#x.\n", hr);
    todo_wine_if(todo) ok_(__FILE__, line)(state == expected,
            "Got unexpected state %#x, expected %#x.\n", state, expected);
}

#define wait_vidpn_exclusive_ownership(a, b, c) wait_vidpn_exclusive_ownership_(__LINE__, a, b, c)
static void wait_vidpn_exclusive_ownership_(unsigned int line,
        const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc, NTSTATUS expected, BOOL todo)
{
    static const unsigned int wait_timeout = 2000;
    static const unsigned int wait_step = 100;
    unsigned int total_time = 0;
    NTSTATUS status;

    while (total_time < wait_timeout)
    {
        status = pD3DKMTCheckVidPnExclusiveOwnership(desc);
        if (status == expected)
            break;
        Sleep(wait_step);
        total_time += wait_step;
    }
    todo_wine_if(todo) ok_(__FILE__, line)(status == expected,
            "Got unexpected status %#x, expected %#x.\n", status, expected);
}

static IDXGIAdapter *create_adapter(void)
{
    IDXGIFactory4 *factory4;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    HRESULT hr;

    if (!use_warp_adapter && !use_adapter_idx)
        return NULL;

    if (FAILED(hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory)))
    {
        trace("Failed to create IDXGIFactory, hr %#x.\n", hr);
        return NULL;
    }

    adapter = NULL;
    if (use_warp_adapter)
    {
        if (SUCCEEDED(hr = IDXGIFactory_QueryInterface(factory, &IID_IDXGIFactory4, (void **)&factory4)))
        {
            hr = IDXGIFactory4_EnumWarpAdapter(factory4, &IID_IDXGIAdapter, (void **)&adapter);
            IDXGIFactory4_Release(factory4);
        }
        else
        {
            trace("Failed to get IDXGIFactory4, hr %#x.\n", hr);
        }
    }
    else
    {
        hr = IDXGIFactory_EnumAdapters(factory, use_adapter_idx, &adapter);
    }
    IDXGIFactory_Release(factory);
    if (FAILED(hr))
        trace("Failed to get adapter, hr %#x.\n", hr);
    return adapter;
}

static IDXGIDevice *create_device(unsigned int flags)
{
    IDXGIDevice *dxgi_device;
    ID3D10Device1 *device;
    IDXGIAdapter *adapter;
    HRESULT hr;

    adapter = create_adapter();
    hr = D3D10CreateDevice1(adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL,
            flags, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device);
    if (adapter)
        IDXGIAdapter_Release(adapter);
    if (SUCCEEDED(hr))
        goto success;

    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_WARP, NULL,
            flags, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        goto success;
    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL,
            flags, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        goto success;

    return NULL;

success:
    hr = ID3D10Device1_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Created device does not implement IDXGIDevice\n");
    ID3D10Device1_Release(device);

    return dxgi_device;
}

static ID3D12Device *create_d3d12_device(void)
{
    IDXGIAdapter *adapter;
    ID3D12Device *device;
    HRESULT hr;

    if (!pD3D12CreateDevice)
        return NULL;

    adapter = create_adapter();
    hr = pD3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&device);
    if (adapter)
        IDXGIAdapter_Release(adapter);
    if (FAILED(hr))
        return NULL;

    return device;
}

static ID3D12CommandQueue *create_d3d12_direct_queue(ID3D12Device *device)
{
    D3D12_COMMAND_QUEUE_DESC command_queue_desc;
    ID3D12CommandQueue *queue;
    HRESULT hr;

    command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    command_queue_desc.NodeMask = 0;
    hr = ID3D12Device_CreateCommandQueue(device, &command_queue_desc,
            &IID_ID3D12CommandQueue, (void **)&queue);
    ok(hr == S_OK, "Failed to create command queue, hr %#x.\n", hr);
    return queue;
}

static HRESULT wait_for_fence(ID3D12Fence *fence, UINT64 value)
{
    HANDLE event;
    HRESULT hr;
    DWORD ret;

    if (ID3D12Fence_GetCompletedValue(fence) >= value)
        return S_OK;

    if (!(event = CreateEventA(NULL, FALSE, FALSE, NULL)))
        return E_FAIL;

    if (FAILED(hr = ID3D12Fence_SetEventOnCompletion(fence, value, event)))
    {
        CloseHandle(event);
        return hr;
    }

    ret = WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
    return ret == WAIT_OBJECT_0 ? S_OK : E_FAIL;
}

#define wait_queue_idle(a, b) wait_queue_idle_(__LINE__, a, b)
static void wait_queue_idle_(unsigned int line, ID3D12Device *device, ID3D12CommandQueue *queue)
{
    ID3D12Fence *fence;
    HRESULT hr;

    hr = ID3D12Device_CreateFence(device, 0, D3D12_FENCE_FLAG_NONE,
            &IID_ID3D12Fence, (void **)&fence);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create fence, hr %#x.\n", hr);

    hr = ID3D12CommandQueue_Signal(queue, fence, 1);
    ok_(__FILE__, line)(hr == S_OK, "Failed to signal fence, hr %#x.\n", hr);
    hr = wait_for_fence(fence, 1);
    ok_(__FILE__, line)(hr == S_OK, "Failed to wait for fence, hr %#x.\n", hr);

    ID3D12Fence_Release(fence);
}

#define wait_device_idle(a) wait_device_idle_(__LINE__, a)
static void wait_device_idle_(unsigned int line, IUnknown *device)
{
    ID3D12Device *d3d12_device;
    ID3D12CommandQueue *queue;
    HRESULT hr;

    hr = IUnknown_QueryInterface(device, &IID_ID3D12CommandQueue, (void **)&queue);
    if (hr != S_OK)
        return;

    hr = ID3D12CommandQueue_GetDevice(queue, &IID_ID3D12Device, (void **)&d3d12_device);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get d3d12 device, hr %#x.\n", hr);

    wait_queue_idle_(line, d3d12_device, queue);

    ID3D12CommandQueue_Release(queue);
    ID3D12Device_Release(d3d12_device);
}

#define get_factory(a, b, c) get_factory_(__LINE__, a, b, c)
static void get_factory_(unsigned int line, IUnknown *device, BOOL is_d3d12, IDXGIFactory **factory)
{
    IDXGIDevice *dxgi_device;
    IDXGIAdapter *adapter;
    HRESULT hr;

    if (is_d3d12)
    {
        hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)factory);
        ok_(__FILE__, line)(hr == S_OK, "Failed to create factory, hr %#x.\n", hr);
    }
    else
    {
        dxgi_device = (IDXGIDevice *)device;
        hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get adapter, hr %#x.\n", hr);
        hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)factory);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get parent, hr %#x.\n", hr);
        IDXGIAdapter_Release(adapter);
    }
}

#define get_adapter(a, b) get_adapter_(__LINE__, a, b)
static IDXGIAdapter *get_adapter_(unsigned int line, IUnknown *device, BOOL is_d3d12)
{
    IDXGIAdapter *adapter = NULL;
    ID3D12Device *d3d12_device;
    IDXGIFactory4 *factory4;
    IDXGIFactory *factory;
    HRESULT hr;
    LUID luid;

    if (is_d3d12)
    {
        get_factory_(line, device, is_d3d12, &factory);
        hr = ID3D12CommandQueue_GetDevice((ID3D12CommandQueue *)device, &IID_ID3D12Device, (void **)&d3d12_device);
        ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        luid = ID3D12Device_GetAdapterLuid(d3d12_device);
        ID3D12Device_Release(d3d12_device);
        hr = IDXGIFactory_QueryInterface(factory, &IID_IDXGIFactory4, (void **)&factory4);
        ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDXGIFactory4_EnumAdapterByLuid(factory4, luid, &IID_IDXGIAdapter, (void **)&adapter);
        ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        IDXGIFactory4_Release(factory4);
        IDXGIFactory_Release(factory);
    }
    else
    {
        hr = IDXGIDevice_GetAdapter((IDXGIDevice *)device, &adapter);
        ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    }

    return adapter;
}

static void test_adapter_desc(void)
{
    DXGI_ADAPTER_DESC1 desc1;
    IDXGIAdapter1 *adapter1;
    DXGI_ADAPTER_DESC desc;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_GetDesc(adapter, NULL);
    ok(hr == E_INVALIDARG, "GetDesc returned %#x, expected %#x.\n",
            hr, E_INVALIDARG);

    hr = IDXGIAdapter_GetDesc(adapter, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

    trace("%s.\n", wine_dbgstr_w(desc.Description));
    trace("%04x: %04x:%04x (rev %02x).\n",
            desc.SubSysId, desc.VendorId, desc.DeviceId, desc.Revision);
    trace("Dedicated video memory: %lu (%lu MB).\n",
            desc.DedicatedVideoMemory, desc.DedicatedVideoMemory / (1024 * 1024));
    trace("Dedicated system memory: %lu (%lu MB).\n",
            desc.DedicatedSystemMemory, desc.DedicatedSystemMemory / (1024 * 1024));
    trace("Shared system memory: %lu (%lu MB).\n",
            desc.SharedSystemMemory, desc.SharedSystemMemory / (1024 * 1024));
    trace("LUID: %08x:%08x.\n", desc.AdapterLuid.HighPart, desc.AdapterLuid.LowPart);

    hr = IDXGIAdapter_QueryInterface(adapter, &IID_IDXGIAdapter1, (void **)&adapter1);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE), "Got unexpected hr %#x.\n", hr);
    if (hr == E_NOINTERFACE)
        goto done;

    hr = IDXGIAdapter1_GetDesc1(adapter1, &desc1);
    ok(SUCCEEDED(hr), "GetDesc1 failed, hr %#x.\n", hr);

    ok(!lstrcmpW(desc.Description, desc1.Description),
            "Got unexpected description %s.\n", wine_dbgstr_w(desc1.Description));
    ok(desc1.VendorId == desc.VendorId, "Got unexpected vendor ID %04x.\n", desc1.VendorId);
    ok(desc1.DeviceId == desc.DeviceId, "Got unexpected device ID %04x.\n", desc1.DeviceId);
    ok(desc1.SubSysId == desc.SubSysId, "Got unexpected sub system ID %04x.\n", desc1.SubSysId);
    ok(desc1.Revision == desc.Revision, "Got unexpected revision %02x.\n", desc1.Revision);
    ok(desc1.DedicatedVideoMemory == desc.DedicatedVideoMemory,
            "Got unexpected dedicated video memory %lu.\n", desc1.DedicatedVideoMemory);
    ok(desc1.DedicatedSystemMemory == desc.DedicatedSystemMemory,
            "Got unexpected dedicated system memory %lu.\n", desc1.DedicatedSystemMemory);
    ok(desc1.SharedSystemMemory == desc.SharedSystemMemory,
            "Got unexpected shared system memory %lu.\n", desc1.SharedSystemMemory);
    ok(equal_luid(desc1.AdapterLuid, desc.AdapterLuid),
            "Got unexpected adapter LUID %08x:%08x.\n", desc1.AdapterLuid.HighPart, desc1.AdapterLuid.LowPart);
    trace("Flags: %08x.\n", desc1.Flags);

    IDXGIAdapter1_Release(adapter1);

done:
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_adapter_luid(void)
{
    DXGI_ADAPTER_DESC device_adapter_desc, desc, desc2;
    static const LUID luid = {0xdeadbeef, 0xdeadbeef};
    IDXGIAdapter *adapter, *adapter2;
    unsigned int found_adapter_count;
    unsigned int adapter_index;
    BOOL is_null_luid_adapter;
    IDXGIFactory4 *factory4;
    IDXGIFactory *factory;
    BOOL have_unique_luid;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Failed to get adapter, hr %#x.\n", hr);
    hr = IDXGIAdapter_GetDesc(adapter, &device_adapter_desc);
    ok(hr == S_OK, "Failed to get adapter desc, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    is_null_luid_adapter = !device_adapter_desc.AdapterLuid.LowPart
            && !device_adapter_desc.SubSysId && !device_adapter_desc.Revision
            && !device_adapter_desc.VendorId && !device_adapter_desc.DeviceId;

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Failed to create DXGI factory, hr %#x.\n", hr);

    hr = IDXGIFactory_QueryInterface(factory, &IID_IDXGIFactory4, (void **)&factory4);
    ok(hr == S_OK || hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);

    have_unique_luid = TRUE;
    found_adapter_count = 0;
    adapter_index = 0;
    while ((hr = IDXGIFactory_EnumAdapters(factory, adapter_index, &adapter)) == S_OK)
    {
        hr = IDXGIAdapter_GetDesc(adapter, &desc);
        ok(hr == S_OK, "Failed to get adapter desc, hr %#x.\n", hr);

        if (equal_luid(desc.AdapterLuid, device_adapter_desc.AdapterLuid))
        {
            check_adapter_desc(&desc, &device_adapter_desc);
            ++found_adapter_count;
        }

        if (equal_luid(desc.AdapterLuid, luid))
            have_unique_luid = FALSE;

        if (factory4)
        {
            hr = IDXGIFactory4_EnumAdapterByLuid(factory4, desc.AdapterLuid,
                    &IID_IDXGIAdapter, (void **)&adapter2);
            ok(hr == S_OK, "Failed to enum adapter by LUID, hr %#x.\n", hr);
            hr = IDXGIAdapter_GetDesc(adapter2, &desc2);
            ok(hr == S_OK, "Failed to get adapter desc, hr %#x.\n", hr);
            check_adapter_desc(&desc2, &desc);
            ok(adapter2 != adapter, "Expected to get new instance of IDXGIAdapter.\n");
            refcount = IDXGIAdapter_Release(adapter2);
            ok(!refcount, "Adapter has %u references left.\n", refcount);
        }

        refcount = IDXGIAdapter_Release(adapter);
        ok(!refcount, "Adapter has %u references left.\n", refcount);

        ++adapter_index;
    }
    ok(hr == DXGI_ERROR_NOT_FOUND, "Got unexpected hr %#x.\n", hr);

    /* Older versions of WARP aren't enumerated by IDXGIFactory_EnumAdapters(). */
    todo_wine ok(found_adapter_count == 1 || broken(is_null_luid_adapter),
            "Found %u adapters for LUID %08x:%08x.\n",
            found_adapter_count, device_adapter_desc.AdapterLuid.HighPart,
            device_adapter_desc.AdapterLuid.LowPart);

    if (factory4)
        IDXGIFactory4_Release(factory4);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);

    if (!pCreateDXGIFactory2
            || FAILED(hr = pCreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory4)))
    {
        skip("DXGI 1.4 is not available.\n");
        return;
    }

    hr = IDXGIFactory4_EnumAdapterByLuid(factory4, device_adapter_desc.AdapterLuid,
            &IID_IDXGIAdapter, (void **)&adapter);
    todo_wine ok(hr == S_OK, "Failed to enum adapter by LUID, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDXGIAdapter_GetDesc(adapter, &desc);
        ok(hr == S_OK, "Failed to get adapter desc, hr %#x.\n", hr);
        check_adapter_desc(&desc, &device_adapter_desc);
        refcount = IDXGIAdapter_Release(adapter);
        ok(!refcount, "Adapter has %u references left.\n", refcount);
    }

    if (have_unique_luid)
    {
        hr = IDXGIFactory4_EnumAdapterByLuid(factory4, luid, &IID_IDXGIAdapter, (void **)&adapter);
        ok(hr == DXGI_ERROR_NOT_FOUND, "Got unexpected hr %#x.\n", hr);
    }
    else
    {
        skip("Our LUID is not unique.\n");
    }

    refcount = IDXGIFactory4_Release(factory4);
    ok(!refcount, "Factory has %u references left.\n", refcount);
}

static void test_query_video_memory_info(void)
{
    DXGI_QUERY_VIDEO_MEMORY_INFO memory_info;
    IDXGIAdapter3 *adapter3;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Failed to get adapter, hr %#x.\n", hr);
    hr = IDXGIAdapter_QueryInterface(adapter, &IID_IDXGIAdapter3, (void **)&adapter3);
    ok(hr == S_OK || hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    if (hr == E_NOINTERFACE)
        goto done;

    hr = IDXGIAdapter3_QueryVideoMemoryInfo(adapter3, 0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memory_info);
    ok(hr == S_OK, "Failed to query video memory info, hr %#x.\n", hr);
    ok(memory_info.Budget >= memory_info.AvailableForReservation,
            "Available for reservation 0x%s is greater than budget 0x%s.\n",
            wine_dbgstr_longlong(memory_info.AvailableForReservation),
            wine_dbgstr_longlong(memory_info.Budget));
    ok(!memory_info.CurrentReservation, "Got unexpected current reservation 0x%s.\n",
            wine_dbgstr_longlong(memory_info.CurrentReservation));

    hr = IDXGIAdapter3_QueryVideoMemoryInfo(adapter3, 0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &memory_info);
    ok(hr == S_OK, "Failed to query video memory info, hr %#x.\n", hr);
    ok(memory_info.Budget >= memory_info.AvailableForReservation,
            "Available for reservation 0x%s is greater than budget 0x%s.\n",
            wine_dbgstr_longlong(memory_info.AvailableForReservation),
            wine_dbgstr_longlong(memory_info.Budget));
    ok(!memory_info.CurrentReservation, "Got unexpected current reservation 0x%s.\n",
            wine_dbgstr_longlong(memory_info.CurrentReservation));

    hr = IDXGIAdapter3_QueryVideoMemoryInfo(adapter3, 0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL + 1, &memory_info);
    ok(hr == E_INVALIDARG, "Failed to query video memory info, hr %#x.\n", hr);

    IDXGIAdapter3_Release(adapter3);

done:
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_check_interface_support(void)
{
    LARGE_INTEGER driver_version;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    IUnknown *iface;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_IDXGIDevice, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_IDXGIDevice, &driver_version);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device, &driver_version);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    trace("UMD version: %u.%u.%u.%u.\n",
            HIWORD(U(driver_version).HighPart), LOWORD(U(driver_version).HighPart),
            HIWORD(U(driver_version).LowPart), LOWORD(U(driver_version).LowPart));

    hr = IDXGIDevice_QueryInterface(device, &IID_ID3D10Device1, (void **)&iface);
    if (SUCCEEDED(hr))
    {
        IUnknown_Release(iface);
        hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device1, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device1, &driver_version);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    }
    else
    {
        win_skip("D3D10.1 is not supported.\n");
    }

    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D11Device, NULL);
    ok(hr == DXGI_ERROR_UNSUPPORTED, "Got unexpected hr %#x.\n", hr);
    driver_version.HighPart = driver_version.LowPart = 0xdeadbeef;
    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D11Device, &driver_version);
    ok(hr == DXGI_ERROR_UNSUPPORTED, "Got unexpected hr %#x.\n", hr);
    ok(driver_version.HighPart == 0xdeadbeef, "Got unexpected driver version %#x.\n", driver_version.HighPart);
    ok(driver_version.LowPart == 0xdeadbeef, "Got unexpected driver version %#x.\n", driver_version.LowPart);

    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_surface(void)
{
    DXGI_SURFACE_DESC desc;
    IDXGISurface *surface;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    desc.Width = 512;
    desc.Height = 512;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    hr = IDXGIDevice_CreateSurface(device, &desc, 1, DXGI_USAGE_RENDER_TARGET_OUTPUT, NULL, &surface);
    ok(SUCCEEDED(hr), "Failed to create a dxgi surface, hr %#x\n", hr);

    check_interface(surface, &IID_ID3D10Texture2D, TRUE, FALSE);
    /* Not available on all Windows versions. */
    check_interface(surface, &IID_ID3D11Texture2D, TRUE, TRUE);
    /* Not available on all Windows versions. */
    check_interface(surface, &IID_IDXGISurface1, TRUE, TRUE);

    IDXGISurface_Release(surface);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_parents(void)
{
    DXGI_SURFACE_DESC surface_desc;
    IDXGISurface *surface;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    IDXGIOutput *output;
    IUnknown *parent;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    surface_desc.Width = 512;
    surface_desc.Height = 512;
    surface_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    surface_desc.SampleDesc.Count = 1;
    surface_desc.SampleDesc.Quality = 0;

    hr = IDXGIDevice_CreateSurface(device, &surface_desc, 1, DXGI_USAGE_RENDER_TARGET_OUTPUT, NULL, &surface);
    ok(SUCCEEDED(hr), "Failed to create a dxgi surface, hr %#x\n", hr);

    hr = IDXGISurface_GetParent(surface, &IID_IDXGIDevice, (void **)&parent);
    IDXGISurface_Release(surface);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);
    ok(parent == (IUnknown *)device, "Got parent %p, expected %p.\n", parent, device);
    IUnknown_Release(parent);

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter has not outputs.\n");
    }
    else
    {
        ok(SUCCEEDED(hr), "EnumOutputs failed, hr %#x.\n", hr);

        hr = IDXGIOutput_GetParent(output, &IID_IDXGIAdapter, (void **)&parent);
        IDXGIOutput_Release(output);
        ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);
        ok(parent == (IUnknown *)adapter, "Got parent %p, expected %p.\n", parent, adapter);
        IUnknown_Release(parent);
    }

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);

    hr = IDXGIFactory_GetParent(factory, &IID_IUnknown, (void **)&parent);
    ok(hr == E_NOINTERFACE, "GetParent returned %#x, expected %#x.\n", hr, E_NOINTERFACE);
    ok(parent == NULL, "Got parent %p, expected %p.\n", parent, NULL);
    IDXGIFactory_Release(factory);

    hr = IDXGIDevice_GetParent(device, &IID_IDXGIAdapter, (void **)&parent);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);
    ok(parent == (IUnknown *)adapter, "Got parent %p, expected %p.\n", parent, adapter);
    IUnknown_Release(parent);

    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_output(void)
{
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    HRESULT hr;
    IDXGIOutput *output;
    ULONG refcount;
    UINT mode_count, mode_count_comp, i;
    DXGI_MODE_DESC *modes;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter doesn't have any outputs.\n");
        IDXGIAdapter_Release(adapter);
        IDXGIDevice_Release(device);
        return;
    }
    ok(SUCCEEDED(hr), "EnumOutputs failed, hr %#x.\n", hr);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, NULL, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, NULL);
    ok(SUCCEEDED(hr)
            || broken(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE), /* Remote Desktop Services / Win 7 testbot */
            "Failed to list modes, hr %#x.\n", hr);
    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
    {
        win_skip("GetDisplayModeList() not supported.\n");
        IDXGIOutput_Release(output);
        IDXGIAdapter_Release(adapter);
        IDXGIDevice_Release(device);
        return;
    }
    mode_count_comp = mode_count;

    hr = IDXGIOutput_GetDisplayModeList(output, 0, 0, &mode_count, NULL);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(!mode_count, "Got unexpected mode_count %u.\n", mode_count);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, NULL);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(mode_count >= mode_count_comp, "Got unexpected mode_count %u, expected >= %u.\n", mode_count, mode_count_comp);
    mode_count_comp = mode_count;

    modes = heap_calloc(mode_count + 10, sizeof(*modes));
    ok(!!modes, "Failed to allocate memory.\n");

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, NULL, modes);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    ok(!modes[0].Height, "No output was expected.\n");

    mode_count = 0;
    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#x.\n", hr);
    ok(!modes[0].Height, "No output was expected.\n");

    mode_count = mode_count_comp;
    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(mode_count == mode_count_comp, "Got unexpected mode_count %u, expected %u.\n", mode_count, mode_count_comp);

    for (i = 0; i < mode_count; i++)
    {
        ok(modes[i].Height && modes[i].Width, "Proper mode was expected\n");
    }

    mode_count += 5;
    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(mode_count == mode_count_comp, "Got unexpected mode_count %u, expected %u.\n", mode_count, mode_count_comp);

    if (mode_count_comp)
    {
        mode_count = mode_count_comp - 1;
        hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_ENUM_MODES_SCALING, &mode_count, modes);
        ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#x.\n", hr);
        ok(mode_count == mode_count_comp - 1, "Got unexpected mode_count %u, expected %u.\n",
                mode_count, mode_count_comp - 1);
    }
    else
    {
        skip("Not enough modes for test.\n");
    }

    heap_free(modes);
    IDXGIOutput_Release(output);
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_find_closest_matching_mode(void)
{
    DXGI_MODE_DESC *modes, mode, matching_mode;
    unsigned int i, mode_count;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    IDXGIOutput *output;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        win_skip("Adapter doesn't have any outputs.\n");
        IDXGIAdapter_Release(adapter);
        IDXGIDevice_Release(device);
        return;
    }
    ok(SUCCEEDED(hr), "EnumOutputs failed, hr %#x.\n", hr);

    memset(&mode, 0, sizeof(mode));
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL || broken(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE), /* Win 7 testbot */
            "Got unexpected hr %#x.\n", hr);
    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
    {
        win_skip("FindClosestMatchingMode() not supported.\n");
        goto done;
    }

    memset(&mode, 0, sizeof(mode));
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, (IUnknown *)device);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, NULL);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);

    modes = heap_calloc(mode_count, sizeof(*modes));
    ok(!!modes, "Failed to allocate memory.\n");

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, modes);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);

    for (i = 0; i < mode_count; ++i)
    {
        mode = modes[i];
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_IGNORE_SCALING);

        mode.Format = DXGI_FORMAT_UNKNOWN;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

        mode = modes[i];
        mode.Width = 0;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

        mode = modes[i];
        mode.Height = 0;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

        mode = modes[i];
        mode.Width = mode.Height = 0;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_IGNORE_SCALING | MODE_DESC_IGNORE_RESOLUTION);
        ok(matching_mode.Width > 0 && matching_mode.Height > 0, "Got unexpected resolution %ux%u.\n",
                matching_mode.Width, matching_mode.Height);

        mode = modes[i];
        mode.RefreshRate.Numerator = mode.RefreshRate.Denominator = 0;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_IGNORE_SCALING | MODE_DESC_IGNORE_REFRESH_RATE);
        ok(matching_mode.RefreshRate.Numerator > 0 && matching_mode.RefreshRate.Denominator > 0,
                "Got unexpected refresh rate %u / %u.\n",
                matching_mode.RefreshRate.Numerator, matching_mode.RefreshRate.Denominator);

        mode = modes[i];
        mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_IGNORE_SCALING | MODE_DESC_IGNORE_SCANLINE_ORDERING);
        ok(matching_mode.ScanlineOrdering, "Got unexpected scanline ordering %#x.\n",
                matching_mode.ScanlineOrdering);

        memset(&mode, 0, sizeof(mode));
        mode.Width = modes[i].Width;
        mode.Height = modes[i].Height;
        mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

        memset(&mode, 0, sizeof(mode));
        mode.Width = modes[i].Width - 1;
        mode.Height = modes[i].Height - 1;
        mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

        memset(&mode, 0, sizeof(mode));
        mode.Width = modes[i].Width + 1;
        mode.Height = modes[i].Height + 1;
        mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);
    }

    memset(&mode, 0, sizeof(mode));
    mode.Width = mode.Height = 10;
    mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* Find mode for the lowest resolution. */
    mode = modes[0];
    for (i = 0; i < mode_count; ++i)
    {
        if (mode.Width >= modes[i].Width && mode.Height >= modes[i].Height)
            mode = modes[i];
    }
    check_mode_desc(&matching_mode, &mode, MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

    memset(&mode, 0, sizeof(mode));
    mode.Width = modes[0].Width;
    mode.Height = modes[0].Height;
    mode.Format = modes[0].Format;
    mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST;
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    check_mode_desc(&matching_mode, &modes[0], MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

    memset(&mode, 0, sizeof(mode));
    mode.Width = modes[0].Width;
    mode.Height = modes[0].Height;
    mode.Format = modes[0].Format;
    mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST;
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    check_mode_desc(&matching_mode, &modes[0], MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

    memset(&mode, 0, sizeof(mode));
    mode.Width = modes[0].Width;
    mode.Height = modes[0].Height;
    mode.Format = modes[0].Format;
    mode.Scaling = DXGI_MODE_SCALING_CENTERED;
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    check_mode_desc(&matching_mode, &modes[0], MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

    memset(&mode, 0, sizeof(mode));
    mode.Width = modes[0].Width;
    mode.Height = modes[0].Height;
    mode.Format = modes[0].Format;
    mode.Scaling = DXGI_MODE_SCALING_STRETCHED;
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    check_mode_desc(&matching_mode, &modes[0], MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

    heap_free(modes);

done:
    IDXGIOutput_Release(output);
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

struct refresh_rates
{
    UINT numerator;
    UINT denominator;
    BOOL numerator_should_pass;
    BOOL denominator_should_pass;
};

static void test_create_swapchain(void)
{
    struct swapchain_fullscreen_state initial_state, expected_state;
    unsigned int  i, expected_width, expected_height;
    DXGI_SWAP_CHAIN_DESC creation_desc, result_desc;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc;
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
    IDXGIDevice *device, *bgra_device;
    ULONG refcount, expected_refcount;
    IUnknown *obj, *obj2, *parent;
    IDXGISwapChain1 *swapchain1;
    RECT *expected_client_rect;
    IDXGISwapChain *swapchain;
    IDXGISurface1 *surface;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    IDXGIOutput *target;
    BOOL fullscreen;
    HWND window;
    HRESULT hr;

    const struct refresh_rates refresh_list[] =
    {
        {60, 60, FALSE, FALSE},
        {60,  0,  TRUE, FALSE},
        {60,  1,  TRUE,  TRUE},
        { 0, 60,  TRUE, FALSE},
        { 0,  0,  TRUE, FALSE},
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    creation_desc.BufferDesc.Width = 800;
    creation_desc.BufferDesc.Height = 600;
    creation_desc.BufferDesc.RefreshRate.Numerator = 60;
    creation_desc.BufferDesc.RefreshRate.Denominator = 60;
    creation_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    creation_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    creation_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    creation_desc.SampleDesc.Count = 1;
    creation_desc.SampleDesc.Quality = 0;
    creation_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    creation_desc.BufferCount = 1;
    creation_desc.OutputWindow = NULL;
    creation_desc.Windowed = TRUE;
    creation_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    creation_desc.Flags = 0;

    hr = IDXGIDevice_QueryInterface(device, &IID_IUnknown, (void **)&obj);
    ok(hr == S_OK, "IDXGIDevice does not implement IUnknown.\n");

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Failed to get adapter, hr %#x.\n", hr);

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Failed to get parent, hr %#x.\n", hr);

    expected_refcount = get_refcount(adapter);
    refcount = get_refcount(factory);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount(device);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    creation_desc.OutputWindow = NULL;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    creation_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    memset(&initial_state, 0, sizeof(initial_state));
    capture_fullscreen_state(&initial_state.fullscreen_state, creation_desc.OutputWindow);

    hr = IDXGIFactory_CreateSwapChain(factory, NULL, &creation_desc, &swapchain);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIFactory_CreateSwapChain(factory, obj, NULL, &swapchain);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);

    refcount = get_refcount(adapter);
    ok(refcount >= expected_refcount, "Got refcount %u, expected >= %u.\n", refcount, expected_refcount);
    refcount = get_refcount(factory);
    todo_wine ok(refcount == 4, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount(device);
    ok(refcount == 3, "Got unexpected refcount %u.\n", refcount);

    hr = IDXGISwapChain_GetDesc(swapchain, NULL);
    ok(hr == E_INVALIDARG, "GetDesc unexpectedly returned %#x.\n", hr);

    hr = IDXGISwapChain_GetParent(swapchain, &IID_IUnknown, (void **)&parent);
    ok(hr == S_OK, "Failed to get parent,%#x.\n", hr);
    ok(parent == (IUnknown *)factory, "Got unexpected parent interface pointer %p.\n", parent);
    refcount = IUnknown_Release(parent);
    todo_wine ok(refcount == 4, "Got unexpected refcount %u.\n", refcount);

    hr = IDXGISwapChain_GetParent(swapchain, &IID_IDXGIFactory, (void **)&parent);
    ok(hr == S_OK, "Failed to get parent,%#x.\n", hr);
    ok(parent == (IUnknown *)factory, "Got unexpected parent interface pointer %p.\n", parent);
    refcount = IUnknown_Release(parent);
    todo_wine ok(refcount == 4, "Got unexpected refcount %u.\n", refcount);

    hr = IDXGISwapChain_QueryInterface(swapchain, &IID_IDXGISwapChain1, (void **)&swapchain1);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Failed to query IDXGISwapChain1 interface, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDXGISwapChain1_GetDesc1(swapchain1, NULL);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
        hr = IDXGISwapChain1_GetDesc1(swapchain1, &swapchain_desc);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(!swapchain_desc.Stereo, "Got unexpected stereo %#x.\n", swapchain_desc.Stereo);
        ok(swapchain_desc.Scaling == DXGI_SCALING_STRETCH,
                "Got unexpected scaling %#x.\n", swapchain_desc.Scaling);
        ok(swapchain_desc.AlphaMode == DXGI_ALPHA_MODE_IGNORE,
                "Got unexpected alpha mode %#x.\n", swapchain_desc.AlphaMode);
        hr = IDXGISwapChain1_GetFullscreenDesc(swapchain1, NULL);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
        hr = IDXGISwapChain1_GetFullscreenDesc(swapchain1, &fullscreen_desc);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(fullscreen_desc.Windowed == creation_desc.Windowed,
                "Got unexpected windowed %#x.\n", fullscreen_desc.Windowed);
        hr = IDXGISwapChain1_GetHwnd(swapchain1, &window);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(window == creation_desc.OutputWindow, "Got unexpected window %p.\n", window);
        IDXGISwapChain1_Release(swapchain1);
    }

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "Swapchain has %u references left.\n", refcount);

    refcount = get_refcount(factory);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    for (i = 0; i < ARRAY_SIZE(refresh_list); ++i)
    {
        creation_desc.BufferDesc.RefreshRate.Numerator = refresh_list[i].numerator;
        creation_desc.BufferDesc.RefreshRate.Denominator = refresh_list[i].denominator;

        hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
        ok(hr == S_OK, "Test %u: Failed to create swapchain, hr %#x.\n", i, hr);

        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(hr == S_OK, "Test %u: Failed to get swapchain desc, hr %#x.\n", i, hr);

        ok(result_desc.Windowed == creation_desc.Windowed, "Test %u: Got unexpected windowed %#x.\n",
                i, result_desc.Windowed);

        todo_wine_if (!refresh_list[i].numerator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Numerator == refresh_list[i].numerator,
                    "Numerator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Numerator);

        todo_wine_if (!refresh_list[i].denominator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Denominator == refresh_list[i].denominator,
                    "Denominator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Denominator);

        fullscreen = 0xdeadbeef;
        target = (void *)0xdeadbeef;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &target);
        ok(hr == S_OK, "Test %u: Failed to get fullscreen state, hr %#x.\n", i, hr);
        ok(!fullscreen, "Test %u: Got unexpected fullscreen %#x.\n", i, fullscreen);
        ok(!target, "Test %u: Got unexpected target %p.\n", i, target);

        hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        fullscreen = 0xdeadbeef;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(!fullscreen, "Test %u: Got unexpected fullscreen %#x.\n", i, fullscreen);
        target = (void *)0xdeadbeef;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, &target);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(!target, "Test %u: Got unexpected target %p.\n", i, target);

        check_swapchain_fullscreen_state(swapchain, &initial_state);
        IDXGISwapChain_Release(swapchain);
    }

    check_window_fullscreen_state(creation_desc.OutputWindow, &initial_state.fullscreen_state);

    /* Test GDI-compatible swapchain */
    bgra_device = create_device(D3D10_CREATE_DEVICE_BGRA_SUPPORT);
    ok(!!bgra_device, "Failed to create BGRA capable device.\n");

    hr = IDXGIDevice_QueryInterface(bgra_device, &IID_IUnknown, (void **)&obj2);
    ok(hr == S_OK, "IDXGIDevice does not implement IUnknown.\n");

    hr = IDXGIFactory_CreateSwapChain(factory, obj2, &creation_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);

    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface1, (void **)&surface);
    if (SUCCEEDED(hr))
    {
        HDC hdc;

        hr = IDXGISurface1_GetDC(surface, FALSE, &hdc);
        ok(FAILED(hr), "Expected GetDC() to fail, %#x\n", hr);

        IDXGISurface1_Release(surface);
        IDXGISwapChain_Release(swapchain);

        creation_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        creation_desc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;

        hr = IDXGIFactory_CreateSwapChain(factory, obj2, &creation_desc, &swapchain);
        ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);

        creation_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        creation_desc.Flags = 0;

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface1, (void **)&surface);
        ok(hr == S_OK, "Failed to get front buffer, hr %#x.\n", hr);

        hr = IDXGISurface1_GetDC(surface, FALSE, &hdc);
        ok(hr == S_OK, "Expected GetDC() to succeed, %#x\n", hr);
        IDXGISurface1_ReleaseDC(surface, NULL);

        IDXGISurface1_Release(surface);
        IDXGISwapChain_Release(swapchain);
    }
    else
    {
        win_skip("IDXGISurface1 is not supported, skipping GetDC() tests.\n");
        IDXGISwapChain_Release(swapchain);
    }
    IUnknown_Release(obj2);
    IDXGIDevice_Release(bgra_device);

    creation_desc.Windowed = FALSE;

    for (i = 0; i < ARRAY_SIZE(refresh_list); ++i)
    {
        creation_desc.BufferDesc.RefreshRate.Numerator = refresh_list[i].numerator;
        creation_desc.BufferDesc.RefreshRate.Denominator = refresh_list[i].denominator;

        hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
        ok(SUCCEEDED(hr), "Test %u: Failed to create swapchain, hr %#x.\n", i, hr);

        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(hr == S_OK, "Test %u: Failed to get swapchain desc, hr %#x.\n", i, hr);

        /* When numerator is non-zero and denominator is zero, the windowed mode is used.
         * Additionally, some versions of WARP seem to always fail to change fullscreen state. */
        if (result_desc.Windowed != creation_desc.Windowed)
            trace("Test %u: Failed to change fullscreen state.\n", i);

        todo_wine_if (!refresh_list[i].numerator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Numerator == refresh_list[i].numerator,
                    "Numerator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Numerator);

        todo_wine_if (!refresh_list[i].denominator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Denominator == refresh_list[i].denominator,
                    "Denominator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Denominator);

        fullscreen = FALSE;
        target = NULL;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &target);
        ok(hr == S_OK, "Test %u: Failed to get fullscreen state, hr %#x.\n", i, hr);
        ok(fullscreen == !result_desc.Windowed, "Test %u: Got fullscreen %#x, expected %#x.\n",
                i, fullscreen, result_desc.Windowed);
        ok(result_desc.Windowed ? !target : !!target, "Test %u: Got unexpected target %p.\n", i, target);
        if (!result_desc.Windowed)
        {
            IDXGIOutput *containing_output;
            hr = IDXGISwapChain_GetContainingOutput(swapchain, &containing_output);
            ok(hr == S_OK, "Test %u: Failed to get containing output, hr %#x.\n", i, hr);
            ok(containing_output == target, "Test %u: Got unexpected containing output pointer %p.\n",
                    i, containing_output);
            IDXGIOutput_Release(containing_output);

            ok(output_belongs_to_adapter(target, adapter),
                    "Test %u: Output %p doesn't belong to adapter %p.\n",
                    i, target, adapter);
            IDXGIOutput_Release(target);

            hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, NULL);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
            fullscreen = FALSE;
            hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
            ok(fullscreen, "Test %u: Got unexpected fullscreen %#x.\n", i, fullscreen);
            target = NULL;
            hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, &target);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
            ok(!!target, "Test %u: Got unexpected target %p.\n", i, target);
            IDXGIOutput_Release(target);
        }

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Test %u: Failed to set fullscreen state, hr %#x.\n", i, hr);

        fullscreen = 0xdeadbeef;
        target = (void *)0xdeadbeef;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &target);
        ok(hr == S_OK, "Test %u: Failed to get fullscreen state, hr %#x.\n", i, hr);
        ok(!fullscreen, "Test %u: Got unexpected fullscreen %#x.\n", i, fullscreen);
        ok(!target, "Test %u: Got unexpected target %p.\n", i, target);

        check_swapchain_fullscreen_state(swapchain, &initial_state);
        IDXGISwapChain_Release(swapchain);
    }

    check_window_fullscreen_state(creation_desc.OutputWindow, &initial_state.fullscreen_state);

    /* Test swapchain creation with DXGI_FORMAT_UNKNOWN. */
    creation_desc.BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    creation_desc.Windowed = TRUE;
    creation_desc.Flags = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    creation_desc.Windowed = FALSE;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    creation_desc.BufferCount = 2;
    creation_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == E_INVALIDARG || hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    creation_desc.BufferCount = 1;
    creation_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    check_window_fullscreen_state(creation_desc.OutputWindow, &initial_state.fullscreen_state);

    /* Test swapchain creation with backbuffer width and height equal to 0. */
    expected_state = initial_state;
    expected_client_rect = &expected_state.fullscreen_state.client_rect;

    /* Windowed */
    expected_width = expected_client_rect->right;
    expected_height = expected_client_rect->bottom;

    creation_desc.BufferDesc.Width = 0;
    creation_desc.BufferDesc.Height = 0;
    creation_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    creation_desc.Windowed = TRUE;
    creation_desc.Flags = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(result_desc.BufferDesc.Width == expected_width, "Got width %u, expected %u.\n",
            result_desc.BufferDesc.Width, expected_width);
    ok(result_desc.BufferDesc.Height == expected_height, "Got height %u, expected %u.\n",
            result_desc.BufferDesc.Height, expected_height);
    check_swapchain_fullscreen_state(swapchain, &expected_state);
    IDXGISwapChain_Release(swapchain);

    DestroyWindow(creation_desc.OutputWindow);
    creation_desc.OutputWindow = CreateWindowA("static", "dxgi_test",
            WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
            0, 0, 222, 222, 0, 0, 0, 0);
    SetRect(&expected_state.fullscreen_state.window_rect, 0, 0, 222, 222);
    GetClientRect(creation_desc.OutputWindow, expected_client_rect);
    expected_width = expected_client_rect->right;
    expected_height = expected_client_rect->bottom;

    creation_desc.BufferDesc.Width = 0;
    creation_desc.BufferDesc.Height = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(result_desc.BufferDesc.Width == expected_width, "Got width %u, expected %u.\n",
            result_desc.BufferDesc.Width, expected_width);
    ok(result_desc.BufferDesc.Height == expected_height, "Got height %u, expected %u.\n",
            result_desc.BufferDesc.Height, expected_height);
    check_swapchain_fullscreen_state(swapchain, &expected_state);
    IDXGISwapChain_Release(swapchain);

    DestroyWindow(creation_desc.OutputWindow);
    creation_desc.OutputWindow = CreateWindowA("static", "dxgi_test",
            WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            1, 1, 0, 0, 0, 0, 0, 0);
    SetRect(&expected_state.fullscreen_state.window_rect, 1, 1, 1, 1);
    SetRectEmpty(expected_client_rect);
    expected_width = expected_height = 8;

    creation_desc.BufferDesc.Width = 0;
    creation_desc.BufferDesc.Height = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(result_desc.BufferDesc.Width == expected_width, "Got width %u, expected %u.\n",
            result_desc.BufferDesc.Width, expected_width);
    ok(result_desc.BufferDesc.Height == expected_height, "Got height %u, expected %u.\n",
            result_desc.BufferDesc.Height, expected_height);
    check_swapchain_fullscreen_state(swapchain, &expected_state);
    IDXGISwapChain_Release(swapchain);

    DestroyWindow(creation_desc.OutputWindow);
    creation_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    check_window_fullscreen_state(creation_desc.OutputWindow, &initial_state.fullscreen_state);

    /* Fullscreen */
    creation_desc.Windowed = FALSE;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(SUCCEEDED(hr), "Failed to create swapchain, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Failed to get swapchain desc, hr %#x.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Failed to set fullscreen state, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetContainingOutput(swapchain, &expected_state.target);
    ok(hr == S_OK || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
            "Failed to get containing output, hr %#x.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &initial_state);
    IDXGISwapChain_Release(swapchain);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        win_skip("GetContainingOutput() not supported.\n");
        goto done;
    }
    if (result_desc.Windowed)
    {
        win_skip("Fullscreen not supported.\n");
        IDXGIOutput_Release(expected_state.target);
        goto done;
    }

    creation_desc.BufferDesc.Width = 0;
    creation_desc.BufferDesc.Height = 0;
    creation_desc.Windowed = FALSE;
    creation_desc.Flags = 0;
    compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
            &creation_desc, &initial_state.fullscreen_state.monitor_rect, 0, 0, expected_state.target);
    expected_width = expected_client_rect->right - expected_client_rect->left;
    expected_height = expected_client_rect->bottom - expected_client_rect->top;

    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Failed to get swapchain desc, hr %#x.\n", hr);
    todo_wine ok(result_desc.BufferDesc.Width == expected_width, "Got width %u, expected %u.\n",
            result_desc.BufferDesc.Width, expected_width);
    todo_wine ok(result_desc.BufferDesc.Height == expected_height, "Got height %u, expected %u.\n",
            result_desc.BufferDesc.Height, expected_height);
    check_swapchain_fullscreen_state(swapchain, &expected_state);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Failed to set fullscreen state, hr %#x.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &initial_state);
    IDXGISwapChain_Release(swapchain);

    /* Fullscreen and DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH */
    creation_desc.BufferDesc.Width = 0;
    creation_desc.BufferDesc.Height = 0;
    creation_desc.Windowed = FALSE;
    creation_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
            &creation_desc, &initial_state.fullscreen_state.monitor_rect, 0, 0, expected_state.target);
    expected_width = expected_client_rect->right - expected_client_rect->left;
    expected_height = expected_client_rect->bottom - expected_client_rect->top;

    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Failed to get swapchain desc, hr %#x.\n", hr);
    todo_wine ok(result_desc.BufferDesc.Width == expected_width, "Got width %u, expected %u.\n",
            result_desc.BufferDesc.Width, expected_width);
    todo_wine ok(result_desc.BufferDesc.Height == expected_height, "Got height %u, expected %u.\n",
            result_desc.BufferDesc.Height, expected_height);
    check_swapchain_fullscreen_state(swapchain, &expected_state);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Failed to set fullscreen state, hr %#x.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &initial_state);
    IDXGISwapChain_Release(swapchain);

    IDXGIOutput_Release(expected_state.target);

done:
    IUnknown_Release(obj);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIAdapter_Release(adapter);
    ok(!refcount, "Adapter has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
    check_window_fullscreen_state(creation_desc.OutputWindow, &initial_state.fullscreen_state);
    DestroyWindow(creation_desc.OutputWindow);
}

static void test_get_containing_output(void)
{
    unsigned int output_count, output_idx;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGIOutput *output, *output2;
    DXGI_OUTPUT_DESC output_desc;
    MONITORINFOEXW monitor_info;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    POINT points[4 * 16];
    IDXGIDevice *device;
    unsigned int i, j;
    HMONITOR monitor;
    ULONG refcount;
    HRESULT hr;
    BOOL ret;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);

    swapchain_desc.BufferDesc.Width = 100;
    swapchain_desc.BufferDesc.Height = 100;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 100, 100, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    output_count = 0;
    while (IDXGIAdapter_EnumOutputs(adapter, output_count, &output) != DXGI_ERROR_NOT_FOUND)
    {
        ok(SUCCEEDED(hr), "Failed to enumerate output %u, hr %#x.\n", output_count, hr);
        IDXGIOutput_Release(output);
        ++output_count;
    }

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

    monitor = MonitorFromWindow(swapchain_desc.OutputWindow, 0);
    ok(!!monitor, "MonitorFromWindow failed.\n");

    monitor_info.cbSize = sizeof(monitor_info);
    ret = GetMonitorInfoW(monitor, (MONITORINFO *)&monitor_info);
    ok(ret, "Failed to get monitor info.\n");

    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
    ok(SUCCEEDED(hr) || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
            "GetContainingOutput failed, hr %#x.\n", hr);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        win_skip("GetContainingOutput() not supported.\n");
        IDXGISwapChain_Release(swapchain);
        goto done;
    }

    hr = IDXGIOutput_GetDesc(output, &output_desc);
    ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output2);
    ok(SUCCEEDED(hr), "GetContainingOutput failed, hr %#x.\n", hr);
    ok(output != output2, "Got unexpected output pointers %p, %p.\n", output, output2);
    check_output_equal(output, output2);

    refcount = IDXGIOutput_Release(output);
    ok(!refcount, "IDXGIOutput has %u references left.\n", refcount);
    refcount = IDXGIOutput_Release(output2);
    ok(!refcount, "IDXGIOutput has %u references left.\n", refcount);

    ok(!lstrcmpW(output_desc.DeviceName, monitor_info.szDevice),
            "Got unexpected device name %s, expected %s.\n",
            wine_dbgstr_w(output_desc.DeviceName), wine_dbgstr_w(monitor_info.szDevice));
    ok(EqualRect(&output_desc.DesktopCoordinates, &monitor_info.rcMonitor),
            "Got unexpected desktop coordinates %s, expected %s.\n",
            wine_dbgstr_rect(&output_desc.DesktopCoordinates),
            wine_dbgstr_rect(&monitor_info.rcMonitor));

    output_idx = 0;
    while ((hr = IDXGIAdapter_EnumOutputs(adapter, output_idx, &output)) != DXGI_ERROR_NOT_FOUND)
    {
        ok(SUCCEEDED(hr), "Failed to enumerate output %u, hr %#x.\n", output_idx, hr);

        hr = IDXGIOutput_GetDesc(output, &output_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

        /* Move the OutputWindow to the current output. */
        ret = SetWindowPos(swapchain_desc.OutputWindow, 0,
                output_desc.DesktopCoordinates.left, output_desc.DesktopCoordinates.top,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
        ok(ret, "SetWindowPos failed.\n");

        hr = IDXGISwapChain_GetContainingOutput(swapchain, &output2);
        ok(SUCCEEDED(hr), "GetContainingOutput failed, hr %#x.\n", hr);

        check_output_equal(output, output2);

        refcount = IDXGIOutput_Release(output2);
        ok(!refcount, "IDXGIOutput has %u references left.\n", refcount);
        refcount = IDXGIOutput_Release(output);
        ok(!refcount, "IDXGIOutput has %u references left.\n", refcount);
        ++output_idx;

        /* Move the OutputWindow around the corners of the current output desktop coordinates. */
        for (i = 0; i < 4; ++i)
        {
            static const POINT offsets[] =
            {
                {  0,   0},
                {-49,   0}, {-50,   0}, {-51,   0},
                {  0, -49}, {  0, -50}, {  0, -51},
                {-49, -49}, {-50, -49}, {-51, -49},
                {-49, -50}, {-50, -50}, {-51, -50},
                {-49, -51}, {-50, -51}, {-51, -51},
            };
            unsigned int x, y;

            switch (i)
            {
                case 0:
                    x = output_desc.DesktopCoordinates.left;
                    y = output_desc.DesktopCoordinates.top;
                    break;
                case 1:
                    x = output_desc.DesktopCoordinates.right;
                    y = output_desc.DesktopCoordinates.top;
                    break;
                case 2:
                    x = output_desc.DesktopCoordinates.right;
                    y = output_desc.DesktopCoordinates.bottom;
                    break;
                case 3:
                    x = output_desc.DesktopCoordinates.left;
                    y = output_desc.DesktopCoordinates.bottom;
                    break;
            }

            for (j = 0; j < ARRAY_SIZE(offsets); ++j)
            {
                unsigned int idx = ARRAY_SIZE(offsets) * i + j;
                assert(idx < ARRAY_SIZE(points));
                points[idx].x = x + offsets[j].x;
                points[idx].y = y + offsets[j].y;
            }
        }

        for (i = 0; i < ARRAY_SIZE(points); ++i)
        {
            ret = SetWindowPos(swapchain_desc.OutputWindow, 0, points[i].x, points[i].y,
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
            ok(ret, "Faled to set window position.\n");

            monitor = MonitorFromWindow(swapchain_desc.OutputWindow, MONITOR_DEFAULTTONEAREST);
            ok(!!monitor, "Failed to get monitor from window.\n");

            monitor_info.cbSize = sizeof(monitor_info);
            ret = GetMonitorInfoW(monitor, (MONITORINFO *)&monitor_info);
            ok(ret, "Failed to get monitor info.\n");

            hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
            ok(hr == S_OK || broken(hr == DXGI_ERROR_UNSUPPORTED),
                    "Failed to get containing output, hr %#x.\n", hr);
            if (hr == DXGI_ERROR_UNSUPPORTED)
                continue;
            ok(!!output, "Got unexpected containing output %p.\n", output);
            hr = IDXGIOutput_GetDesc(output, &output_desc);
            ok(hr == S_OK, "Failed to get output desc, hr %#x.\n", hr);
            refcount = IDXGIOutput_Release(output);
            ok(!refcount, "IDXGIOutput has %u references left.\n", refcount);

            ok(!lstrcmpW(output_desc.DeviceName, monitor_info.szDevice),
                    "Got unexpected device name %s, expected %s.\n",
                    wine_dbgstr_w(output_desc.DeviceName), wine_dbgstr_w(monitor_info.szDevice));
            ok(EqualRect(&output_desc.DesktopCoordinates, &monitor_info.rcMonitor),
                    "Got unexpected desktop coordinates %s, expected %s.\n",
                    wine_dbgstr_rect(&output_desc.DesktopCoordinates),
                    wine_dbgstr_rect(&monitor_info.rcMonitor));
        }
    }

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

done:
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIAdapter_Release(adapter);
    ok(!refcount, "Adapter has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
    DestroyWindow(swapchain_desc.OutputWindow);
}

static void test_swapchain_fullscreen_state(IDXGISwapChain *swapchain,
        IDXGIAdapter *adapter, const struct swapchain_fullscreen_state *initial_state)
{
    MONITORINFOEXW monitor_info, *output_monitor_info;
    struct swapchain_fullscreen_state expected_state;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_OUTPUT_DESC output_desc;
    unsigned int i, output_count;
    IDXGIOutput *output;
    HRESULT hr;
    BOOL ret;

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

    check_swapchain_fullscreen_state(swapchain, initial_state);

    expected_state = *initial_state;
    compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
            &swapchain_desc, &initial_state->fullscreen_state.monitor_rect, 800, 600, NULL);
    hr = IDXGISwapChain_GetContainingOutput(swapchain, &expected_state.target);
    ok(SUCCEEDED(hr), "GetContainingOutput failed, hr %#x.\n", hr);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "Got unexpected hr %#x.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not change fullscreen state.\n");
        IDXGIOutput_Release(expected_state.target);
        return;
    }
    check_swapchain_fullscreen_state(swapchain, &expected_state);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &expected_state);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);
    check_swapchain_fullscreen_state(swapchain, initial_state);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    check_swapchain_fullscreen_state(swapchain, initial_state);

    IDXGIOutput_Release(expected_state.target);
    expected_state.target = NULL;

    output_count = 0;
    while (IDXGIAdapter_EnumOutputs(adapter, output_count, &output) != DXGI_ERROR_NOT_FOUND)
    {
        IDXGIOutput_Release(output);
        ++output_count;
    }

    output_monitor_info = heap_calloc(output_count, sizeof(*output_monitor_info));
    ok(!!output_monitor_info, "Failed to allocate memory.\n");
    for (i = 0; i < output_count; ++i)
    {
        hr = IDXGIAdapter_EnumOutputs(adapter, i, &output);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

        hr = IDXGIOutput_GetDesc(output, &output_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

        output_monitor_info[i].cbSize = sizeof(*output_monitor_info);
        ret = GetMonitorInfoW(output_desc.Monitor, (MONITORINFO *)&output_monitor_info[i]);
        ok(ret, "Failed to get monitor info.\n");

        IDXGIOutput_Release(output);
    }

    for (i = 0; i < output_count; ++i)
    {
        RECT orig_monitor_rect = output_monitor_info[i].rcMonitor;
        IDXGIOutput *target;
        BOOL fullscreen;

        hr = IDXGIAdapter_EnumOutputs(adapter, i, &output);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDXGIOutput_GetDesc(output, &output_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

        expected_state = *initial_state;
        expected_state.target = output;
        expected_state.fullscreen_state.monitor = output_desc.Monitor;
        expected_state.fullscreen_state.monitor_rect = orig_monitor_rect;
        compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
                &swapchain_desc, &orig_monitor_rect, 800, 600, NULL);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &expected_state);

        target = NULL;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, &target);
        ok(SUCCEEDED(hr), "GetFullscreenState failed, hr %#x.\n", hr);
        ok(target == output, "Got target pointer %p, expected %p.\n", target, output);
        IDXGIOutput_Release(target);
        fullscreen = FALSE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(SUCCEEDED(hr), "GetFullscreenState failed, hr %#x.\n", hr);
        ok(fullscreen, "Got unexpected fullscreen %#x.\n", hr);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &expected_state);
        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, output);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &expected_state);
        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_swapchain_fullscreen_state(swapchain, initial_state);

        fullscreen = TRUE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(SUCCEEDED(hr), "GetFullscreenState failed, hr %#x.\n", hr);
        ok(!fullscreen, "Got unexpected fullscreen %#x.\n", hr);

        check_swapchain_fullscreen_state(swapchain, initial_state);
        monitor_info.cbSize = sizeof(monitor_info);
        ret = GetMonitorInfoW(output_desc.Monitor, (MONITORINFO *)&monitor_info);
        ok(ret, "Failed to get monitor info.\n");
        ok(EqualRect(&monitor_info.rcMonitor, &orig_monitor_rect), "Got monitor rect %s, expected %s.\n",
                wine_dbgstr_rect(&monitor_info.rcMonitor), wine_dbgstr_rect(&orig_monitor_rect));

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

        IDXGIOutput_Release(output);
    }

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    check_swapchain_fullscreen_state(swapchain, initial_state);

    for (i = 0; i < output_count; ++i)
    {
        hr = IDXGIAdapter_EnumOutputs(adapter, i, &output);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

        hr = IDXGIOutput_GetDesc(output, &output_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

        monitor_info.cbSize = sizeof(monitor_info);
        ret = GetMonitorInfoW(output_desc.Monitor, (MONITORINFO *)&monitor_info);
        ok(ret, "Failed to get monitor info.\n");

        ok(EqualRect(&monitor_info.rcMonitor, &output_monitor_info[i].rcMonitor),
                "Got monitor rect %s, expected %s.\n",
                wine_dbgstr_rect(&monitor_info.rcMonitor),
                wine_dbgstr_rect(&output_monitor_info[i].rcMonitor));

        IDXGIOutput_Release(output);
    }

    heap_free(output_monitor_info);
}

static void test_set_fullscreen(IUnknown *device, BOOL is_d3d12)
{
    struct swapchain_fullscreen_state initial_state;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGIAdapter *adapter = NULL;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIOutput *output;
    ULONG refcount;
    HRESULT hr;

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    memset(&initial_state, 0, sizeof(initial_state));
    capture_fullscreen_state(&initial_state.fullscreen_state, swapchain_desc.OutputWindow);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
    ok(SUCCEEDED(hr) || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
            "Failed to get containing output, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not get output.\n");
        goto done;
    }
    hr = IDXGIOutput_GetParent(output, &IID_IDXGIAdapter, (void **)&adapter);
    ok(hr == S_OK, "Failed to get parent, hr %#x.\n", hr);
    IDXGIOutput_Release(output);

    check_swapchain_fullscreen_state(swapchain, &initial_state);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(SUCCEEDED(hr) || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
            || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
            "SetFullscreenState failed, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not change fullscreen state.\n");
        goto done;
    }
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

    DestroyWindow(swapchain_desc.OutputWindow);
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &initial_state);
    test_swapchain_fullscreen_state(swapchain, adapter, &initial_state);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

    DestroyWindow(swapchain_desc.OutputWindow);
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    DestroyWindow(swapchain_desc.OutputWindow);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "Got unexpected hr %#x.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    DestroyWindow(swapchain_desc.OutputWindow);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "Got unexpected hr %#x.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);
    swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &initial_state);
    test_swapchain_fullscreen_state(swapchain, adapter, &initial_state);

done:
    if (adapter)
        IDXGIAdapter_Release(adapter);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);
    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);
    DestroyWindow(swapchain_desc.OutputWindow);

    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %u.\n", refcount);
}

static void test_default_fullscreen_target_output(void)
{
    IDXGIOutput *output, *containing_output, *target;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_OUTPUT_DESC output_desc;
    IDXGISwapChain *swapchain;
    unsigned int output_idx;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;
    BOOL ret;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);

    swapchain_desc.BufferDesc.Width = 100;
    swapchain_desc.BufferDesc.Height = 100;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 100, 100, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

    output_idx = 0;
    while ((hr = IDXGIAdapter_EnumOutputs(adapter, output_idx, &output)) != DXGI_ERROR_NOT_FOUND)
    {
        ok(SUCCEEDED(hr), "Failed to enumerate output %u, hr %#x.\n", output_idx, hr);

        hr = IDXGIOutput_GetDesc(output, &output_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

        /* Move the OutputWindow to the current output. */
        ret = SetWindowPos(swapchain_desc.OutputWindow, 0,
                output_desc.DesktopCoordinates.left, output_desc.DesktopCoordinates.top,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
        ok(ret, "SetWindowPos failed.\n");

        hr = IDXGISwapChain_GetContainingOutput(swapchain, &containing_output);
        ok(SUCCEEDED(hr) || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
                "GetContainingOutput failed, hr %#x.\n", hr);
        if (hr == DXGI_ERROR_UNSUPPORTED)
        {
            win_skip("GetContainingOutput() not supported.\n");
            IDXGIOutput_Release(output);
            goto done;
        }

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
        ok(SUCCEEDED(hr) || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE,
                "SetFullscreenState failed, hr %#x.\n", hr);
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            skip("Could not change fullscreen state.\n");
            IDXGIOutput_Release(containing_output);
            IDXGIOutput_Release(output);
            goto done;
        }

        target = NULL;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, &target);
        ok(SUCCEEDED(hr), "GetFullscreenState failed, hr %#x.\n", hr);
        ok(target != containing_output, "Got unexpected output pointers %p, %p.\n",
                target, containing_output);
        check_output_equal(target, containing_output);

        refcount = IDXGIOutput_Release(containing_output);
        ok(!refcount, "IDXGIOutput has %u references left.\n", refcount);

        hr = IDXGISwapChain_GetContainingOutput(swapchain, &containing_output);
        ok(SUCCEEDED(hr), "GetContainingOutput failed, hr %#x.\n", hr);
        ok(containing_output == target, "Got unexpected containing output %p, expected %p.\n",
                containing_output, target);
        refcount = IDXGIOutput_Release(containing_output);
        ok(refcount >= 2, "Got unexpected refcount %u.\n", refcount);
        refcount = IDXGIOutput_Release(target);
        ok(refcount >= 1, "Got unexpected refcount %u.\n", refcount);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);

        IDXGIOutput_Release(output);
        ++output_idx;
    }

done:
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIAdapter_Release(adapter);
    ok(!refcount, "Adapter has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
    DestroyWindow(swapchain_desc.OutputWindow);
}

static void test_windowed_resize_target(IDXGISwapChain *swapchain, HWND window,
        struct swapchain_fullscreen_state *state)
{
    struct swapchain_fullscreen_state expected_state;
    struct fullscreen_state *e;
    DXGI_MODE_DESC mode;
    RECT window_rect;
    unsigned int i;
    HRESULT hr;
    BOOL ret;

    static const struct
    {
        unsigned int width, height;
    }
    sizes[] =
    {
        {200, 200},
        {400, 200},
        {400, 400},
        {600, 800},
        {1000, 600},
        {1600, 100},
        {2000, 1000},
    };

    check_swapchain_fullscreen_state(swapchain, state);
    expected_state = *state;
    e = &expected_state.fullscreen_state;

    for (i = 0; i < ARRAY_SIZE(sizes); ++i)
    {
        SetRect(&e->client_rect, 0, 0, sizes[i].width, sizes[i].height);
        e->window_rect = e->client_rect;
        ret = AdjustWindowRectEx(&e->window_rect, GetWindowLongW(window, GWL_STYLE),
                FALSE, GetWindowLongW(window, GWL_EXSTYLE));
        ok(ret, "AdjustWindowRectEx failed.\n");
        if (GetMenu(window))
            e->client_rect.bottom -= GetSystemMetrics(SM_CYMENU);
        SetRect(&e->window_rect, 0, 0,
                e->window_rect.right - e->window_rect.left,
                e->window_rect.bottom - e->window_rect.top);
        GetWindowRect(window, &window_rect);
        OffsetRect(&e->window_rect, window_rect.left, window_rect.top);
        if (e->window_rect.right >= e->monitor_rect.right
                || e->window_rect.bottom >= e->monitor_rect.bottom)
        {
            skip("Test %u: Window %s does not fit on screen %s.\n",
                    i, wine_dbgstr_rect(&e->window_rect), wine_dbgstr_rect(&e->monitor_rect));
            continue;
        }

        memset(&mode, 0, sizeof(mode));
        mode.Width = sizes[i].width;
        mode.Height = sizes[i].height;
        hr = IDXGISwapChain_ResizeTarget(swapchain, &mode);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &expected_state);
    }

    ret = MoveWindow(window, 0, 0, 0, 0, TRUE);
    ok(ret, "Failed to move window.\n");
    GetWindowRect(window, &e->window_rect);
    GetClientRect(window, &e->client_rect);
    ret = MoveWindow(window, 0, 0, 200, 200, TRUE);
    ok(ret, "Failed to move window.\n");

    memset(&mode, 0, sizeof(mode));
    hr = IDXGISwapChain_ResizeTarget(swapchain, &mode);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &expected_state);

    GetWindowRect(window, &e->window_rect);
    GetClientRect(window, &e->client_rect);
    *state = expected_state;
}

static void test_fullscreen_resize_target(IDXGISwapChain *swapchain,
        const struct swapchain_fullscreen_state *initial_state)
{
    struct swapchain_fullscreen_state expected_state;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_OUTPUT_DESC output_desc;
    unsigned int i, mode_count;
    DXGI_MODE_DESC *modes;
    IDXGIOutput *target;
    HRESULT hr;

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

    hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, &target);
    ok(SUCCEEDED(hr), "GetFullscreenState failed, hr %#x.\n", hr);

    hr = IDXGIOutput_GetDisplayModeList(target, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, NULL);
    ok(SUCCEEDED(hr) || broken(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE), /* Win 7 testbot */
            "Failed to list modes, hr %#x.\n", hr);
    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
    {
        win_skip("GetDisplayModeList() not supported.\n");
        IDXGIOutput_Release(target);
        return;
    }

    modes = heap_calloc(mode_count, sizeof(*modes));
    ok(!!modes, "Failed to allocate memory.\n");

    hr = IDXGIOutput_GetDisplayModeList(target, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, modes);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);

    expected_state = *initial_state;
    for (i = 0; i < min(mode_count, 20); ++i)
    {
        /* FIXME: Modes with scaling aren't fully tested. */
        if (!(swapchain_desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
                && modes[i].Scaling != DXGI_MODE_SCALING_UNSPECIFIED)
            continue;

        hr = IDXGIOutput_GetDesc(target, &output_desc);
        ok(hr == S_OK, "Failed to get desc, hr %#x.\n", hr);

        compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
                &swapchain_desc, &output_desc.DesktopCoordinates, modes[i].Width, modes[i].Height, NULL);

        hr = IDXGISwapChain_ResizeTarget(swapchain, &modes[i]);
        ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "Got unexpected hr %#x.\n", hr);
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            skip("Failed to change to video mode %u.\n", i);
            break;
        }
        check_swapchain_fullscreen_state(swapchain, &expected_state);

        hr = IDXGIOutput_GetDesc(target, &output_desc);
        ok(hr == S_OK, "Failed to get desc, hr %#x.\n", hr);
        ok(EqualRect(&output_desc.DesktopCoordinates, &expected_state.fullscreen_state.monitor_rect),
                "Got desktop coordinates %s, expected %s.\n",
                wine_dbgstr_rect(&output_desc.DesktopCoordinates),
                wine_dbgstr_rect(&expected_state.fullscreen_state.monitor_rect));
    }

    heap_free(modes);
    IDXGIOutput_Release(target);
}

static void test_resize_target(IUnknown *device, BOOL is_d3d12)
{
    struct swapchain_fullscreen_state initial_state, expected_state;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    static const struct
    {
        POINT origin;
        BOOL fullscreen;
        BOOL menu;
        unsigned int flags;
    }
    tests[] =
    {
        {{ 0,  0}, TRUE,  FALSE, 0},
        {{10, 10}, TRUE,  FALSE, 0},
        {{ 0,  0}, TRUE,  FALSE, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
        {{10, 10}, TRUE,  FALSE, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
        {{ 0,  0}, FALSE, FALSE, 0},
        {{ 0,  0}, FALSE, FALSE, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
        {{10, 10}, FALSE, FALSE, 0},
        {{10, 10}, FALSE, FALSE, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
        {{ 0,  0}, FALSE, TRUE,  0},
        {{ 0,  0}, FALSE, TRUE,  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
        {{10, 10}, FALSE, TRUE,  0},
        {{10, 10}, FALSE, TRUE,  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
    };

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        swapchain_desc.Flags = tests[i].flags;
        swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0,
                tests[i].origin.x, tests[i].origin.y, 400, 200, 0, 0, 0, 0);
        if (tests[i].menu)
        {
            HMENU menu_bar = CreateMenu();
            HMENU menu = CreateMenu();
            AppendMenuA(menu_bar, MF_POPUP, (UINT_PTR)menu, "Menu");
            SetMenu(swapchain_desc.OutputWindow, menu_bar);
        }

        memset(&initial_state, 0, sizeof(initial_state));
        capture_fullscreen_state(&initial_state.fullscreen_state, swapchain_desc.OutputWindow);

        hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
        ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &initial_state);

        expected_state = initial_state;
        if (tests[i].fullscreen)
        {
            expected_state.fullscreen = TRUE;
            compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
                    &swapchain_desc, &initial_state.fullscreen_state.monitor_rect, 800, 600, NULL);
            hr = IDXGISwapChain_GetContainingOutput(swapchain, &expected_state.target);
            ok(SUCCEEDED(hr) || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
                    "GetContainingOutput failed, hr %#x.\n", hr);
            if (hr == DXGI_ERROR_UNSUPPORTED)
            {
                win_skip("GetContainingOutput() not supported.\n");
                IDXGISwapChain_Release(swapchain);
                DestroyWindow(swapchain_desc.OutputWindow);
                continue;
            }

            hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
            ok(SUCCEEDED(hr) || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE,
                    "SetFullscreenState failed, hr %#x.\n", hr);
            if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
            {
                skip("Could not change fullscreen state.\n");
                IDXGIOutput_Release(expected_state.target);
                IDXGISwapChain_Release(swapchain);
                DestroyWindow(swapchain_desc.OutputWindow);
                continue;
            }
        }
        check_swapchain_fullscreen_state(swapchain, &expected_state);

        hr = IDXGISwapChain_ResizeTarget(swapchain, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &expected_state);

        if (tests[i].fullscreen)
        {
            test_fullscreen_resize_target(swapchain, &expected_state);

            hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
            ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);
            check_swapchain_fullscreen_state(swapchain, &initial_state);
            IDXGIOutput_Release(expected_state.target);
            check_swapchain_fullscreen_state(swapchain, &initial_state);
            expected_state = initial_state;
        }
        else
        {
            test_windowed_resize_target(swapchain, swapchain_desc.OutputWindow, &expected_state);

            check_swapchain_fullscreen_state(swapchain, &expected_state);
        }

        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);
        check_window_fullscreen_state(swapchain_desc.OutputWindow, &expected_state.fullscreen_state);
        DestroyWindow(swapchain_desc.OutputWindow);
    }

    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %u.\n", refcount);
}

static LRESULT CALLBACK resize_target_wndproc(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam)
{
    IDXGISwapChain *swapchain = (IDXGISwapChain *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    DXGI_SWAP_CHAIN_DESC desc;
    HRESULT hr;

    switch (message)
    {
        case WM_SIZE:
            ok(!!swapchain, "GWLP_USERDATA is NULL.\n");
            hr = IDXGISwapChain_GetDesc(swapchain, &desc);
            ok(hr == S_OK, "Failed to get desc, hr %#x.\n", hr);
            ok(desc.BufferDesc.Width == 800, "Got unexpected buffer width %u.\n", desc.BufferDesc.Width);
            ok(desc.BufferDesc.Height == 600, "Got unexpected buffer height %u.\n", desc.BufferDesc.Height);
            return 0;

        default:
            return DefWindowProcA(hwnd, message, wparam, lparam);
    }
}

struct window_thread_data
{
    HWND window;
    HANDLE window_created;
    HANDLE finished;
};

static DWORD WINAPI window_thread(void *data)
{
    struct window_thread_data *thread_data = data;
    unsigned int ret;
    WNDCLASSA wc;
    MSG msg;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = resize_target_wndproc;
    wc.lpszClassName = "dxgi_resize_target_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_data->window = CreateWindowA("dxgi_resize_target_wndproc_wc", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    ok(!!thread_data->window, "Failed to create window.\n");

    ret = SetEvent(thread_data->window_created);
    ok(ret, "Failed to set event, last error %#x.\n", GetLastError());

    for (;;)
    {
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);

        ret = WaitForSingleObject(thread_data->finished, 0);
        if (ret != WAIT_TIMEOUT)
            break;
    }
    ok(ret == WAIT_OBJECT_0, "Failed to wait for event, ret %#x, last error %#x.\n", ret, GetLastError());

    DestroyWindow(thread_data->window);
    thread_data->window = NULL;

    UnregisterClassA("dxgi_test_wndproc_wc", GetModuleHandleA(NULL));

    return 0;
}

static void test_resize_target_wndproc(void)
{
    struct window_thread_data thread_data;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    DXGI_MODE_DESC mode;
    IDXGIDevice *device;
    unsigned int ret;
    ULONG refcount;
    LONG_PTR data;
    HANDLE thread;
    HRESULT hr;
    RECT rect;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    memset(&thread_data, 0, sizeof(thread_data));
    thread_data.window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_data.window_created, "Failed to create event, last error %#x.\n", GetLastError());
    thread_data.finished = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_data.finished, "Failed to create event, last error %#x.\n", GetLastError());

    thread = CreateThread(NULL, 0, window_thread, &thread_data, 0, NULL);
    ok(!!thread, "Failed to create thread, last error %#x.\n", GetLastError());
    ret = WaitForSingleObject(thread_data.window_created, INFINITE);
    ok(ret == WAIT_OBJECT_0, "Failed to wait for thread, ret %#x, last error %#x.\n", ret, GetLastError());

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Failed to get adapter, hr %#x.\n", hr);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Failed to get parent, hr %#x.\n", hr);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = thread_data.window;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);

    data = SetWindowLongPtrA(thread_data.window, GWLP_USERDATA, (LONG_PTR)swapchain);
    ok(!data, "Got unexpected GWLP_USERDATA %p.\n", (void *)data);

    memset(&mode, 0, sizeof(mode));
    mode.Width = 600;
    mode.Height = 400;
    hr = IDXGISwapChain_ResizeTarget(swapchain, &mode);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(hr == S_OK, "Getswapchain_desc failed, hr %#x.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 800,
            "Got unexpected buffer width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 600,
            "Got unexpected buffer height %u.\n", swapchain_desc.BufferDesc.Height);

    ret = GetClientRect(swapchain_desc.OutputWindow, &rect);
    ok(ret, "Failed to get client rect.\n");
    ok(rect.right == mode.Width && rect.bottom == mode.Height,
            "Got unexpected client rect %s.\n", wine_dbgstr_rect(&rect));

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);

    ret = SetEvent(thread_data.finished);
    ok(ret, "Failed to set event, last error %#x.\n", GetLastError());
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "Failed to wait for thread, ret %#x, last error %#x.\n", ret, GetLastError());
    CloseHandle(thread);
    CloseHandle(thread_data.window_created);
    CloseHandle(thread_data.finished);
}

static void test_inexact_modes(void)
{
    struct swapchain_fullscreen_state initial_state, expected_state;
    DXGI_SWAP_CHAIN_DESC swapchain_desc, result_desc;
    IDXGIOutput *output = NULL;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    static const struct
    {
        unsigned int width, height;
    }
    sizes[] =
    {
        {101, 101},
        {203, 204},
        {799, 601},
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = FALSE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    memset(&initial_state, 0, sizeof(initial_state));
    capture_fullscreen_state(&initial_state.fullscreen_state, swapchain_desc.OutputWindow);

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
    ok(SUCCEEDED(hr) || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
            "GetContainingOutput failed, hr %#x.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        win_skip("GetContainingOutput() not supported.\n");
        goto done;
    }
    if (result_desc.Windowed)
    {
        win_skip("Fullscreen not supported.\n");
        goto done;
    }

    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);

    for (i = 0; i < ARRAY_SIZE(sizes); ++i)
    {
        /* Test CreateSwapChain(). */
        swapchain_desc.BufferDesc.Width = sizes[i].width;
        swapchain_desc.BufferDesc.Height = sizes[i].height;
        swapchain_desc.Windowed = FALSE;

        expected_state = initial_state;
        compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
                &swapchain_desc, &initial_state.fullscreen_state.monitor_rect,
                sizes[i].width, sizes[i].height, output);

        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
        ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

        check_swapchain_fullscreen_state(swapchain, &expected_state);
        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);
        ok(result_desc.BufferDesc.Width == sizes[i].width, "Got width %u, expected %u.\n",
                result_desc.BufferDesc.Width, sizes[i].width);
        ok(result_desc.BufferDesc.Height == sizes[i].height, "Got height %u, expected %u.\n",
                result_desc.BufferDesc.Height, sizes[i].height);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &initial_state);

        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

        /* Test SetFullscreenState(). */
        swapchain_desc.BufferDesc.Width = sizes[i].width;
        swapchain_desc.BufferDesc.Height = sizes[i].height;
        swapchain_desc.Windowed = TRUE;

        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
        ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);

        check_swapchain_fullscreen_state(swapchain, &expected_state);
        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);
        ok(result_desc.BufferDesc.Width == sizes[i].width, "Got width %u, expected %u.\n",
                result_desc.BufferDesc.Width, sizes[i].width);
        ok(result_desc.BufferDesc.Height == sizes[i].height, "Got height %u, expected %u.\n",
                result_desc.BufferDesc.Height, sizes[i].height);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &initial_state);

        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

        /* Test ResizeTarget(). */
        swapchain_desc.BufferDesc.Width = 800;
        swapchain_desc.BufferDesc.Height = 600;
        swapchain_desc.Windowed = TRUE;

        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
        ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);

        swapchain_desc.BufferDesc.Width = sizes[i].width;
        swapchain_desc.BufferDesc.Height = sizes[i].height;
        hr = IDXGISwapChain_ResizeTarget(swapchain, &swapchain_desc.BufferDesc);
        ok(SUCCEEDED(hr), "ResizeTarget failed, hr %#x.\n", hr);

        check_swapchain_fullscreen_state(swapchain, &expected_state);
        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);
        ok(result_desc.BufferDesc.Width == 800, "Got width %u.\n", result_desc.BufferDesc.Width);
        ok(result_desc.BufferDesc.Height == 600, "Got height %u.\n", result_desc.BufferDesc.Height);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &initial_state);

        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);
    }

done:
    if (output)
        IDXGIOutput_Release(output);
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
}

static void test_create_factory(void)
{
    IUnknown *iface;
    ULONG refcount;
    HRESULT hr;

    iface = (void *)0xdeadbeef;
    hr = CreateDXGIFactory(&IID_IDXGIDevice, (void **)&iface);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    hr = CreateDXGIFactory(&IID_IUnknown, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IUnknown, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = CreateDXGIFactory(&IID_IDXGIObject, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IDXGIObject, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IDXGIFactory, hr %#x.\n", hr);
    check_interface(iface, &IID_IDXGIFactory1, FALSE, FALSE);
    IUnknown_Release(iface);

    iface = (void *)0xdeadbeef;
    hr = CreateDXGIFactory(&IID_IDXGIFactory1, (void **)&iface);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = NULL;
    hr = CreateDXGIFactory(&IID_IDXGIFactory2, (void **)&iface);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Got unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        refcount = IUnknown_Release(iface);
        ok(!refcount, "Factory has %u references left.\n", refcount);
    }

    if (!pCreateDXGIFactory1)
    {
        win_skip("CreateDXGIFactory1 not available.\n");
        return;
    }

    iface = (void *)0xdeadbeef;
    hr = pCreateDXGIFactory1(&IID_IDXGIDevice, (void **)&iface);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    hr = pCreateDXGIFactory1(&IID_IUnknown, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IUnknown, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = pCreateDXGIFactory1(&IID_IDXGIObject, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IDXGIObject, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = pCreateDXGIFactory1(&IID_IDXGIFactory, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IDXGIFactory, hr %#x.\n", hr);
    check_interface(iface, &IID_IDXGIFactory1, TRUE, FALSE);
    refcount = IUnknown_Release(iface);
    ok(!refcount, "Factory has %u references left.\n", refcount);

    hr = pCreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IDXGIFactory1, hr %#x.\n", hr);
    IUnknown_Release(iface);

    iface = NULL;
    hr = pCreateDXGIFactory1(&IID_IDXGIFactory2, (void **)&iface);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Got unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        refcount = IUnknown_Release(iface);
        ok(!refcount, "Factory has %u references left.\n", refcount);
    }

    if (!pCreateDXGIFactory2)
    {
        win_skip("CreateDXGIFactory2 not available.\n");
        return;
    }

    hr = pCreateDXGIFactory2(0, &IID_IDXGIFactory3, (void **)&iface);
    ok(hr == S_OK, "Failed to create factory, hr %#x.\n", hr);
    check_interface(iface, &IID_IDXGIFactory, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory1, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory2, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory3, TRUE, FALSE);
    /* Not available on all Windows versions. */
    check_interface(iface, &IID_IDXGIFactory4, TRUE, TRUE);
    check_interface(iface, &IID_IDXGIFactory5, TRUE, TRUE);
    refcount = IUnknown_Release(iface);
    ok(!refcount, "Factory has %u references left.\n", refcount);

    hr = pCreateDXGIFactory2(0, &IID_IDXGIFactory, (void **)&iface);
    ok(hr == S_OK, "Failed to create factory, hr %#x.\n", hr);
    check_interface(iface, &IID_IDXGIFactory, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory1, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory2, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory3, TRUE, FALSE);
    refcount = IUnknown_Release(iface);
    ok(!refcount, "Factory has %u references left.\n", refcount);
}

static void test_private_data(void)
{
    ULONG refcount, expected_refcount;
    IDXGIDevice *device;
    HRESULT hr;
    IDXGIDevice *test_object;
    IUnknown *ptr;
    static const DWORD data[] = {1, 2, 3, 4};
    UINT size;
    static const GUID dxgi_private_data_test_guid =
    {
        0xfdb37466,
        0x428f,
        0x4edf,
        {0xa3, 0x7f, 0x9b, 0x1d, 0xf4, 0x88, 0xc5, 0xfc}
    };
    static const GUID dxgi_private_data_test_guid2 =
    {
        0x2e5afac2,
        0x87b5,
        0x4c10,
        {0x9b, 0x4b, 0x89, 0xd7, 0xd1, 0x12, 0xe7, 0x2b}
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    test_object = create_device(0);

    /* SetPrivateData with a pointer of NULL has the purpose of FreePrivateData in previous
     * d3d versions. A successful clear returns S_OK. A redundant clear S_FALSE. Setting a
     * NULL interface is not considered a clear but as setting an interface pointer that
     * happens to be NULL. */
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 0, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, ~0U, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, ~0U, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(ptr) * 2;
    ptr = (IUnknown *)0xdeadbeef;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!ptr, "Got unexpected pointer %p.\n", ptr);
    ok(size == sizeof(IUnknown *), "Got unexpected size %u.\n", size);

    refcount = get_refcount(test_object);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount + 1;
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount--;
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(data);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, size, data);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 42, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 42, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount++;
    size = 2 * sizeof(ptr);
    ptr = NULL;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(test_object), "Got unexpected size %u.\n", size);
    expected_refcount++;
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    if (ptr)
        IUnknown_Release(ptr);
    expected_refcount--;

    ptr = (IUnknown *)0xdeadbeef;
    size = 1;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    size = 2 * sizeof(ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    size = 1;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid2, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    size = 0xdeadbabe;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid2, &size, &ptr);
    ok(hr == DXGI_ERROR_NOT_FOUND, "Got unexpected hr %#x.\n", hr);
    ok(size == 0, "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, NULL, &ptr);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIDevice_Release(test_object);
    ok(!refcount, "Test object has %u references left.\n", refcount);
}

#define check_surface_desc(a, b) check_surface_desc_(__LINE__, a, b)
static void check_surface_desc_(unsigned int line, IDXGISurface *surface,
        const DXGI_SWAP_CHAIN_DESC *swapchain_desc)
{
    DXGI_SURFACE_DESC surface_desc;
    HRESULT hr;

    hr = IDXGISurface_GetDesc(surface, &surface_desc);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get surface desc, hr %#x.\n", hr);
    ok_(__FILE__, line)(surface_desc.Width == swapchain_desc->BufferDesc.Width,
            "Got Width %u, expected %u.\n", surface_desc.Width, swapchain_desc->BufferDesc.Width);
    ok_(__FILE__, line)(surface_desc.Height == swapchain_desc->BufferDesc.Height,
            "Got Height %u, expected %u.\n", surface_desc.Height, swapchain_desc->BufferDesc.Height);
    ok_(__FILE__, line)(surface_desc.Format == swapchain_desc->BufferDesc.Format,
            "Got Format %#x, expected %#x.\n", surface_desc.Format, swapchain_desc->BufferDesc.Format);
    ok_(__FILE__, line)(surface_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", surface_desc.SampleDesc.Count);
    ok_(__FILE__, line)(!surface_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", surface_desc.SampleDesc.Quality);
}

#define check_texture_desc(a, b) check_texture_desc_(__LINE__, a, b)
static void check_texture_desc_(unsigned int line, ID3D10Texture2D *texture,
        const DXGI_SWAP_CHAIN_DESC *swapchain_desc)
{
    D3D10_TEXTURE2D_DESC texture_desc;

    ID3D10Texture2D_GetDesc(texture, &texture_desc);
    ok_(__FILE__, line)(texture_desc.Width == swapchain_desc->BufferDesc.Width,
            "Got Width %u, expected %u.\n", texture_desc.Width, swapchain_desc->BufferDesc.Width);
    ok_(__FILE__, line)(texture_desc.Height == swapchain_desc->BufferDesc.Height,
            "Got Height %u, expected %u.\n", texture_desc.Height, swapchain_desc->BufferDesc.Height);
    ok_(__FILE__, line)(texture_desc.MipLevels == 1, "Got unexpected MipLevels %u.\n", texture_desc.MipLevels);
    ok_(__FILE__, line)(texture_desc.ArraySize == 1, "Got unexpected ArraySize %u.\n", texture_desc.ArraySize);
    ok_(__FILE__, line)(texture_desc.Format == swapchain_desc->BufferDesc.Format,
            "Got Format %#x, expected %#x.\n", texture_desc.Format, swapchain_desc->BufferDesc.Format);
    ok_(__FILE__, line)(texture_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", texture_desc.SampleDesc.Count);
    ok_(__FILE__, line)(!texture_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", texture_desc.SampleDesc.Quality);
    ok_(__FILE__, line)(texture_desc.Usage == D3D10_USAGE_DEFAULT,
            "Got unexpected Usage %#x.\n", texture_desc.Usage);
    ok_(__FILE__, line)(texture_desc.BindFlags == D3D10_BIND_RENDER_TARGET,
            "Got unexpected BindFlags %#x.\n", texture_desc.BindFlags);
    ok_(__FILE__, line)(!texture_desc.CPUAccessFlags,
            "Got unexpected CPUAccessFlags %#x.\n", texture_desc.CPUAccessFlags);
    ok_(__FILE__, line)(!texture_desc.MiscFlags, "Got unexpected MiscFlags %#x.\n", texture_desc.MiscFlags);
}

#define check_resource_desc(a, b) check_resource_desc_(__LINE__, a, b)
static void check_resource_desc_(unsigned int line, ID3D12Resource *resource,
        const DXGI_SWAP_CHAIN_DESC *swapchain_desc)
{
    D3D12_RESOURCE_DESC resource_desc;

    resource_desc = ID3D12Resource_GetDesc(resource);
    ok_(__FILE__, line)(resource_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            "Got unexpected Dimension %#x.\n", resource_desc.Dimension);
    ok_(__FILE__, line)(resource_desc.Width == swapchain_desc->BufferDesc.Width, "Got Width %s, expected %u.\n",
            wine_dbgstr_longlong(resource_desc.Width), swapchain_desc->BufferDesc.Width);
    ok_(__FILE__, line)(resource_desc.Height == swapchain_desc->BufferDesc.Height,
            "Got Height %u, expected %u.\n", resource_desc.Height, swapchain_desc->BufferDesc.Height);
    ok_(__FILE__, line)(resource_desc.DepthOrArraySize == 1,
            "Got unexpected DepthOrArraySize %u.\n", resource_desc.DepthOrArraySize);
    ok_(__FILE__, line)(resource_desc.MipLevels == 1,
            "Got unexpected MipLevels %u.\n", resource_desc.MipLevels);
    ok_(__FILE__, line)(resource_desc.Format == swapchain_desc->BufferDesc.Format,
            "Got Format %#x, expected %#x.\n", resource_desc.Format, swapchain_desc->BufferDesc.Format);
    ok_(__FILE__, line)(resource_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", resource_desc.SampleDesc.Count);
    ok_(__FILE__, line)(!resource_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", resource_desc.SampleDesc.Quality);
    ok_(__FILE__, line)(resource_desc.Layout == D3D12_TEXTURE_LAYOUT_UNKNOWN,
            "Got unexpected Layout %#x.\n", resource_desc.Layout);
}

static void test_swapchain_resize(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_SWAP_EFFECT swap_effect;
    IDXGISwapChain *swapchain;
    ID3D12Resource *resource;
    ID3D10Texture2D *texture;
    HRESULT hr, expected_hr;
    IDXGISurface *surface;
    IDXGIFactory *factory;
    RECT client_rect, r;
    ULONG refcount;
    HWND window;
    BOOL ret;

    get_factory(device, is_d3d12, &factory);

    window = CreateWindowA("static", "dxgi_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ret = GetClientRect(window, &client_rect);
    ok(ret, "Failed to get client rect.\n");

    swap_effect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;

    swapchain_desc.BufferDesc.Width = 640;
    swapchain_desc.BufferDesc.Height = 480;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 2;
    swapchain_desc.OutputWindow = window;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = swap_effect;
    swapchain_desc.Flags = 0;

    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
    expected_hr = is_d3d12 ? E_NOINTERFACE : S_OK;
    ok(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    ok(!surface || hr == S_OK, "Got unexpected pointer %p.\n", surface);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D10Texture2D, (void **)&texture);
    ok(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    ok(!texture || hr == S_OK, "Got unexpected pointer %p.\n", texture);
    expected_hr = is_d3d12 ? S_OK : E_NOINTERFACE;
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D12Resource, (void **)&resource);
    ok(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    ok(!resource || hr == S_OK, "Got unexpected pointer %p.\n", resource);

    ret = GetClientRect(window, &r);
    ok(ret, "Failed to get client rect.\n");
    ok(EqualRect(&r, &client_rect), "Got unexpected rect %s, expected %s.\n",
            wine_dbgstr_rect(&r), wine_dbgstr_rect(&client_rect));

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(SUCCEEDED(hr), "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 640,
            "Got unexpected BufferDesc.Width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 480,
            "Got unexpected bufferDesc.Height %u.\n", swapchain_desc.BufferDesc.Height);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 2,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == swap_effect,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    if (surface)
        check_surface_desc(surface, &swapchain_desc);
    if (texture)
        check_texture_desc(texture, &swapchain_desc);
    if (resource)
        check_resource_desc(resource, &swapchain_desc);

    hr = IDXGISwapChain_ResizeBuffers(swapchain, 2, 320, 240, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    ret = GetClientRect(window, &r);
    ok(ret, "Failed to get client rect.\n");
    ok(EqualRect(&r, &client_rect), "Got unexpected rect %s, expected %s.\n",
            wine_dbgstr_rect(&r), wine_dbgstr_rect(&client_rect));

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(SUCCEEDED(hr), "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 640,
            "Got unexpected BufferDesc.Width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 480,
            "Got unexpected bufferDesc.Height %u.\n", swapchain_desc.BufferDesc.Height);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 2,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == swap_effect,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    if (surface)
    {
        check_surface_desc(surface, &swapchain_desc);
        IDXGISurface_Release(surface);
    }
    if (texture)
    {
        check_texture_desc(texture, &swapchain_desc);
        ID3D10Texture2D_Release(texture);
    }
    if (resource)
    {
        check_resource_desc(resource, &swapchain_desc);
        ID3D12Resource_Release(resource);
    }

    hr = IDXGISwapChain_ResizeBuffers(swapchain, 2, 320, 240, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    ok(hr == S_OK, "Failed to resize buffers, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
    expected_hr = is_d3d12 ? E_NOINTERFACE : S_OK;
    ok(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    ok(!surface || hr == S_OK, "Got unexpected pointer %p.\n", surface);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D10Texture2D, (void **)&texture);
    ok(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    ok(!texture || hr == S_OK, "Got unexpected pointer %p.\n", texture);
    expected_hr = is_d3d12 ? S_OK : E_NOINTERFACE;
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D12Resource, (void **)&resource);
    ok(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    ok(!resource || hr == S_OK, "Got unexpected pointer %p.\n", resource);

    ret = GetClientRect(window, &r);
    ok(ret, "Failed to get client rect.\n");
    ok(EqualRect(&r, &client_rect), "Got unexpected rect %s, expected %s.\n",
            wine_dbgstr_rect(&r), wine_dbgstr_rect(&client_rect));

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(SUCCEEDED(hr), "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 320,
            "Got unexpected BufferDesc.Width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 240,
            "Got unexpected bufferDesc.Height %u.\n", swapchain_desc.BufferDesc.Height);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 2,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == swap_effect,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    if (surface)
    {
        check_surface_desc(surface, &swapchain_desc);
        IDXGISurface_Release(surface);
    }
    if (texture)
    {
        check_texture_desc(texture, &swapchain_desc);
        ID3D10Texture2D_Release(texture);
    }
    if (resource)
    {
        check_resource_desc(resource, &swapchain_desc);
        ID3D12Resource_Release(resource);
    }

    hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    ok(hr == S_OK, "Failed to resize buffers, hr %#x.\n", hr);

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(SUCCEEDED(hr), "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == client_rect.right - client_rect.left,
            "Got unexpected BufferDesc.Width %u, expected %u.\n",
            swapchain_desc.BufferDesc.Width, client_rect.right - client_rect.left);
    ok(swapchain_desc.BufferDesc.Height == client_rect.bottom - client_rect.top,
            "Got unexpected bufferDesc.Height %u, expected %u.\n",
            swapchain_desc.BufferDesc.Height, client_rect.bottom - client_rect.top);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 2,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == swap_effect,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    IDXGISwapChain_Release(swapchain);
    DestroyWindow(window);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %u.\n", refcount);
}

static void test_swapchain_parameters(void)
{
    DXGI_USAGE usage, expected_usage, broken_usage;
    D3D10_TEXTURE2D_DESC d3d10_texture_desc;
    D3D11_TEXTURE2D_DESC d3d11_texture_desc;
    unsigned int expected_bind_flags;
    ID3D10Texture2D *d3d10_texture;
    ID3D11Texture2D *d3d11_texture;
    DXGI_SWAP_CHAIN_DESC desc;
    IDXGISwapChain *swapchain;
    IDXGIResource *resource;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    IDXGIDevice *device;
    unsigned int i, j;
    ULONG refcount;
    IUnknown *obj;
    HWND window;
    HRESULT hr;

    static const struct
    {
        BOOL windowed;
        UINT buffer_count;
        DXGI_SWAP_EFFECT swap_effect;
        HRESULT hr, vista_hr;
        UINT highest_accessible_buffer;
    }
    tests[] =
    {
        {TRUE,   0, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {TRUE,   2, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {TRUE,   0, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     0},
        {TRUE,   2, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     1},
        {TRUE,   3, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     2},
        {TRUE,   0, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   0, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  1},
        {TRUE,   3, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  2},
        {TRUE,   0, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   0, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  16, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {TRUE,  16, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                    15},
        {TRUE,  16, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL, 15},
        {TRUE,  16, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},

        {FALSE,  0, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {FALSE,  2, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {FALSE,  0, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     0},
        {FALSE,  2, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     1},
        {FALSE,  3, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     2},
        {FALSE,  0, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  2, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  0, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  2, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  1},
        {FALSE,  3, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  2},
        {FALSE,  0, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  2, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  0, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  2, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 16, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {FALSE, 16, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                    15},
        {FALSE, 16, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL, 15},
        /* The following test fails on Nvidia with E_OUTOFMEMORY and leaks device references in the
         * process. Disable it for now.
        {FALSE, 16, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
         */
        {FALSE, 17, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 17, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 17, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 17, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
    };
    static const DXGI_USAGE usage_tests[] =
    {
        0,
        DXGI_USAGE_BACK_BUFFER,
        DXGI_USAGE_SHADER_INPUT,
        DXGI_USAGE_RENDER_TARGET_OUTPUT,
        DXGI_USAGE_DISCARD_ON_PRESENT,
        DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER,
        DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_DISCARD_ON_PRESENT,
        DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_DISCARD_ON_PRESENT,
        DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT,
        DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_DISCARD_ON_PRESENT,
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }
    window = CreateWindowA("static", "dxgi_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, 0, 0, 0, 0);

    hr = IDXGIDevice_QueryInterface(device, &IID_IUnknown, (void **)&obj);
    ok(hr == S_OK, "IDXGIDevice does not implement IUnknown.\n");

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Failed to get adapter, hr %#x.\n", hr);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Failed to get parent, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        memset(&desc, 0, sizeof(desc));
        desc.BufferDesc.Width = registry_mode.dmPelsWidth;
        desc.BufferDesc.Height = registry_mode.dmPelsHeight;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = window;

        desc.Windowed = tests[i].windowed;
        desc.BufferCount = tests[i].buffer_count;
        desc.SwapEffect = tests[i].swap_effect;

        hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
        ok(hr == tests[i].hr || broken(hr == tests[i].vista_hr)
                || (SUCCEEDED(tests[i].hr) && hr == DXGI_STATUS_OCCLUDED),
                "Got unexpected hr %#x, test %u.\n", hr, i);
        if (FAILED(hr))
            continue;

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGIResource, (void **)&resource);
        todo_wine ok(SUCCEEDED(hr), "GetBuffer(0) failed, hr %#x, test %u.\n", hr, i);
        if (FAILED(hr))
        {
            hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
            ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);

            IDXGISwapChain_Release(swapchain);
            continue;
        }

        expected_usage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
        if (tests[i].swap_effect == DXGI_SWAP_EFFECT_DISCARD)
            expected_usage |= DXGI_USAGE_DISCARD_ON_PRESENT;
        hr = IDXGIResource_GetUsage(resource, &usage);
        ok(SUCCEEDED(hr), "Failed to get resource usage, hr %#x, test %u.\n", hr, i);
        ok(usage == expected_usage, "Got usage %x, expected %x, test %u.\n", usage, expected_usage, i);

        IDXGIResource_Release(resource);

        hr = IDXGISwapChain_GetDesc(swapchain, &desc);
        ok(SUCCEEDED(hr), "Failed to get swapchain desc, hr %#x.\n", hr);

        for (j = 1; j <= tests[i].highest_accessible_buffer; j++)
        {
            hr = IDXGISwapChain_GetBuffer(swapchain, j, &IID_IDXGIResource, (void **)&resource);
            ok(SUCCEEDED(hr), "GetBuffer(%u) failed, hr %#x, test %u.\n", hr, i, j);

            /* Buffers > 0 are supposed to be read only. This is the case except that in
             * fullscreen mode on Windows <= 8 the last backbuffer (BufferCount - 1) is
             * writable. This is not the case if an unsupported refresh rate is passed
             * for some reason, probably because the invalid refresh rate triggers a
             * kinda-sorta windowed mode.
             *
             * On Windows 10 all buffers > 0 are read-only. Mark the earlier behavior
             * broken.
             *
             * This last buffer acts as a shadow frontbuffer. Writing to it doesn't show
             * the draw on the screen right away (Aero on or off doesn't matter), but
             * Present with DXGI_PRESENT_DO_NOT_SEQUENCE will show the modifications.
             *
             * Note that if the application doesn't have focused creating a fullscreen
             * swapchain returns DXGI_STATUS_OCCLUDED and we get a windowed swapchain,
             * so use the Windowed property of the swapchain that was actually created. */
            expected_usage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_READ_ONLY;
            broken_usage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;

            if (desc.Windowed || j < tests[i].highest_accessible_buffer)
                broken_usage |= DXGI_USAGE_READ_ONLY;

            hr = IDXGIResource_GetUsage(resource, &usage);
            ok(SUCCEEDED(hr), "Failed to get resource usage, hr %#x, test %u, buffer %u.\n", hr, i, j);
            ok(usage == expected_usage || broken(usage == broken_usage),
                    "Got usage %x, expected %x, test %u, buffer %u.\n",
                    usage, expected_usage, i, j);

            IDXGIResource_Release(resource);
        }
        hr = IDXGISwapChain_GetBuffer(swapchain, j, &IID_IDXGIResource, (void **)&resource);
        ok(hr == DXGI_ERROR_INVALID_CALL, "GetBuffer(%u) returned unexpected hr %#x, test %u.\n", j, hr, i);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);

        IDXGISwapChain_Release(swapchain);
    }

    for (i = 0; i < ARRAY_SIZE(usage_tests); ++i)
    {
        usage = usage_tests[i];

        memset(&desc, 0, sizeof(desc));
        desc.BufferDesc.Width = registry_mode.dmPelsWidth;
        desc.BufferDesc.Height = registry_mode.dmPelsHeight;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = usage;
        desc.BufferCount = 1;
        desc.OutputWindow = window;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#x, test %u.\n", hr, i);

        hr = IDXGISwapChain_GetDesc(swapchain, &desc);
        ok(hr == S_OK, "Failed to get swapchain desc, hr %#x, test %u.\n", hr, i);
        todo_wine_if(usage & ~(DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT))
        ok(desc.BufferUsage == usage, "Got usage %#x, expected %#x, test %u.\n", desc.BufferUsage, usage, i);

        expected_bind_flags = 0;
        if (usage & DXGI_USAGE_RENDER_TARGET_OUTPUT)
            expected_bind_flags |= D3D11_BIND_RENDER_TARGET;
        if (usage & DXGI_USAGE_SHADER_INPUT)
            expected_bind_flags |= D3D11_BIND_SHADER_RESOURCE;

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D10Texture2D, (void **)&d3d10_texture);
        ok(hr == S_OK, "Failed to get d3d10 texture, hr %#x, test %u.\n", hr, i);
        ID3D10Texture2D_GetDesc(d3d10_texture, &d3d10_texture_desc);
        ok(d3d10_texture_desc.BindFlags == expected_bind_flags,
                "Got d3d10 bind flags %#x, expected %#x, test %u.\n",
                d3d10_texture_desc.BindFlags, expected_bind_flags, i);
        ID3D10Texture2D_Release(d3d10_texture);

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, (void **)&d3d11_texture);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Failed to get d3d11 texture, hr %#x, test %u.\n", hr, i);
        if (SUCCEEDED(hr))
        {
            ID3D11Texture2D_GetDesc(d3d11_texture, &d3d11_texture_desc);
            ok(d3d11_texture_desc.BindFlags == expected_bind_flags,
                    "Got d3d11 bind flags %#x, expected %#x, test %u.\n",
                    d3d11_texture_desc.BindFlags, expected_bind_flags, i);
            ID3D11Texture2D_Release(d3d11_texture);
        }

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGIResource, (void **)&resource);
        todo_wine ok(hr == S_OK, "Failed to get buffer, hr %#x, test %u.\n", hr, i);
        if (FAILED(hr))
        {
            IDXGISwapChain_Release(swapchain);
            continue;
        }
        expected_usage = usage | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_DISCARD_ON_PRESENT;
        hr = IDXGIResource_GetUsage(resource, &usage);
        ok(hr == S_OK, "Failed to get resource usage, hr %#x, test %u.\n", hr, i);
        ok(usage == expected_usage, "Got usage %x, expected %x, test %u.\n", usage, expected_usage, i);
        IDXGIResource_Release(resource);

        IDXGISwapChain_Release(swapchain);
    }

    /* multisampling */
    memset(&desc, 0, sizeof(desc));
    desc.BufferDesc.Width = registry_mode.dmPelsWidth;
    desc.BufferDesc.Height = registry_mode.dmPelsHeight;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 4;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 4;
    desc.OutputWindow = window;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    if (check_multisample_quality_levels(device, desc.BufferDesc.Format, desc.SampleDesc.Count))
    {
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        IDXGISwapChain_Release(swapchain);
        desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
        hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        IDXGISwapChain_Release(swapchain);
    }
    else
    {
        skip("Multisampling not supported for DXGI_FORMAT_R8G8B8A8_UNORM.\n");
    }

    IDXGIFactory_Release(factory);
    IUnknown_Release(obj);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_swapchain_present(IUnknown *device, BOOL is_d3d12)
{
    static const DWORD flags[] = {0, DXGI_PRESENT_TEST};
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIOutput *output;
    BOOL fullscreen;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);

    for (i = 0; i < 10; ++i)
    {
        hr = IDXGISwapChain_Present(swapchain, i, 0);
        ok(hr == (i <= 4 ? S_OK : DXGI_ERROR_INVALID_CALL),
                "Got unexpected hr %#x for sync interval %u.\n", hr, i);
    }
    hr = IDXGISwapChain_Present(swapchain, 0, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(flags); ++i)
    {
        HWND occluding_window = CreateWindowA("static", "occluding_window",
                WS_POPUP | WS_VISIBLE, 0, 0, 400, 200, NULL, NULL, NULL, NULL);

        /* Another window covers the swapchain window. Not reported as occluded. */
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        /* Minimised window. */
        ShowWindow(swapchain_desc.OutputWindow, SW_MINIMIZE);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == (is_d3d12 ? S_OK : DXGI_STATUS_OCCLUDED), "Test %u: Got unexpected hr %#x.\n", i, hr);
        ShowWindow(swapchain_desc.OutputWindow, SW_NORMAL);

        /* Hidden window. */
        ShowWindow(swapchain_desc.OutputWindow, SW_HIDE);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ShowWindow(swapchain_desc.OutputWindow, SW_SHOW);
        DestroyWindow(occluding_window);

        /* Test that IDXGIOutput_ReleaseOwnership() makes the swapchain exit
         * fullscreen. */
        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
        /* DXGI_ERROR_NOT_CURRENTLY_AVAILABLE on some machines.
         * DXGI_ERROR_UNSUPPORTED on the Windows 7 testbot. */
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE || broken(hr == DXGI_ERROR_UNSUPPORTED))
        {
            skip("Test %u: Could not change fullscreen state.\n", i);
            continue;
        }
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        todo_wine_if(!is_d3d12) ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        output = NULL;
        fullscreen = FALSE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &output);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);
        ok(!!output, "Test %u: Got unexpected output.\n", i);

        if (output)
            IDXGIOutput_ReleaseOwnership(output);
        /* Still fullscreen. */
        fullscreen = FALSE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);
        /* Calling IDXGISwapChain_Present() will exit fullscreen. */
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        fullscreen = TRUE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        /* Now fullscreen mode is exited. */
        if (!flags[i] && !is_d3d12)
            /* Still fullscreen on vista and 2008. */
            todo_wine ok(!fullscreen || broken(fullscreen), "Test %u: Got unexpected fullscreen status.\n", i);
        else
            ok(fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);
        if (output)
            IDXGIOutput_Release(output);

        /* Test creating a window when swapchain is in fullscreen.
         *
         * The window should break the swapchain out of fullscreen mode on
         * d3d10/11. D3d12 is different, a new occluding window doesn't break
         * the swapchain out of fullscreen because d3d12 fullscreen swapchains
         * don't take exclusive ownership over the output, nor do they disable
         * compositing. D3d12 fullscreen mode acts just like borderless
         * fullscreen window mode. */
        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        fullscreen = FALSE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        todo_wine_if(!is_d3d12) ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        occluding_window = CreateWindowA("static", "occluding_window", WS_POPUP, 0, 0, 400, 200, 0, 0, 0, 0);
        /* An invisible window doesn't cause the swapchain to exit fullscreen
         * mode. */
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        todo_wine_if(!is_d3d12) ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        fullscreen = FALSE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);
        /* A visible, but with bottom z-order window still causes the
         * swapchain to exit fullscreen mode. */
        SetWindowPos(occluding_window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        ShowWindow(occluding_window, SW_SHOW);
        /* Fullscreen mode takes a while to exit. */
        if (!is_d3d12)
            wait_fullscreen_state(swapchain, FALSE, TRUE);

        /* No longer fullscreen before calling IDXGISwapChain_Present() except
         * for d3d12. */
        fullscreen = TRUE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        todo_wine_if(!is_d3d12) ok(is_d3d12 ? fullscreen : !fullscreen,
                "Test %u: Got unexpected fullscreen status.\n", i);

        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        todo_wine_if(!is_d3d12) ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        todo_wine_if(is_d3d12) ok(hr == (is_d3d12 ? DXGI_STATUS_OCCLUDED : S_OK),
                "Test %u: Got unexpected hr %#x.\n", i, hr);

        fullscreen = TRUE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        if (flags[i] == DXGI_PRESENT_TEST)
            todo_wine_if(!is_d3d12) ok(is_d3d12 ? fullscreen : !fullscreen,
                    "Test %u: Got unexpected fullscreen status.\n", i);
        else
            todo_wine ok(!fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);

        /* Even though d3d12 doesn't exit fullscreen, a
         * IDXGISwapChain_ResizeBuffers() is still needed for subsequent
         * IDXGISwapChain_Present() calls to work, otherwise they will return
         * DXGI_ERROR_INVALID_CALL */
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        todo_wine_if(!is_d3d12) ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        if (flags[i] == DXGI_PRESENT_TEST)
            todo_wine_if(is_d3d12) ok(hr == (is_d3d12 ? DXGI_STATUS_OCCLUDED : S_OK),
                    "Test %u: Got unexpected hr %#x.\n", i, hr);
        else
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        /* Trying to break out of fullscreen mode again. This time, don't call
         * IDXGISwapChain_GetFullscreenState() before IDXGISwapChain_Present(). */
        ShowWindow(occluding_window, SW_HIDE);
        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ShowWindow(occluding_window, SW_SHOW);

        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        todo_wine_if(!is_d3d12) ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        /* hr == S_OK on vista and 2008 */
        todo_wine ok(hr == DXGI_STATUS_OCCLUDED || broken(hr == S_OK),
                "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        todo_wine_if(!is_d3d12) ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        if (flags[i] == DXGI_PRESENT_TEST)
        {
            todo_wine ok(hr == DXGI_STATUS_OCCLUDED || broken(hr == S_OK),
                    "Test %u: Got unexpected hr %#x.\n", i, hr);
            /* IDXGISwapChain_Present() without flags refreshes the occlusion
             * state. */
            hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            todo_wine_if(!is_d3d12) ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
            hr = IDXGISwapChain_Present(swapchain, 0, 0);
            todo_wine ok(hr == DXGI_STATUS_OCCLUDED || broken(hr == S_OK),
                    "Test %u: Got unexpected hr %#x.\n", i, hr);
            hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            todo_wine_if(!is_d3d12) ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
            hr = IDXGISwapChain_Present(swapchain, 0, DXGI_PRESENT_TEST);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        }
        else
        {
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        }
        fullscreen = TRUE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        todo_wine ok(!fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);

        DestroyWindow(occluding_window);
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        todo_wine_if(!is_d3d12) ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
    }

    wait_device_idle(device);

    IDXGISwapChain_Release(swapchain);
    DestroyWindow(swapchain_desc.OutputWindow);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %u.\n", refcount);
}

static void test_swapchain_backbuffer_index(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    unsigned int index, expected_index;
    IDXGISwapChain3 *swapchain3;
    IDXGISwapChain *swapchain;
    HRESULT hr, expected_hr;
    IDXGIFactory *factory;
    unsigned int i, j;
    ULONG refcount;
    RECT rect;
    BOOL ret;

    static const DXGI_SWAP_EFFECT swap_effects[] =
    {
        DXGI_SWAP_EFFECT_DISCARD,
        DXGI_SWAP_EFFECT_SEQUENTIAL,
        DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
        DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 4;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    ret = GetClientRect(swapchain_desc.OutputWindow, &rect);
    ok(ret, "Failed to get client rect.\n");
    swapchain_desc.BufferDesc.Width = rect.right;
    swapchain_desc.BufferDesc.Height = rect.bottom;

    for (i = 0; i < ARRAY_SIZE(swap_effects); ++i)
    {
        swapchain_desc.SwapEffect = swap_effects[i];
        expected_hr = is_d3d12 && !is_flip_model(swap_effects[i]) ? DXGI_ERROR_INVALID_CALL : S_OK;
        hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
        ok(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
        if (FAILED(hr))
            continue;

        hr = IDXGISwapChain_QueryInterface(swapchain, &IID_IDXGISwapChain3, (void **)&swapchain3);
        if (hr == E_NOINTERFACE)
        {
            skip("IDXGISwapChain3 is not supported.\n");
            IDXGISwapChain_Release(swapchain);
            goto done;
        }

        for (j = 0; j < 2 * swapchain_desc.BufferCount; ++j)
        {
            index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain3);
            expected_index = is_d3d12 ? j % swapchain_desc.BufferCount : 0;
            ok(index == expected_index, "Got back buffer index %u, expected %u.\n", index, expected_index);
            hr = IDXGISwapChain3_Present(swapchain3, 0, 0);
            ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        }

        wait_device_idle(device);

        IDXGISwapChain3_Release(swapchain3);
        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "Swapchain has %u references left.\n", refcount);
    }

done:
    DestroyWindow(swapchain_desc.OutputWindow);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %u.\n", refcount);
}

static void test_swapchain_formats(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    HRESULT hr, expected_hr;
    IDXGIFactory *factory;
    unsigned int i;
    ULONG refcount;
    RECT rect;
    BOOL ret;

    static const struct
    {
        DXGI_FORMAT format;
        DXGI_SWAP_EFFECT swap_effect;
        BOOL supported;
    }
    tests[] =
    {
        {DXGI_FORMAT_UNKNOWN,                    DXGI_SWAP_EFFECT_DISCARD,         FALSE},
        {DXGI_FORMAT_UNKNOWN,                    DXGI_SWAP_EFFECT_SEQUENTIAL,      FALSE},
        {DXGI_FORMAT_UNKNOWN,                    DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, FALSE},
        {DXGI_FORMAT_UNKNOWN,                    DXGI_SWAP_EFFECT_FLIP_DISCARD,    FALSE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,             DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,             DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,             DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,             DXGI_SWAP_EFFECT_FLIP_DISCARD,    TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, FALSE},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_FLIP_DISCARD,    FALSE},
        {DXGI_FORMAT_B8G8R8A8_UNORM,             DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM,             DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM,             DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM,             DXGI_SWAP_EFFECT_FLIP_DISCARD,    TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, FALSE},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_FLIP_DISCARD,    FALSE},
        {DXGI_FORMAT_R10G10B10A2_UNORM,          DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_R10G10B10A2_UNORM,          DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_R10G10B10A2_UNORM,          DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, TRUE},
        {DXGI_FORMAT_R10G10B10A2_UNORM,          DXGI_SWAP_EFFECT_FLIP_DISCARD,    TRUE},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,         DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,         DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,         DXGI_SWAP_EFFECT_FLIP_DISCARD,    TRUE},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,         DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, TRUE},
        {DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, DXGI_SWAP_EFFECT_FLIP_DISCARD,    FALSE},
        {DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, FALSE},
    };

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 4;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.Flags = 0;

    ret = GetClientRect(swapchain_desc.OutputWindow, &rect);
    ok(ret, "Failed to get client rect.\n");
    swapchain_desc.BufferDesc.Width = rect.right;
    swapchain_desc.BufferDesc.Height = rect.bottom;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        if (is_d3d12 && !is_flip_model(tests[i].swap_effect))
            continue;

        swapchain_desc.BufferDesc.Format = tests[i].format;
        swapchain_desc.SwapEffect = tests[i].swap_effect;
        hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
        expected_hr = tests[i].supported ? S_OK : DXGI_ERROR_INVALID_CALL;
        if (tests[i].format == DXGI_FORMAT_UNKNOWN && !is_d3d12)
            expected_hr = E_INVALIDARG;
        ok(hr == expected_hr
                /* Flip presentation model not supported. */
                || broken(hr == DXGI_ERROR_INVALID_CALL && is_flip_model(tests[i].swap_effect) && !is_d3d12),
                "Test %u, d3d12 %#x: Got hr %#x, expected %#x.\n", i, is_d3d12, hr, expected_hr);

        if (SUCCEEDED(hr))
        {
            refcount = IDXGISwapChain_Release(swapchain);
            ok(!refcount, "Swapchain has %u references left.\n", refcount);
        }
    }

    DestroyWindow(swapchain_desc.OutputWindow);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %u.\n", refcount);
}

static void test_maximum_frame_latency(void)
{
    IDXGIDevice1 *device1;
    IDXGIDevice *device;
    UINT max_latency;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    if (SUCCEEDED(IDXGIDevice_QueryInterface(device, &IID_IDXGIDevice1, (void **)&device1)))
    {
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(max_latency == DEFAULT_FRAME_LATENCY, "Got unexpected maximum frame latency %u.\n", max_latency);

        hr = IDXGIDevice1_SetMaximumFrameLatency(device1, MAX_FRAME_LATENCY);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(max_latency == MAX_FRAME_LATENCY, "Got unexpected maximum frame latency %u.\n", max_latency);

        hr = IDXGIDevice1_SetMaximumFrameLatency(device1, MAX_FRAME_LATENCY + 1);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(max_latency == MAX_FRAME_LATENCY, "Got unexpected maximum frame latency %u.\n", max_latency);

        hr = IDXGIDevice1_SetMaximumFrameLatency(device1, 0);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        /* 0 does not reset to the default frame latency on all Windows versions. */
        ok(max_latency == DEFAULT_FRAME_LATENCY || broken(!max_latency),
                "Got unexpected maximum frame latency %u.\n", max_latency);

        IDXGIDevice1_Release(device1);
    }
    else
    {
        win_skip("IDXGIDevice1 is not implemented.\n");
    }

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_output_desc(void)
{
    IDXGIAdapter *adapter, *adapter2;
    IDXGIOutput *output, *output2;
    DXGI_OUTPUT_DESC desc;
    IDXGIFactory *factory;
    unsigned int i, j;
    ULONG refcount;
    HRESULT hr;

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create DXGI factory, hr %#x.\n", hr);

    for (i = 0; ; ++i)
    {
        hr = IDXGIFactory_EnumAdapters(factory, i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;
        ok(SUCCEEDED(hr), "Failed to enumerate adapter %u, hr %#x.\n", i, hr);

        hr = IDXGIFactory_EnumAdapters(factory, i, &adapter2);
        ok(SUCCEEDED(hr), "Failed to enumerate adapter %u, hr %#x.\n", i, hr);
        ok(adapter != adapter2, "Expected to get new instance of IDXGIAdapter, %p == %p.\n", adapter, adapter2);
        refcount = get_refcount(adapter);
        ok(refcount == 1, "Get unexpected refcount %u for adapter %u.\n", refcount, i);
        IDXGIAdapter_Release(adapter2);

        refcount = get_refcount(factory);
        ok(refcount == 2, "Get unexpected refcount %u.\n", refcount);
        refcount = get_refcount(adapter);
        ok(refcount == 1, "Get unexpected refcount %u for adapter %u.\n", refcount, i);

        for (j = 0; ; ++j)
        {
            MONITORINFOEXW monitor_info;
            BOOL ret;

            hr = IDXGIAdapter_EnumOutputs(adapter, j, &output);
            if (hr == DXGI_ERROR_NOT_FOUND)
                break;
            ok(SUCCEEDED(hr), "Failed to enumerate output %u on adapter %u, hr %#x.\n", j, i, hr);

            hr = IDXGIAdapter_EnumOutputs(adapter, j, &output2);
            ok(SUCCEEDED(hr), "Failed to enumerate output %u on adapter %u, hr %#x.\n", j, i, hr);
            ok(output != output2, "Expected to get new instance of IDXGIOutput, %p == %p.\n", output, output2);
            refcount = get_refcount(output);
            ok(refcount == 1, "Get unexpected refcount %u for output %u, adapter %u.\n", refcount, j, i);
            IDXGIOutput_Release(output2);

            refcount = get_refcount(factory);
            ok(refcount == 2, "Get unexpected refcount %u.\n", refcount);
            refcount = get_refcount(adapter);
            ok(refcount == 2, "Get unexpected refcount %u for adapter %u.\n", refcount, i);
            refcount = get_refcount(output);
            ok(refcount == 1, "Get unexpected refcount %u for output %u, adapter %u.\n", refcount, j, i);

            hr = IDXGIOutput_GetDesc(output, &desc);
            ok(SUCCEEDED(hr), "Failed to get desc for output %u on adapter %u, hr %#x.\n", j, i, hr);

            monitor_info.cbSize = sizeof(monitor_info);
            ret = GetMonitorInfoW(desc.Monitor, (MONITORINFO *)&monitor_info);
            ok(ret, "Failed to get monitor info.\n");
            ok(!lstrcmpW(desc.DeviceName, monitor_info.szDevice), "Got unexpected device name %s, expected %s.\n",
                    wine_dbgstr_w(desc.DeviceName), wine_dbgstr_w(monitor_info.szDevice));
            ok(EqualRect(&desc.DesktopCoordinates, &monitor_info.rcMonitor),
                    "Got unexpected desktop coordinates %s, expected %s.\n",
                    wine_dbgstr_rect(&desc.DesktopCoordinates),
                    wine_dbgstr_rect(&monitor_info.rcMonitor));

            IDXGIOutput_Release(output);
            refcount = get_refcount(adapter);
            ok(refcount == 1, "Get unexpected refcount %u for adapter %u.\n", refcount, i);
        }

        IDXGIAdapter_Release(adapter);
        refcount = get_refcount(factory);
        ok(refcount == 1, "Get unexpected refcount %u.\n", refcount);
    }

    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "IDXGIFactory has %u references left.\n", refcount);
}

struct dxgi_adapter
{
    IDXGIAdapter IDXGIAdapter_iface;
    IDXGIAdapter *wrapped_iface;
};

static inline struct dxgi_adapter *impl_from_IDXGIAdapter(IDXGIAdapter *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_adapter, IDXGIAdapter_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_QueryInterface(IDXGIAdapter *iface, REFIID iid, void **out)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);

    if (IsEqualGUID(iid, &IID_IDXGIAdapter)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IDXGIAdapter_AddRef(adapter->wrapped_iface);
        *out = iface;
        return S_OK;
    }
    return IDXGIAdapter_QueryInterface(adapter->wrapped_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_AddRef(IDXGIAdapter *iface)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_AddRef(adapter->wrapped_iface);
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_Release(IDXGIAdapter *iface)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_Release(adapter->wrapped_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateData(IDXGIAdapter *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_SetPrivateData(adapter->wrapped_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateDataInterface(IDXGIAdapter *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_SetPrivateDataInterface(adapter->wrapped_iface, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetPrivateData(IDXGIAdapter *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_GetPrivateData(adapter->wrapped_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetParent(IDXGIAdapter *iface, REFIID iid, void **parent)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_GetParent(adapter->wrapped_iface, iid, parent);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_EnumOutputs(IDXGIAdapter *iface,
        UINT output_idx, IDXGIOutput **output)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_EnumOutputs(adapter->wrapped_iface, output_idx, output);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc(IDXGIAdapter *iface, DXGI_ADAPTER_DESC *desc)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_GetDesc(adapter->wrapped_iface, desc);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_CheckInterfaceSupport(IDXGIAdapter *iface,
        REFGUID guid, LARGE_INTEGER *umd_version)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_CheckInterfaceSupport(adapter->wrapped_iface, guid, umd_version);
}

static const struct IDXGIAdapterVtbl dxgi_adapter_vtbl =
{
    dxgi_adapter_QueryInterface,
    dxgi_adapter_AddRef,
    dxgi_adapter_Release,
    dxgi_adapter_SetPrivateData,
    dxgi_adapter_SetPrivateDataInterface,
    dxgi_adapter_GetPrivateData,
    dxgi_adapter_GetParent,
    dxgi_adapter_EnumOutputs,
    dxgi_adapter_GetDesc,
    dxgi_adapter_CheckInterfaceSupport,
};

static void test_object_wrapping(void)
{
    struct dxgi_adapter wrapper;
    DXGI_ADAPTER_DESC desc;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    ID3D10Device1 *device;
    ULONG refcount;
    HRESULT hr;

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Failed to create DXGI factory, hr %#x.\n", hr);

    hr = IDXGIFactory_EnumAdapters(factory, 0, &adapter);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Could not enumerate adapters.\n");
        IDXGIFactory_Release(factory);
        return;
    }
    ok(hr == S_OK, "Failed to enumerate adapter, hr %#x.\n", hr);

    wrapper.IDXGIAdapter_iface.lpVtbl = &dxgi_adapter_vtbl;
    wrapper.wrapped_iface = adapter;

    hr = D3D10CreateDevice1(&wrapper.IDXGIAdapter_iface, D3D10_DRIVER_TYPE_HARDWARE, NULL,
            0, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device);
    if (SUCCEEDED(hr))
    {
        refcount = ID3D10Device1_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }

    hr = IDXGIAdapter_GetDesc(&wrapper.IDXGIAdapter_iface, &desc);
    ok(hr == S_OK, "Failed to get adapter desc, hr %#x.\n", hr);

    refcount = IDXGIAdapter_Release(&wrapper.IDXGIAdapter_iface);
    ok(!refcount, "Adapter has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    int diff = 200;
    DWORD time;
    MSG msg;

    time = GetTickCount() + diff;
    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

struct message
{
    unsigned int message;
    BOOL check_wparam;
    WPARAM expect_wparam;
};

static BOOL expect_no_messages;
static const struct message *expect_messages;
static const struct message *expect_messages_broken;

static BOOL check_message(const struct message *expected,
        HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam)
{
    if (expected->message != message)
        return FALSE;

    if (expected->check_wparam)
    {
        ok(wparam == expected->expect_wparam,
                "Got unexpected wparam %lx for message %x, expected %lx.\n",
                wparam, message, expected->expect_wparam);
    }

    return TRUE;
}

static LRESULT CALLBACK test_wndproc(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam)
{
    ok(!expect_no_messages, "Got unexpected message %#x, hwnd %p, wparam %#lx, lparam %#lx.\n",
            message, hwnd, wparam, lparam);

    if (expect_messages)
    {
        if (check_message(expect_messages, hwnd, message, wparam, lparam))
            ++expect_messages;
    }

    if (expect_messages_broken)
    {
        if (check_message(expect_messages_broken, hwnd, message, wparam, lparam))
            ++expect_messages_broken;
    }

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

static void test_swapchain_window_messages(void)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    DXGI_MODE_DESC mode_desc;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    ULONG refcount;
    WNDCLASSA wc;
    HWND window;
    HRESULT hr;

    static const struct message enter_fullscreen_messages[] =
    {
        {WM_STYLECHANGING,     TRUE,  GWL_STYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_STYLE},
        {WM_STYLECHANGING,     TRUE,  GWL_EXSTYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_EXSTYLE},
        {WM_WINDOWPOSCHANGING, FALSE, 0},
        {WM_GETMINMAXINFO,     FALSE, 0},
        {WM_NCCALCSIZE,        FALSE, 0},
        {WM_WINDOWPOSCHANGED,  FALSE, 0},
        {WM_MOVE,              FALSE, 0},
        {WM_SIZE,              FALSE, 0},
        {0,                    FALSE, 0},
    };
    static const struct message enter_fullscreen_messages_vista[] =
    {
        {WM_STYLECHANGING,     TRUE,  GWL_STYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_STYLE},
        {WM_WINDOWPOSCHANGING, FALSE, 0},
        {WM_NCCALCSIZE,        FALSE, 0},
        {WM_WINDOWPOSCHANGED,  FALSE, 0},
        {WM_MOVE,              FALSE, 0},
        {WM_SIZE,              FALSE, 0},
        {WM_STYLECHANGING,     TRUE,  GWL_EXSTYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_EXSTYLE},
        {WM_WINDOWPOSCHANGING, FALSE, 0},
        {WM_GETMINMAXINFO,     FALSE, 0},
        {WM_NCCALCSIZE,        FALSE, 0},
        {WM_WINDOWPOSCHANGED,  FALSE, 0},
        {WM_SIZE,              FALSE, 0},
        {0,                    FALSE, 0},
    };
    static const struct message leave_fullscreen_messages[] =
    {
        {WM_STYLECHANGING,     TRUE,  GWL_STYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_STYLE},
        {WM_STYLECHANGING,     TRUE,  GWL_EXSTYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_EXSTYLE},
        {WM_WINDOWPOSCHANGING, FALSE, 0},
        {WM_GETMINMAXINFO,     FALSE, 0},
        {WM_NCCALCSIZE,        FALSE, 0},
        {WM_WINDOWPOSCHANGED,  FALSE, 0},
        {WM_MOVE,              FALSE, 0},
        {WM_SIZE,              FALSE, 0},
        {0,                    FALSE, 0},
    };
    static const struct message resize_target_messages[] =
    {
        {WM_WINDOWPOSCHANGING, FALSE, 0},
        {WM_GETMINMAXINFO,     FALSE, 0},
        {WM_NCCALCSIZE,        FALSE, 0},
        {WM_WINDOWPOSCHANGED,  FALSE, 0},
        {WM_SIZE,              FALSE, 0},
        {0,                    FALSE, 0},
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = test_wndproc;
    wc.lpszClassName = "dxgi_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");
    window = CreateWindowA("dxgi_test_wndproc_wc", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    ok(!!window, "Failed to create window.\n");

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Failed to get adapter, hr %#x.\n", hr);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Failed to get parent, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = window;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    /* create swapchain */
    flush_events();
    expect_no_messages = TRUE;
    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);
    flush_events();
    expect_no_messages = FALSE;

    /* resize target */
    expect_messages = resize_target_messages;
    memset(&mode_desc, 0, sizeof(mode_desc));
    mode_desc.Width = 800;
    mode_desc.Height = 600;
    hr = IDXGISwapChain_ResizeTarget(swapchain, &mode_desc);
    ok(hr == S_OK, "Failed to resize target, hr %#x.\n", hr);
    flush_events();
    ok(!expect_messages->message, "Expected message %#x.\n", expect_messages->message);

    expect_messages = resize_target_messages;
    memset(&mode_desc, 0, sizeof(mode_desc));
    mode_desc.Width = 400;
    mode_desc.Height = 200;
    hr = IDXGISwapChain_ResizeTarget(swapchain, &mode_desc);
    ok(hr == S_OK, "Failed to resize target, hr %#x.\n", hr);
    flush_events();
    ok(!expect_messages->message, "Expected message %#x.\n", expect_messages->message);

    /* enter fullscreen */
    expect_messages = enter_fullscreen_messages;
    expect_messages_broken = enter_fullscreen_messages_vista;
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
             || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
            "Failed to enter fullscreen, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not change fullscreen state.\n");
        goto done;
    }
    flush_events();
    todo_wine
    ok(!expect_messages->message || broken(!expect_messages_broken->message),
            "Expected message %#x or %#x.\n",
            expect_messages->message, expect_messages_broken->message);
    expect_messages_broken = NULL;

    /* leave fullscreen */
    expect_messages = leave_fullscreen_messages;
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    flush_events();
    ok(!expect_messages->message, "Expected message %#x.\n", expect_messages->message);
    expect_messages = NULL;

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

    /* create fullscreen swapchain */
    DestroyWindow(window);
    window = CreateWindowA("dxgi_test_wndproc_wc", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    ok(!!window, "Failed to create window.\n");
    swapchain_desc.OutputWindow = window;
    swapchain_desc.Windowed = FALSE;
    swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    flush_events();

    expect_messages = enter_fullscreen_messages;
    expect_messages_broken = enter_fullscreen_messages_vista;
    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);
    flush_events();
    todo_wine
    ok(!expect_messages->message || broken(!expect_messages_broken->message),
            "Expected message %#x or %#x.\n",
            expect_messages->message, expect_messages_broken->message);
    expect_messages_broken = NULL;

    /* leave fullscreen */
    expect_messages = leave_fullscreen_messages;
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    flush_events();
    ok(!expect_messages->message, "Expected message %#x.\n", expect_messages->message);
    expect_messages = NULL;

done:
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);
    DestroyWindow(window);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);

    UnregisterClassA("dxgi_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_swapchain_window_styles(void)
{
    LONG style, exstyle, fullscreen_style, fullscreen_exstyle;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    ULONG refcount;
    unsigned int i;
    HRESULT hr;

    static const struct
    {
        LONG style, exstyle;
        LONG expected_style, expected_exstyle;
    }
    tests[] =
    {
        {WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 0,
         WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS,
         WS_EX_WINDOWEDGE},
        {WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE, 0,
         WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS | WS_VISIBLE,
         WS_EX_WINDOWEDGE},
        {WS_OVERLAPPED | WS_VISIBLE, 0,
         WS_OVERLAPPED | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION, WS_EX_WINDOWEDGE},
        {WS_CAPTION | WS_DISABLED, WS_EX_TOPMOST,
         WS_CAPTION | WS_DISABLED | WS_CLIPSIBLINGS, WS_EX_TOPMOST | WS_EX_WINDOWEDGE},
        {WS_CAPTION | WS_DISABLED | WS_VISIBLE, WS_EX_TOPMOST,
         WS_CAPTION | WS_DISABLED | WS_VISIBLE | WS_CLIPSIBLINGS, WS_EX_TOPMOST | WS_EX_WINDOWEDGE},
        {WS_CAPTION | WS_SYSMENU | WS_VISIBLE, WS_EX_APPWINDOW,
         WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_CLIPSIBLINGS, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE},
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Failed to get adapter, hr %#x.\n", hr);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Failed to get parent, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        swapchain_desc.OutputWindow = CreateWindowExA(tests[i].exstyle, "static", "dxgi_test",
                tests[i].style, 0, 0, 400, 200, 0, 0, 0, 0);

        style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
        exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
        ok(style == tests[i].expected_style, "Test %u: Got style %#x, expected %#x.\n",
                i, style, tests[i].expected_style);
        ok(exstyle == tests[i].expected_exstyle, "Test %u: Got exstyle %#x, expected %#x.\n",
                i, exstyle, tests[i].expected_exstyle);

        fullscreen_style = tests[i].expected_style & (WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS);
        fullscreen_exstyle = (tests[i].expected_exstyle & WS_EX_APPWINDOW) | WS_EX_TOPMOST;

        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
        ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);

        style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
        exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
        ok(style == tests[i].expected_style, "Test %u: Got style %#x, expected %#x.\n",
                i, style, tests[i].expected_style);
        ok(exstyle == tests[i].expected_exstyle, "Test %u: Got exstyle %#x, expected %#x.\n",
                i, exstyle, tests[i].expected_exstyle);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
        ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
                || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
                "Failed to set fullscreen state, hr %#x.\n", hr);
        if (SUCCEEDED(hr))
        {
            style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
            exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
            todo_wine
            ok(style == fullscreen_style, "Test %u: Got style %#x, expected %#x.\n",
                    i, style, fullscreen_style);
            ok(exstyle == fullscreen_exstyle, "Test %u: Got exstyle %#x, expected %#x.\n",
                    i, exstyle, fullscreen_exstyle);

            hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
            ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        }
        else
        {
            skip("Test %u: Could not change fullscreen state.\n", i);
        }

        style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
        exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
        todo_wine_if(!(tests[i].expected_style & WS_VISIBLE))
        ok(style == tests[i].expected_style, "Test %u: Got style %#x, expected %#x.\n",
                i, style, tests[i].expected_style);
        todo_wine_if(!(tests[i].expected_exstyle & WS_EX_TOPMOST))
        ok(exstyle == tests[i].expected_exstyle, "Test %u: Got exstyle %#x, expected %#x.\n",
                i, exstyle, tests[i].expected_exstyle);

        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

        style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
        exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
        todo_wine_if(!(tests[i].expected_style & WS_VISIBLE))
        ok(style == tests[i].expected_style, "Test %u: Got style %#x, expected %#x.\n",
                i, style, tests[i].expected_style);
        todo_wine_if(!(tests[i].expected_exstyle & WS_EX_TOPMOST))
        ok(exstyle == tests[i].expected_exstyle, "Test %u: Got exstyle %#x, expected %#x.\n",
                i, exstyle, tests[i].expected_exstyle);

        DestroyWindow(swapchain_desc.OutputWindow);
    }

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
}

static void test_gamma_control(void)
{
    DXGI_GAMMA_CONTROL_CAPABILITIES caps;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    DXGI_GAMMA_CONTROL gamma;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    IDXGIOutput *output;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter doesn't have any outputs.\n");
        IDXGIAdapter_Release(adapter);
        IDXGIDevice_Release(device);
        return;
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIOutput_GetGammaControlCapabilities(output, &caps);
    todo_wine
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    IDXGIOutput_Release(output);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
            || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
            "Failed to enter fullscreen, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not change fullscreen state.\n");
        goto done;
    }

    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    memset(&caps, 0, sizeof(caps));
    hr = IDXGIOutput_GetGammaControlCapabilities(output, &caps);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    ok(caps.MaxConvertedValue > caps.MinConvertedValue
            || broken(caps.MaxConvertedValue == 0.0f && caps.MinConvertedValue == 1.0f) /* WARP */,
            "Expected max gamma value (%.8e) to be bigger than min value (%.8e).\n",
            caps.MaxConvertedValue, caps.MinConvertedValue);

    for (i = 1; i < caps.NumGammaControlPoints; ++i)
    {
        ok(caps.ControlPointPositions[i] > caps.ControlPointPositions[i - 1],
                "Expected control point positions to be strictly monotonically increasing (%.8e > %.8e).\n",
                caps.ControlPointPositions[i], caps.ControlPointPositions[i - 1]);
    }

    memset(&gamma, 0, sizeof(gamma));
    hr = IDXGIOutput_GetGammaControl(output, &gamma);
    todo_wine
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIOutput_SetGammaControl(output, &gamma);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    IDXGIOutput_Release(output);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

done:
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);
    DestroyWindow(swapchain_desc.OutputWindow);

    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
}

static void test_window_association(void)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    LONG_PTR original_wndproc, wndproc;
    IDXGIFactory *factory, *factory2;
    IDXGISwapChain *swapchain;
    IDXGIOutput *output;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    HWND hwnd, hwnd2;
    BOOL fullscreen;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    static const struct
    {
        UINT flag;
        BOOL expect_fullscreen;
        BOOL broken_d3d10;
    }
    tests[] =
    {
        /* There are two reasons why VK_TAB and VK_ESC are not tested here:
         *
         * - Posting them to the window doesn't exit fullscreen like
         *   Alt+Enter does. Alt+Tab and Alt+Esc are handled somewhere else.
         *   E.g., not calling IDXGISwapChain::Present() will break Alt+Tab
         *   and Alt+Esc while Alt+Enter will still function.
         *
         * - Posting them hangs the posting thread. Another thread that keeps
         *   sending input is needed to avoid the hang. The hang is not
         *   because of flush_events(). */
        {0, TRUE},
        {0, FALSE},
        {DXGI_MWA_NO_WINDOW_CHANGES, FALSE},
        {DXGI_MWA_NO_WINDOW_CHANGES, FALSE},
        {DXGI_MWA_NO_ALT_ENTER, FALSE, TRUE},
        {DXGI_MWA_NO_ALT_ENTER, FALSE},
        {DXGI_MWA_NO_PRINT_SCREEN, TRUE},
        {DXGI_MWA_NO_PRINT_SCREEN, FALSE},
        {0, TRUE},
        {0, FALSE}
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    swapchain_desc.BufferDesc.Width = 640;
    swapchain_desc.BufferDesc.Height = 480;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    original_wndproc = GetWindowLongPtrW(swapchain_desc.OutputWindow, GWLP_WNDPROC);

    hwnd2 = CreateWindowA("static", "dxgi_test2", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory2);
    ok(hr == S_OK, "Failed to create DXGI factory, hr %#x.\n", hr);

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = IDXGIAdapter_Release(adapter);

    hr = IDXGIFactory_GetWindowAssociation(factory, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i <= DXGI_MWA_VALID; ++i)
    {
        hr = IDXGIFactory_MakeWindowAssociation(factory, NULL, i);
        ok(hr == S_OK, "Got unexpected hr %#x for flags %#x.\n", hr, i);

        hr = IDXGIFactory_MakeWindowAssociation(factory, swapchain_desc.OutputWindow, i);
        ok(hr == S_OK, "Got unexpected hr %#x for flags %#x.\n", hr, i);

        wndproc = GetWindowLongPtrW(swapchain_desc.OutputWindow, GWLP_WNDPROC);
        ok(wndproc == original_wndproc, "Got unexpected wndproc %#lx, expected %#lx for flags %#x.\n",
                wndproc, original_wndproc, i);

        hwnd = (HWND)0xdeadbeef;
        hr = IDXGIFactory_GetWindowAssociation(factory, &hwnd);
        ok(hr == S_OK, "Got unexpected hr %#x for flags %#x.\n", hr, i);
        /* Apparently GetWindowAssociation() always returns NULL, even when
         * MakeWindowAssociation() and GetWindowAssociation() are both
         * successfully called. */
        ok(!hwnd, "Expect null associated window.\n");
    }

    hr = IDXGIFactory_MakeWindowAssociation(factory, swapchain_desc.OutputWindow, DXGI_MWA_VALID + 1);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    /* Alt+Enter tests. */
    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    wndproc = GetWindowLongPtrW(swapchain_desc.OutputWindow, GWLP_WNDPROC);
    ok(wndproc == original_wndproc, "Got unexpected wndproc %#lx, expected %#lx.\n", wndproc, original_wndproc);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
            || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Windows 7 testbot */,
            "Got unexpected hr %#x.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not change fullscreen state.\n");
    }
    else
    {
        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

        for (i = 0; i < ARRAY_SIZE(tests); ++i)
        {
            /* First associate a window with the opposite flags. */
            hr = IDXGIFactory_MakeWindowAssociation(factory, hwnd2, ~tests[i].flag & DXGI_MWA_VALID);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

            /* Associate the current test window. */
            hwnd = tests[i].flag ? swapchain_desc.OutputWindow : NULL;
            hr = IDXGIFactory_MakeWindowAssociation(factory, hwnd, tests[i].flag);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

            /* Associating a new test window doesn't override the old window. */
            hr = IDXGIFactory_MakeWindowAssociation(factory, hwnd2, ~tests[i].flag & DXGI_MWA_VALID);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

            /* Associations with a different factory don't affect the existing
             * association. */
            hr = IDXGIFactory_MakeWindowAssociation(factory2, hwnd, ~tests[i].flag & DXGI_MWA_VALID);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

            /* Post synthesized Alt + VK_RETURN WM_SYSKEYDOWN. */
            PostMessageA(swapchain_desc.OutputWindow, WM_SYSKEYDOWN, VK_RETURN,
                    (MapVirtualKeyA(VK_RETURN, MAPVK_VK_TO_VSC) << 16) | 0x20000001);
            flush_events();
            output = NULL;
            hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &output);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
            ok(fullscreen == tests[i].expect_fullscreen
                    || broken(tests[i].broken_d3d10 && fullscreen),
                    "Test %u: Got unexpected fullscreen %#x.\n", i, fullscreen);
            ok(fullscreen ? !!output : !output, "Test %u: Got wrong output.\n", i);
            if (output)
                IDXGIOutput_Release(output);

            wndproc = GetWindowLongPtrW(swapchain_desc.OutputWindow, GWLP_WNDPROC);
            ok(wndproc == original_wndproc, "Test %u: Got unexpected wndproc %#lx, expected %#lx.\n",
                    i, wndproc, original_wndproc);
        }
    }

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    refcount = IDXGIFactory_Release(factory2);
    ok(!refcount, "Factory has %u references left.\n", refcount);
    DestroyWindow(hwnd2);

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);
    DestroyWindow(swapchain_desc.OutputWindow);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
}

static void test_output_ownership(IUnknown *device, BOOL is_d3d12)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP check_ownership_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_OUTPUT_DESC output_desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIOutput *output;
    BOOL fullscreen;
    NTSTATUS status;
    ULONG refcount;
    HRESULT hr;

    if (!pD3DKMTCheckVidPnExclusiveOwnership
            || pD3DKMTCheckVidPnExclusiveOwnership(NULL) == STATUS_PROCEDURE_NOT_FOUND)
    {
        skip("D3DKMTCheckVidPnExclusiveOwnership() is unavailable.\n");
        return;
    }

    get_factory(device, is_d3d12, &factory);
    adapter = get_adapter(device, is_d3d12);
    ok(!!adapter, "Failed to get adapter.\n");

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    IDXGIAdapter_Release(adapter);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter doesn't have any outputs.\n");
        IDXGIFactory_Release(factory);
        return;
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIOutput_GetDesc(output, &output_desc);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    lstrcpyW(open_adapter_gdi_desc.DeviceName, output_desc.DeviceName);
    status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status);

    check_ownership_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_ownership_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    wait_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_SUCCESS, FALSE);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, NULL, NULL, NULL, NULL);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Swapchain in fullscreen mode. */
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
    /* DXGI_ERROR_NOT_CURRENTLY_AVAILABLE on some machines.
     * DXGI_ERROR_UNSUPPORTED on the Windows 7 testbot. */
    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE || broken(hr == DXGI_ERROR_UNSUPPORTED))
    {
        skip("Failed to change fullscreen state.\n");
        goto done;
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    fullscreen = FALSE;
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(fullscreen, "Got unexpected fullscreen state.\n");
    if (is_d3d12)
        wait_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_SUCCESS, FALSE);
    else
        wait_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_GRAPHICS_PRESENT_OCCLUDED, TRUE);
    hr = IDXGIOutput_TakeOwnership(output, device, FALSE);
    todo_wine ok(hr == (is_d3d12 ? E_NOINTERFACE : E_INVALIDARG), "Got unexpected hr %#x.\n", hr);
    hr = IDXGIOutput_TakeOwnership(output, device, TRUE);
    todo_wine ok(hr == (is_d3d12 ? E_NOINTERFACE : S_OK), "Got unexpected hr %#x.\n", hr);
    IDXGIOutput_ReleaseOwnership(output);
    wait_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_SUCCESS, FALSE);

    /* IDXGIOutput_TakeOwnership always returns E_NOINTERFACE for d3d12. Tests
     * finished. */
    if (is_d3d12)
        goto done;

    hr = IDXGIOutput_TakeOwnership(output, device, FALSE);
    todo_wine ok(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "Got unexpected hr %#x.\n", hr);
    IDXGIOutput_ReleaseOwnership(output);

    hr = IDXGIOutput_TakeOwnership(output, device, TRUE);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* Note that the "exclusive" parameter to IDXGIOutput_TakeOwnership()
     * seems to behave opposite to what's described by MSDN. */
    wait_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_GRAPHICS_PRESENT_OCCLUDED, TRUE);
    hr = IDXGIOutput_TakeOwnership(output, device, FALSE);
    todo_wine ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    IDXGIOutput_ReleaseOwnership(output);

    /* Swapchain in windowed mode. */
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    fullscreen = TRUE;
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!fullscreen, "Unexpected fullscreen state.\n");
    wait_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_SUCCESS, FALSE);

    hr = IDXGIOutput_TakeOwnership(output, device, FALSE);
    todo_wine ok(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIOutput_TakeOwnership(output, device, TRUE);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    wait_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_GRAPHICS_PRESENT_OCCLUDED, TRUE);
    IDXGIOutput_ReleaseOwnership(output);
    wait_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_SUCCESS, FALSE);

done:
    IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    wait_device_idle(device);

    IDXGIOutput_Release(output);
    IDXGISwapChain_Release(swapchain);
    DestroyWindow(swapchain_desc.OutputWindow);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %u.\n", refcount);

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status);
}

static void run_on_d3d10(void (*test_func)(IUnknown *device, BOOL is_d3d12))
{
    IDXGIDevice *device;
    ULONG refcount;

    if (!(device = create_device(0)))
    {
        skip("Failed to create Direct3D 10 device.\n");
        return;
    }

    test_func((IUnknown *)device, FALSE);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void run_on_d3d12(void (*test_func)(IUnknown *device, BOOL is_d3d12))
{
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    ULONG refcount;

    if (!(device = create_d3d12_device()))
    {
        skip("Failed to create Direct3D 12 device.\n");
        return;
    }

    queue = create_d3d12_direct_queue(device);

    test_func((IUnknown *)queue, TRUE);

    wait_queue_idle(device, queue);

    refcount = ID3D12CommandQueue_Release(queue);
    ok(!refcount, "Command queue has %u references left.\n", refcount);
    refcount = ID3D12Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

START_TEST(dxgi)
{
    HMODULE dxgi_module, d3d12_module, gdi32_module;
    BOOL enable_debug_layer = FALSE;
    unsigned int argc, i;
    ID3D12Debug *debug;
    char **argv;

    dxgi_module = GetModuleHandleA("dxgi.dll");
    pCreateDXGIFactory1 = (void *)GetProcAddress(dxgi_module, "CreateDXGIFactory1");
    pCreateDXGIFactory2 = (void *)GetProcAddress(dxgi_module, "CreateDXGIFactory2");

    gdi32_module = GetModuleHandleA("gdi32.dll");
    pD3DKMTCheckVidPnExclusiveOwnership = (void *)GetProcAddress(gdi32_module, "D3DKMTCheckVidPnExclusiveOwnership");
    pD3DKMTCloseAdapter = (void *)GetProcAddress(gdi32_module, "D3DKMTCloseAdapter");
    pD3DKMTOpenAdapterFromGdiDisplayName = (void *)GetProcAddress(gdi32_module, "D3DKMTOpenAdapterFromGdiDisplayName");

    registry_mode.dmSize = sizeof(registry_mode);
    ok(EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &registry_mode), "Failed to get display mode.\n");

    use_mt = !getenv("WINETEST_NO_MT_D3D");

    argc = winetest_get_mainargs(&argv);
    for (i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--validate"))
            enable_debug_layer = TRUE;
        else if (!strcmp(argv[i], "--warp"))
            use_warp_adapter = TRUE;
        else if (!strcmp(argv[i], "--adapter") && i + 1 < argc)
            use_adapter_idx = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--single"))
            use_mt = FALSE;
    }

    queue_test(test_adapter_desc);
    queue_test(test_adapter_luid);
    queue_test(test_query_video_memory_info);
    queue_test(test_check_interface_support);
    queue_test(test_create_surface);
    queue_test(test_parents);
    queue_test(test_output);
    queue_test(test_find_closest_matching_mode);
    queue_test(test_get_containing_output);
    queue_test(test_resize_target_wndproc);
    queue_test(test_create_factory);
    queue_test(test_private_data);
    queue_test(test_maximum_frame_latency);
    queue_test(test_output_desc);
    queue_test(test_object_wrapping);

    run_queued_tests();

    /* These tests use full-screen swapchains, so shouldn't run in parallel. */
    test_create_swapchain();
    test_default_fullscreen_target_output();
    test_inexact_modes();
    test_gamma_control();
    test_swapchain_parameters();
    test_swapchain_window_messages();
    test_swapchain_window_styles();
    test_window_association();
    run_on_d3d10(test_set_fullscreen);
    run_on_d3d10(test_resize_target);
    run_on_d3d10(test_swapchain_resize);
    run_on_d3d10(test_swapchain_present);
    run_on_d3d10(test_swapchain_backbuffer_index);
    run_on_d3d10(test_swapchain_formats);
    run_on_d3d10(test_output_ownership);

    if (!(d3d12_module = LoadLibraryA("d3d12.dll")))
    {
        skip("Direct3D 12 is not available.\n");
        return;
    }

    pD3D12CreateDevice = (void *)GetProcAddress(d3d12_module, "D3D12CreateDevice");
    pD3D12GetDebugInterface = (void *)GetProcAddress(d3d12_module, "D3D12GetDebugInterface");

    if (enable_debug_layer && SUCCEEDED(pD3D12GetDebugInterface(&IID_ID3D12Debug, (void **)&debug)))
    {
        ID3D12Debug_EnableDebugLayer(debug);
        ID3D12Debug_Release(debug);
    }

    run_on_d3d12(test_set_fullscreen);
    run_on_d3d12(test_resize_target);
    run_on_d3d12(test_swapchain_resize);
    run_on_d3d12(test_swapchain_present);
    run_on_d3d12(test_swapchain_backbuffer_index);
    run_on_d3d12(test_swapchain_formats);
    run_on_d3d12(test_output_ownership);

    FreeLibrary(d3d12_module);
}
