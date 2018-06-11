/*
 * Copyright 2017 Józef Kucia for CodeWeavers
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

#include <stdlib.h>
#define COBJMACROS
#include "initguid.h"
#include "d3d12.h"
#include "dxgi1_6.h"
#include "wine/test.h"

static BOOL compare_color(DWORD c1, DWORD c2, unsigned int max_diff)
{
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff)
        return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff)
        return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff)
        return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff)
        return FALSE;
    return TRUE;
}

static unsigned int format_size(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return 4;
        default:
            trace("Unhandled format %#x.\n", format);
            return 1;
    }
}

static size_t align(size_t addr, size_t alignment)
{
    return (addr + (alignment - 1)) & ~(alignment - 1);
}

static void set_viewport(D3D12_VIEWPORT *vp, float x, float y,
        float width, float height, float min_depth, float max_depth)
{
    vp->TopLeftX = x;
    vp->TopLeftY = y;
    vp->Width = width;
    vp->Height = height;
    vp->MinDepth = min_depth;
    vp->MaxDepth = max_depth;
}

static BOOL use_warp_adapter;
static unsigned int use_adapter_idx;

static IDXGIAdapter *create_adapter(void)
{
    IDXGIFactory4 *factory;
    IDXGIAdapter *adapter;
    HRESULT hr;

    if (!use_warp_adapter && !use_adapter_idx)
        return NULL;

    hr = CreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory);
    ok(hr == S_OK, "Failed to create factory, hr %#x.\n", hr);

    adapter = NULL;
    if (use_warp_adapter)
    {
        hr = IDXGIFactory4_EnumWarpAdapter(factory, &IID_IDXGIAdapter, (void **)&adapter);
    }
    else
    {
        hr = IDXGIFactory4_EnumAdapters(factory, use_adapter_idx, &adapter);
    }
    IDXGIFactory4_Release(factory);
    if (FAILED(hr))
        trace("Failed to get adapter, hr %#x.\n", hr);
    return adapter;
}

static ID3D12Device *create_device(void)
{
    IDXGIAdapter *adapter;
    ID3D12Device *device;
    HRESULT hr;

    adapter = create_adapter();
    hr = D3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&device);
    if (adapter)
        IDXGIAdapter_Release(adapter);
    if (FAILED(hr))
        return NULL;

    return device;
}

static void print_adapter_info(void)
{
    DXGI_ADAPTER_DESC adapter_desc;
    IDXGIFactory4 *factory;
    IDXGIAdapter *adapter;
    ID3D12Device *device;
    HRESULT hr;
    LUID luid;

    if (!(device = create_device()))
        return;
    luid = ID3D12Device_GetAdapterLuid(device);
    ID3D12Device_Release(device);

    hr = CreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory);
    ok(hr == S_OK, "Failed to create factory, hr %#x.\n", hr);
    hr = IDXGIFactory4_EnumAdapterByLuid(factory, luid, &IID_IDXGIAdapter, (void **)&adapter);
    todo_wine ok(hr == S_OK, "Failed to enum adapter by LUID, hr %#x.\n", hr);
    IDXGIFactory4_Release(factory);

    if (FAILED(hr))
        return;

    hr = IDXGIAdapter_GetDesc(adapter, &adapter_desc);
    ok(hr == S_OK, "Failed to get adapter desc, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);

    trace("Adapter: %s, %04x:%04x.\n", wine_dbgstr_w(adapter_desc.Description),
            adapter_desc.VendorId, adapter_desc.DeviceId);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static HRESULT create_root_signature(ID3D12Device *device, const D3D12_ROOT_SIGNATURE_DESC *desc,
        ID3D12RootSignature **root_signature)
{
    ID3DBlob *blob;
    HRESULT hr;

    if (FAILED(hr = D3D12SerializeRootSignature(desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, NULL)))
        return hr;

    hr = ID3D12Device_CreateRootSignature(device, 0, ID3D10Blob_GetBufferPointer(blob),
            ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, (void **)root_signature);
    ID3D10Blob_Release(blob);
    return hr;
}

#define create_empty_root_signature(device, flags) create_empty_root_signature_(__LINE__, device, flags)
static ID3D12RootSignature *create_empty_root_signature_(unsigned int line,
        ID3D12Device *device, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    ID3D12RootSignature *root_signature = NULL;
    HRESULT hr;

    root_signature_desc.NumParameters = 0;
    root_signature_desc.pParameters = NULL;
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers = NULL;
    root_signature_desc.Flags = flags;
    hr = create_root_signature(device, &root_signature_desc, &root_signature);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create root signature, hr %#x.\n", hr);

    return root_signature;
}

#define create_pipeline_state(a, b, c) create_pipeline_state_(__LINE__, a, b, c)
static ID3D12PipelineState *create_pipeline_state_(unsigned int line, ID3D12Device *device,
        ID3D12RootSignature *root_signature, DXGI_FORMAT rt_format)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc;
    ID3D12PipelineState *pipeline_state;
    HRESULT hr;

    static const DWORD vs_code[] =
    {
#if 0
        void main(uint id : SV_VertexID, out float4 position : SV_Position)
        {
            float2 coords = float2((id << 1) & 2, id & 2);
            position = float4(coords * float2(2, -2) + float2(-1, 1), 0, 1);
        }
#endif
        0x43425844, 0xf900d25e, 0x68bfefa7, 0xa63ac0a7, 0xa476af7a, 0x00000001, 0x0000018c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000006, 0x00000001, 0x00000000, 0x00000101, 0x565f5653, 0x65747265, 0x00444978,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x7469736f, 0x006e6f69, 0x58454853, 0x000000f0, 0x00010050,
        0x0000003c, 0x0100086a, 0x04000060, 0x00101012, 0x00000000, 0x00000006, 0x04000067, 0x001020f2,
        0x00000000, 0x00000001, 0x02000068, 0x00000001, 0x0b00008c, 0x00100012, 0x00000000, 0x00004001,
        0x00000001, 0x00004001, 0x00000001, 0x0010100a, 0x00000000, 0x00004001, 0x00000000, 0x07000001,
        0x00100042, 0x00000000, 0x0010100a, 0x00000000, 0x00004001, 0x00000002, 0x05000056, 0x00100032,
        0x00000000, 0x00100086, 0x00000000, 0x0f000032, 0x00102032, 0x00000000, 0x00100046, 0x00000000,
        0x00004002, 0x40000000, 0xc0000000, 0x00000000, 0x00000000, 0x00004002, 0xbf800000, 0x3f800000,
        0x00000000, 0x00000000, 0x08000036, 0x001020c2, 0x00000000, 0x00004002, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE vs = {vs_code, sizeof(vs_code)};
    static const DWORD ps_code[] =
    {
#if 0
        void main(const in float4 position : SV_Position, out float4 target : SV_Target0)
        {
            target = float4(0.0f, 1.0f, 0.0f, 1.0f);
        }
#endif
        0x43425844, 0x8a4a8140, 0x5eba8e0b, 0x714e0791, 0xb4b8eed2, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x0000003c, 0x00000050,
        0x0000000f, 0x0100086a, 0x03000065, 0x001020f2, 0x00000000, 0x08000036, 0x001020f2, 0x00000000,
        0x00004002, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps = {ps_code, sizeof(ps_code)};

    memset(&pipeline_state_desc, 0, sizeof(pipeline_state_desc));
    pipeline_state_desc.pRootSignature = root_signature;
    pipeline_state_desc.VS = vs;
    pipeline_state_desc.PS = ps;
    pipeline_state_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pipeline_state_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pipeline_state_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    pipeline_state_desc.SampleMask = ~(UINT)0;
    pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline_state_desc.NumRenderTargets = 1;
    pipeline_state_desc.RTVFormats[0] = rt_format;
    pipeline_state_desc.SampleDesc.Count = 1;
    hr = ID3D12Device_CreateGraphicsPipelineState(device, &pipeline_state_desc,
            &IID_ID3D12PipelineState, (void **)&pipeline_state);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create graphics pipeline state, hr %#x.\n", hr);

    return pipeline_state;
}

struct test_context
{
    ID3D12Device *device;

    ID3D12CommandQueue *queue;
    ID3D12CommandAllocator *allocator;
    ID3D12GraphicsCommandList *list;

    D3D12_RESOURCE_DESC render_target_desc;
    ID3D12Resource *render_target;

    ID3D12DescriptorHeap *rtv_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv;

    ID3D12RootSignature *root_signature;
    ID3D12PipelineState *pipeline_state;

    D3D12_VIEWPORT viewport;
    RECT scissor_rect;
};

#define create_render_target(context, a, b) create_render_target_(__LINE__, context, a, b)
static void create_render_target_(unsigned int line, struct test_context *context,
        ID3D12Resource **render_target, const D3D12_CPU_DESCRIPTOR_HANDLE *rtv)
{
    D3D12_HEAP_PROPERTIES heap_properties;
    D3D12_RESOURCE_DESC resource_desc;
    D3D12_CLEAR_VALUE clear_value;
    HRESULT hr;

    memset(&heap_properties, 0, sizeof(heap_properties));
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_desc.Alignment = 0;
    resource_desc.Width = 32;
    resource_desc.Height = 32;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    clear_value.Format = resource_desc.Format;
    clear_value.Color[0] = 1.0f;
    clear_value.Color[1] = 1.0f;
    clear_value.Color[2] = 1.0f;
    clear_value.Color[3] = 1.0f;
    hr = ID3D12Device_CreateCommittedResource(context->device,
            &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
            D3D12_RESOURCE_STATE_RENDER_TARGET, &clear_value,
            &IID_ID3D12Resource, (void **)render_target);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create texture, hr %#x.\n", hr);

    context->render_target_desc = resource_desc;

    if (rtv)
        ID3D12Device_CreateRenderTargetView(context->device, *render_target, NULL, *rtv);
}

#define init_test_context(context) init_test_context_(__LINE__, context)
static BOOL init_test_context_(unsigned int line, struct test_context *context)
{
    D3D12_COMMAND_QUEUE_DESC command_queue_desc;
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
    ID3D12Device *device;
    HRESULT hr;

    memset(context, 0, sizeof(*context));

    if (!(context->device = create_device()))
    {
        skip_(__FILE__, line)("Failed to create device.\n");
        return FALSE;
    }
    device = context->device;

    command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    command_queue_desc.NodeMask = 0;
    hr = ID3D12Device_CreateCommandQueue(device, &command_queue_desc,
            &IID_ID3D12CommandQueue, (void **)&context->queue);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create command queue, hr %#x.\n", hr);

    hr = ID3D12Device_CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT,
            &IID_ID3D12CommandAllocator, (void **)&context->allocator);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create command allocator, hr %#x.\n", hr);

    hr = ID3D12Device_CreateCommandList(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            context->allocator, NULL, &IID_ID3D12GraphicsCommandList, (void **)&context->list);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create command list, hr %#x.\n", hr);

    rtv_heap_desc.NumDescriptors = 16;
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_heap_desc.NodeMask = 0;
    hr = ID3D12Device_CreateDescriptorHeap(device, &rtv_heap_desc,
            &IID_ID3D12DescriptorHeap, (void **)&context->rtv_heap);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create descriptor heap, hr %#x.\n", hr);

    context->rtv = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(context->rtv_heap);

    create_render_target_(line, context, &context->render_target, &context->rtv);

    set_viewport(&context->viewport, 0.0f, 0.0f,
            context->render_target_desc.Width, context->render_target_desc.Height, 0.0f, 1.0f);
    SetRect(&context->scissor_rect, 0, 0,
            context->render_target_desc.Width, context->render_target_desc.Height);

    context->root_signature = create_empty_root_signature_(line,
            device, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    context->pipeline_state = create_pipeline_state_(line, device,
            context->root_signature, context->render_target_desc.Format);

    return TRUE;
}

#define destroy_test_context(context) destroy_test_context_(__LINE__, context)
static void destroy_test_context_(unsigned int line, struct test_context *context)
{
    ULONG refcount;

    if (context->pipeline_state)
        ID3D12PipelineState_Release(context->pipeline_state);
    if (context->root_signature)
        ID3D12RootSignature_Release(context->root_signature);

    if (context->rtv_heap)
        ID3D12DescriptorHeap_Release(context->rtv_heap);
    if (context->render_target)
        ID3D12Resource_Release(context->render_target);

    ID3D12CommandAllocator_Release(context->allocator);
    ID3D12CommandQueue_Release(context->queue);
    ID3D12GraphicsCommandList_Release(context->list);

    refcount = ID3D12Device_Release(context->device);
    ok_(__FILE__, line)(!refcount, "ID3D12Device has %u references left.\n", (unsigned int)refcount);
}

static void exec_command_list(ID3D12CommandQueue *queue, ID3D12GraphicsCommandList *list)
{
    ID3D12CommandList *lists[] = {(ID3D12CommandList *)list};
    ID3D12CommandQueue_ExecuteCommandLists(queue, 1, lists);
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

static void transition_sub_resource_state(ID3D12GraphicsCommandList *list, ID3D12Resource *resource,
        unsigned int sub_resource_idx, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after)
{
    D3D12_RESOURCE_BARRIER barrier;

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = sub_resource_idx;
    barrier.Transition.StateBefore = state_before;
    barrier.Transition.StateAfter = state_after;

    ID3D12GraphicsCommandList_ResourceBarrier(list, 1, &barrier);
}

#define create_buffer(a, b, c, d, e) create_buffer_(__LINE__, a, b, c, d, e)
static ID3D12Resource *create_buffer_(unsigned int line, ID3D12Device *device,
        D3D12_HEAP_TYPE heap_type, size_t size, D3D12_RESOURCE_FLAGS resource_flags,
        D3D12_RESOURCE_STATES initial_resource_state)
{
    D3D12_HEAP_PROPERTIES heap_properties;
    D3D12_RESOURCE_DESC resource_desc;
    ID3D12Resource *buffer;
    HRESULT hr;

    memset(&heap_properties, 0, sizeof(heap_properties));
    heap_properties.Type = heap_type;

    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment = 0;
    resource_desc.Width = size;
    resource_desc.Height = 1;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags = resource_flags;

    hr = ID3D12Device_CreateCommittedResource(device, &heap_properties,
            D3D12_HEAP_FLAG_NONE, &resource_desc, initial_resource_state,
            NULL, &IID_ID3D12Resource, (void **)&buffer);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create buffer, hr %#x.\n", hr);
    return buffer;
}

#define create_readback_buffer(a, b) create_readback_buffer_(__LINE__, a, b)
static ID3D12Resource *create_readback_buffer_(unsigned int line, ID3D12Device *device,
        size_t size)
{
    return create_buffer_(line, device, D3D12_HEAP_TYPE_READBACK, size,
            D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
}

static HWND create_window(DWORD style)
{
    return CreateWindowA("static", "d3d12_test", style, 0, 0, 256, 256, NULL, NULL, NULL, NULL);
}

static IDXGISwapChain3 *create_swapchain(ID3D12CommandQueue *queue, HWND window,
        unsigned int width, unsigned int height)
{
    IDXGISwapChain1 *swapchain1;
    DXGI_SWAP_CHAIN_DESC1 desc;
    IDXGISwapChain3 *swapchain;
    IDXGIFactory4 *factory;
    HRESULT hr;

    hr = CreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory);
    ok(hr == S_OK, "Failed to create factory, hr %#x.\n", hr);

    desc.Width = width;
    desc.Height = height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags = 0;
    hr = IDXGIFactory4_CreateSwapChainForHwnd(factory, (IUnknown *)queue, window, &desc, NULL, NULL, &swapchain1);
    ok(hr == S_OK, "Failed to create swapchain, hr %#x.\n", hr);

    IDXGIFactory4_Release(factory);

    hr = IDXGISwapChain1_QueryInterface(swapchain1, &IID_IDXGISwapChain3, (void **)&swapchain);
    ok(hr == S_OK, "Failed to query IDXGISwapChain3, hr %#x.\n", hr);
    IDXGISwapChain1_Release(swapchain1);
    return swapchain;
}

struct resource_readback
{
    unsigned int width;
    unsigned int height;
    ID3D12Resource *resource;
    unsigned int row_pitch;
    void *data;
};

static void get_texture_readback_with_command_list(ID3D12Resource *texture, unsigned int sub_resource,
        struct resource_readback *rb, ID3D12CommandQueue *queue, ID3D12GraphicsCommandList *command_list)
{
    D3D12_TEXTURE_COPY_LOCATION dst_location, src_location;
    D3D12_RESOURCE_DESC resource_desc;
    D3D12_RANGE read_range;
    unsigned int miplevel;
    ID3D12Device *device;
    DXGI_FORMAT format;
    HRESULT hr;

    hr = ID3D12Resource_GetDevice(texture, &IID_ID3D12Device, (void **)&device);
    ok(hr == S_OK, "Failed to get device, hr %#x.\n", hr);

    resource_desc = ID3D12Resource_GetDesc(texture);
    ok(resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER,
            "Resource %p is not texture.\n", texture);
    ok(resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D,
            "Readback not implemented for 3D textures.\n");

    miplevel = sub_resource % resource_desc.MipLevels;
    rb->width = max(1, resource_desc.Width >> miplevel);
    rb->height = max(1, resource_desc.Height >> miplevel);
    rb->row_pitch = align(rb->width * format_size(resource_desc.Format), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    rb->data = NULL;

    format = resource_desc.Format;

    rb->resource = create_readback_buffer(device, rb->row_pitch * rb->height);

    dst_location.pResource = rb->resource;
    dst_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst_location.PlacedFootprint.Offset = 0;
    dst_location.PlacedFootprint.Footprint.Format = format;
    dst_location.PlacedFootprint.Footprint.Width = rb->width;
    dst_location.PlacedFootprint.Footprint.Height = rb->height;
    dst_location.PlacedFootprint.Footprint.Depth = 1;
    dst_location.PlacedFootprint.Footprint.RowPitch = rb->row_pitch;

    src_location.pResource = texture;
    src_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src_location.SubresourceIndex = sub_resource;

    ID3D12GraphicsCommandList_CopyTextureRegion(command_list, &dst_location, 0, 0, 0, &src_location, NULL);
    hr = ID3D12GraphicsCommandList_Close(command_list);
    ok(hr == S_OK, "Failed to close command list, hr %#x.\n", hr);

    exec_command_list(queue, command_list);
    wait_queue_idle(device, queue);
    ID3D12Device_Release(device);

    read_range.Begin = 0;
    read_range.End = resource_desc.Width;
    hr = ID3D12Resource_Map(rb->resource, 0, &read_range, &rb->data);
    ok(hr == S_OK, "Failed to map readback buffer, hr %#x.\n", hr);
}

static void *get_readback_data(struct resource_readback *rb, unsigned int x, unsigned int y,
        size_t element_size)
{
    return &((BYTE *)rb->data)[rb->row_pitch * y + x * element_size];
}

static unsigned int get_readback_uint(struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return *(unsigned int *)get_readback_data(rb, x, y, sizeof(unsigned int));
}

static void release_resource_readback(struct resource_readback *rb)
{
    D3D12_RANGE range = {0, 0};
    ID3D12Resource_Unmap(rb->resource, 0, &range);
    ID3D12Resource_Release(rb->resource);
}

#define check_readback_data_uint(a, b, c, d) check_readback_data_uint_(__LINE__, a, b, c, d)
static void check_readback_data_uint_(unsigned int line, struct resource_readback *rb,
        const RECT *rect, unsigned int expected, unsigned int max_diff)
{
    RECT r = {0, 0, rb->width, rb->height};
    unsigned int x = 0, y;
    BOOL all_match = TRUE;
    unsigned int got = 0;

    if (rect)
        r = *rect;

    for (y = r.top; y < r.bottom; ++y)
    {
        for (x = r.left; x < r.right; ++x)
        {
            got = get_readback_uint(rb, x, y);
            if (!compare_color(got, expected, max_diff))
            {
                all_match = FALSE;
                break;
            }
        }
        if (!all_match)
            break;
    }
    ok_(__FILE__, line)(all_match, "Got 0x%08x, expected 0x%08x at (%u, %u).\n", got, expected, x, y);
}

#define check_sub_resource_uint(a, b, c, d, e, f) check_sub_resource_uint_(__LINE__, a, b, c, d, e, f)
static void check_sub_resource_uint_(unsigned int line, ID3D12Resource *texture,
        unsigned int sub_resource_idx, ID3D12CommandQueue *queue, ID3D12GraphicsCommandList *command_list,
        unsigned int expected, unsigned int max_diff)
{
    struct resource_readback rb;

    get_texture_readback_with_command_list(texture, 0, &rb, queue, command_list);
    check_readback_data_uint_(line, &rb, NULL, expected, max_diff);
    release_resource_readback(&rb);
}

static void test_interfaces(void)
{
    D3D12_COMMAND_QUEUE_DESC desc;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    check_interface(device, &IID_ID3D12Object, TRUE);
    check_interface(device, &IID_ID3D12DeviceChild, FALSE);
    check_interface(device, &IID_ID3D12Pageable, FALSE);
    check_interface(device, &IID_ID3D12Device, TRUE);

    check_interface(device, &IID_IDXGIObject, FALSE);
    check_interface(device, &IID_IDXGIDeviceSubObject, FALSE);
    check_interface(device, &IID_IDXGIDevice, FALSE);
    check_interface(device, &IID_IDXGIDevice1, FALSE);
    check_interface(device, &IID_IDXGIDevice2, FALSE);
    check_interface(device, &IID_IDXGIDevice3, FALSE);
    check_interface(device, &IID_IDXGIDevice4, FALSE);

    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    hr = ID3D12Device_CreateCommandQueue(device, &desc, &IID_ID3D12CommandQueue, (void **)&queue);
    ok(hr == S_OK, "Failed to create command queue, hr %#x.\n", hr);

    check_interface(queue, &IID_ID3D12Object, TRUE);
    check_interface(queue, &IID_ID3D12DeviceChild, TRUE);
    check_interface(queue, &IID_ID3D12Pageable, TRUE);
    check_interface(queue, &IID_ID3D12CommandQueue, TRUE);

    check_interface(queue, &IID_IDXGIObject, FALSE);
    check_interface(queue, &IID_IDXGIDeviceSubObject, FALSE);
    check_interface(queue, &IID_IDXGIDevice, FALSE);
    check_interface(queue, &IID_IDXGIDevice1, FALSE);
    check_interface(queue, &IID_IDXGIDevice2, FALSE);
    check_interface(queue, &IID_IDXGIDevice3, FALSE);
    check_interface(queue, &IID_IDXGIDevice4, FALSE);

    refcount = ID3D12CommandQueue_Release(queue);
    ok(!refcount, "Command queue has %u references left.\n", refcount);
    refcount = ID3D12Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_draw(void)
{
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    ID3D12GraphicsCommandList *command_list;
    struct test_context context;
    ID3D12CommandQueue *queue;

    if (!init_test_context(&context))
        return;
    command_list = context.list;
    queue = context.queue;

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, FALSE, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_sub_resource_state(command_list, context.render_target, 0,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

    check_sub_resource_uint(context.render_target, 0, queue, command_list, 0xff00ff00, 0);

    destroy_test_context(&context);
}

static void test_swapchain_draw(void)
{
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    ID3D12GraphicsCommandList *command_list;
    struct test_context context;
    ID3D12Resource *backbuffer;
    IDXGISwapChain3 *swapchain;
    ID3D12CommandQueue *queue;
    unsigned int index;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    RECT rect;
    BOOL ret;

    if (!init_test_context(&context))
        return;
    command_list = context.list;
    queue = context.queue;

    window = create_window(0);
    ret = GetClientRect(window, &rect);
    ok(ret, "Failed to get client rect.\n");
    set_viewport(&context.viewport, 0.0f, 0.0f, rect.right, rect.bottom, 0.0f, 1.0f);
    swapchain = create_swapchain(queue, window, rect.right, rect.bottom);
    index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain);
    hr = IDXGISwapChain3_GetBuffer(swapchain, index, &IID_ID3D12Resource, (void **)&backbuffer);
    ok(hr == S_OK, "Failed to get swapchain buffer %u, hr %#x.\n", index, hr);
    ID3D12Device_CreateRenderTargetView(context.device, backbuffer, NULL, context.rtv);

    transition_sub_resource_state(command_list, backbuffer, 0,
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, FALSE, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &rect);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_sub_resource_state(command_list, backbuffer, 0,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

    check_sub_resource_uint(backbuffer, 0, queue, command_list, 0xff00ff00, 0);

    refcount = ID3D12Resource_Release(backbuffer);
    ok(!refcount, "Backbuffer has %u references left.\n", refcount);
    refcount = IDXGISwapChain3_Release(swapchain);
    ok(!refcount, "Swapchain has %u references left.\n", refcount);
    DestroyWindow(window);
    destroy_test_context(&context);
}

START_TEST(d3d12)
{
    BOOL enable_debug_layer = FALSE;
    unsigned int argc, i;
    ID3D12Debug *debug;
    char **argv;

    argc = winetest_get_mainargs(&argv);
    for (i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--validate"))
            enable_debug_layer = TRUE;
        else if (!strcmp(argv[i], "--warp"))
            use_warp_adapter = TRUE;
        else if (!strcmp(argv[i], "--adapter") && i + 1 < argc)
            use_adapter_idx = atoi(argv[++i]);
    }

    if (enable_debug_layer && SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug, (void **)&debug)))
    {
        ID3D12Debug_EnableDebugLayer(debug);
        ID3D12Debug_Release(debug);
    }

    print_adapter_info();

    test_interfaces();
    test_draw();
    test_swapchain_draw();
}
