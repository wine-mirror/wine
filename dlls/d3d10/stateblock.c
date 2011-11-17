/*
 * Copyright 2011 Henri Verbeet for CodeWeavers
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "d3d10_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10);

struct d3d10_stateblock
{
    ID3D10StateBlock ID3D10StateBlock_iface;
    LONG refcount;
};

static inline struct d3d10_stateblock *impl_from_ID3D10StateBlock(ID3D10StateBlock *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_stateblock, ID3D10StateBlock_iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_stateblock_QueryInterface(ID3D10StateBlock *iface, REFIID iid, void **object)
{
    struct d3d10_stateblock *stateblock;

    TRACE("iface %p, iid %s, object %p.\n", iface, debugstr_guid(iid), object);

    stateblock = impl_from_ID3D10StateBlock(iface);

    if (IsEqualGUID(iid, &IID_ID3D10StateBlock)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IUnknown_AddRef(&stateblock->ID3D10StateBlock_iface);
        *object = &stateblock->ID3D10StateBlock_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *object = NULL;

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d10_stateblock_AddRef(ID3D10StateBlock *iface)
{
    struct d3d10_stateblock *stateblock = impl_from_ID3D10StateBlock(iface);
    ULONG refcount = InterlockedIncrement(&stateblock->refcount);

    TRACE("%p increasing refcount to %u.\n", stateblock, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_stateblock_Release(ID3D10StateBlock *iface)
{
    struct d3d10_stateblock *stateblock = impl_from_ID3D10StateBlock(iface);
    ULONG refcount = InterlockedDecrement(&stateblock->refcount);

    TRACE("%p decreasing refcount to %u.\n", stateblock, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, stateblock);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d10_stateblock_Capture(ID3D10StateBlock *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_stateblock_Apply(ID3D10StateBlock *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_stateblock_ReleaseAllDeviceObjects(ID3D10StateBlock *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_stateblock_GetDevice(ID3D10StateBlock *iface, ID3D10Device **device)
{
    FIXME("iface %p, device %p stub!\n", iface, device);

    return E_NOTIMPL;
}

static const struct ID3D10StateBlockVtbl d3d10_stateblock_vtbl =
{
    /* IUnknown methods */
    d3d10_stateblock_QueryInterface,
    d3d10_stateblock_AddRef,
    d3d10_stateblock_Release,
    /* ID3D10StateBlock methods */
    d3d10_stateblock_Capture,
    d3d10_stateblock_Apply,
    d3d10_stateblock_ReleaseAllDeviceObjects,
    d3d10_stateblock_GetDevice,
};

HRESULT WINAPI D3D10CreateStateBlock(ID3D10Device *device,
        D3D10_STATE_BLOCK_MASK *mask, ID3D10StateBlock **stateblock)
{
    struct d3d10_stateblock *object;

    FIXME("device %p, mask %p, stateblock %p stub!\n", device, mask, stateblock);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate D3D10 stateblock object memory.\n");
        return E_OUTOFMEMORY;
    }

    object->ID3D10StateBlock_iface.lpVtbl = &d3d10_stateblock_vtbl;
    object->refcount = 1;

    TRACE("Created stateblock %p.\n", object);
    *stateblock = &object->ID3D10StateBlock_iface;

    return S_OK;
}

static BOOL stateblock_mask_get_bit(BYTE *field, UINT field_size, UINT idx)
{
    if (idx >= field_size)
        return FALSE;

    return field[idx >> 3] & (1 << (idx & 7));
}

static HRESULT stateblock_mask_set_bits(BYTE *field, UINT field_size, UINT start_bit, UINT count)
{
    UINT end_bit = start_bit + count;
    BYTE start_mask = 0xff << (start_bit & 7);
    BYTE end_mask = 0x7f >> (~end_bit & 7);
    UINT start_idx = start_bit >> 3;
    UINT end_idx = end_bit >> 3;

    if (start_bit >= field_size || field_size - start_bit < count)
        return E_INVALIDARG;

    if (start_idx == end_idx)
    {
        field[start_idx] |= start_mask & end_mask;
        return S_OK;
    }

    if (start_bit & 7)
    {
        field[start_idx] |= start_mask;
        ++start_idx;
    }

    memset(&field[start_idx], 0xff, end_idx - start_idx);

    if (end_bit & 7)
        field[end_idx] |= end_mask;

    return S_OK;
}

static HRESULT stateblock_mask_clear_bits(BYTE *field, UINT field_size, UINT start_bit, UINT count)
{
    UINT end_bit = start_bit + count;
    BYTE start_mask = 0x7f >> (~start_bit & 7);
    BYTE end_mask = 0xff << (end_bit & 7);
    UINT start_idx = start_bit >> 3;
    UINT end_idx = end_bit >> 3;

    if (start_bit >= field_size || field_size - start_bit < count)
        return E_INVALIDARG;

    if (start_idx == end_idx)
    {
        field[start_idx] &= start_mask | end_mask;
        return S_OK;
    }

    if (start_bit & 7)
    {
        field[start_idx] &= start_mask;
        ++start_idx;
    }

    memset(&field[start_idx], 0, end_idx - start_idx);

    if (end_bit & 7)
        field[end_idx] &= end_mask;

    return S_OK;
}

HRESULT WINAPI D3D10StateBlockMaskDifference(D3D10_STATE_BLOCK_MASK *mask_x,
        D3D10_STATE_BLOCK_MASK *mask_y, D3D10_STATE_BLOCK_MASK *result)
{
    UINT count = sizeof(*result) / sizeof(DWORD);
    UINT i;

    TRACE("mask_x %p, mask_y %p, result %p.\n", mask_x, mask_y, result);

    if (!mask_x || !mask_y || !result)
        return E_INVALIDARG;

    for (i = 0; i < count; ++i)
    {
        ((DWORD *)result)[i] = ((DWORD *)mask_x)[i] ^ ((DWORD *)mask_y)[i];
    }
    for (i = count * sizeof(DWORD); i < sizeof(*result); ++i)
    {
        ((BYTE *)result)[i] = ((BYTE *)mask_x)[i] ^ ((BYTE *)mask_y)[i];
    }

    return S_OK;
}

HRESULT WINAPI D3D10StateBlockMaskDisableAll(D3D10_STATE_BLOCK_MASK *mask)
{
    TRACE("mask %p.\n", mask);

    if (!mask)
        return E_INVALIDARG;

    memset(mask, 0, sizeof(*mask));

    return S_OK;
}

HRESULT WINAPI D3D10StateBlockMaskDisableCapture(D3D10_STATE_BLOCK_MASK *mask,
        D3D10_DEVICE_STATE_TYPES state_type, UINT start_idx, UINT count)
{
    TRACE("mask %p state_type %s, start_idx %u, count %u.\n",
            mask, debug_d3d10_device_state_types(state_type), start_idx, count);

    if (!mask)
        return E_INVALIDARG;

    switch (state_type)
    {
        case D3D10_DST_SO_BUFFERS:
            return stateblock_mask_clear_bits(&mask->SOBuffers, 1, start_idx, count);
        case D3D10_DST_OM_RENDER_TARGETS:
            return stateblock_mask_clear_bits(&mask->OMRenderTargets, 1, start_idx, count);
        case D3D10_DST_DEPTH_STENCIL_STATE:
            return stateblock_mask_clear_bits(&mask->OMDepthStencilState, 1, start_idx, count);
        case D3D10_DST_BLEND_STATE:
            return stateblock_mask_clear_bits(&mask->OMBlendState, 1, start_idx, count);
        case D3D10_DST_VS:
            return stateblock_mask_clear_bits(&mask->VS, 1, start_idx, count);
        case D3D10_DST_VS_SAMPLERS:
            return stateblock_mask_clear_bits(mask->VSSamplers,
                    D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, start_idx, count);
        case D3D10_DST_VS_SHADER_RESOURCES:
            return stateblock_mask_clear_bits(mask->VSShaderResources,
                    D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, start_idx, count);
        case D3D10_DST_VS_CONSTANT_BUFFERS:
            return stateblock_mask_clear_bits(mask->VSConstantBuffers,
                    D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, start_idx, count);
        case D3D10_DST_GS:
            return stateblock_mask_clear_bits(&mask->GS, 1, start_idx, count);
        case D3D10_DST_GS_SAMPLERS:
            return stateblock_mask_clear_bits(mask->GSSamplers,
                    D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, start_idx, count);
        case D3D10_DST_GS_SHADER_RESOURCES:
            return stateblock_mask_clear_bits(mask->GSShaderResources,
                    D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, start_idx, count);
        case D3D10_DST_GS_CONSTANT_BUFFERS:
            return stateblock_mask_clear_bits(mask->GSConstantBuffers,
                    D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, start_idx, count);
        case D3D10_DST_PS:
            return stateblock_mask_clear_bits(&mask->PS, 1, start_idx, count);
        case D3D10_DST_PS_SAMPLERS:
            return stateblock_mask_clear_bits(mask->PSSamplers,
                    D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, start_idx, count);
        case D3D10_DST_PS_SHADER_RESOURCES:
            return stateblock_mask_clear_bits(mask->PSShaderResources,
                    D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, start_idx, count);
        case D3D10_DST_PS_CONSTANT_BUFFERS:
            return stateblock_mask_clear_bits(mask->PSConstantBuffers,
                    D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, start_idx, count);
        case D3D10_DST_IA_VERTEX_BUFFERS:
            return stateblock_mask_clear_bits(mask->IAVertexBuffers,
                    D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, start_idx, count);
        case D3D10_DST_IA_INDEX_BUFFER:
            return stateblock_mask_clear_bits(&mask->IAIndexBuffer, 1, start_idx, count);
        case D3D10_DST_IA_INPUT_LAYOUT:
            return stateblock_mask_clear_bits(&mask->IAInputLayout, 1, start_idx, count);
        case D3D10_DST_IA_PRIMITIVE_TOPOLOGY:
            return stateblock_mask_clear_bits(&mask->IAPrimitiveTopology, 1, start_idx, count);
        case D3D10_DST_RS_VIEWPORTS:
            return stateblock_mask_clear_bits(&mask->RSViewports, 1, start_idx, count);
        case D3D10_DST_RS_SCISSOR_RECTS:
            return stateblock_mask_clear_bits(&mask->RSScissorRects, 1, start_idx, count);
        case D3D10_DST_RS_RASTERIZER_STATE:
            return stateblock_mask_clear_bits(&mask->RSRasterizerState, 1, start_idx, count);
        case D3D10_DST_PREDICATION:
            return stateblock_mask_clear_bits(&mask->Predication, 1, start_idx, count);
        default:
            FIXME("Unhandled state_type %#x.\n", state_type);
            return E_INVALIDARG;
    }
}

HRESULT WINAPI D3D10StateBlockMaskEnableAll(D3D10_STATE_BLOCK_MASK *mask)
{
    TRACE("mask %p.\n", mask);

    if (!mask)
        return E_INVALIDARG;

    memset(mask, 0xff, sizeof(*mask));

    return S_OK;
}

HRESULT WINAPI D3D10StateBlockMaskEnableCapture(D3D10_STATE_BLOCK_MASK *mask,
        D3D10_DEVICE_STATE_TYPES state_type, UINT start_idx, UINT count)
{
    TRACE("mask %p state_type %s, start_idx %u, count %u.\n",
            mask, debug_d3d10_device_state_types(state_type), start_idx, count);

    if (!mask)
        return E_INVALIDARG;

    switch (state_type)
    {
        case D3D10_DST_SO_BUFFERS:
            return stateblock_mask_set_bits(&mask->SOBuffers, 1, start_idx, count);
        case D3D10_DST_OM_RENDER_TARGETS:
            return stateblock_mask_set_bits(&mask->OMRenderTargets, 1, start_idx, count);
        case D3D10_DST_DEPTH_STENCIL_STATE:
            return stateblock_mask_set_bits(&mask->OMDepthStencilState, 1, start_idx, count);
        case D3D10_DST_BLEND_STATE:
            return stateblock_mask_set_bits(&mask->OMBlendState, 1, start_idx, count);
        case D3D10_DST_VS:
            return stateblock_mask_set_bits(&mask->VS, 1, start_idx, count);
        case D3D10_DST_VS_SAMPLERS:
            return stateblock_mask_set_bits(mask->VSSamplers,
                    D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, start_idx, count);
        case D3D10_DST_VS_SHADER_RESOURCES:
            return stateblock_mask_set_bits(mask->VSShaderResources,
                    D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, start_idx, count);
        case D3D10_DST_VS_CONSTANT_BUFFERS:
            return stateblock_mask_set_bits(mask->VSConstantBuffers,
                    D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, start_idx, count);
        case D3D10_DST_GS:
            return stateblock_mask_set_bits(&mask->GS, 1, start_idx, count);
        case D3D10_DST_GS_SAMPLERS:
            return stateblock_mask_set_bits(mask->GSSamplers,
                    D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, start_idx, count);
        case D3D10_DST_GS_SHADER_RESOURCES:
            return stateblock_mask_set_bits(mask->GSShaderResources,
                    D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, start_idx, count);
        case D3D10_DST_GS_CONSTANT_BUFFERS:
            return stateblock_mask_set_bits(mask->GSConstantBuffers,
                    D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, start_idx, count);
        case D3D10_DST_PS:
            return stateblock_mask_set_bits(&mask->PS, 1, start_idx, count);
        case D3D10_DST_PS_SAMPLERS:
            return stateblock_mask_set_bits(mask->PSSamplers,
                    D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, start_idx, count);
        case D3D10_DST_PS_SHADER_RESOURCES:
            return stateblock_mask_set_bits(mask->PSShaderResources,
                    D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, start_idx, count);
        case D3D10_DST_PS_CONSTANT_BUFFERS:
            return stateblock_mask_set_bits(mask->PSConstantBuffers,
                    D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, start_idx, count);
        case D3D10_DST_IA_VERTEX_BUFFERS:
            return stateblock_mask_set_bits(mask->IAVertexBuffers,
                    D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, start_idx, count);
        case D3D10_DST_IA_INDEX_BUFFER:
            return stateblock_mask_set_bits(&mask->IAIndexBuffer, 1, start_idx, count);
        case D3D10_DST_IA_INPUT_LAYOUT:
            return stateblock_mask_set_bits(&mask->IAInputLayout, 1, start_idx, count);
        case D3D10_DST_IA_PRIMITIVE_TOPOLOGY:
            return stateblock_mask_set_bits(&mask->IAPrimitiveTopology, 1, start_idx, count);
        case D3D10_DST_RS_VIEWPORTS:
            return stateblock_mask_set_bits(&mask->RSViewports, 1, start_idx, count);
        case D3D10_DST_RS_SCISSOR_RECTS:
            return stateblock_mask_set_bits(&mask->RSScissorRects, 1, start_idx, count);
        case D3D10_DST_RS_RASTERIZER_STATE:
            return stateblock_mask_set_bits(&mask->RSRasterizerState, 1, start_idx, count);
        case D3D10_DST_PREDICATION:
            return stateblock_mask_set_bits(&mask->Predication, 1, start_idx, count);
        default:
            FIXME("Unhandled state_type %#x.\n", state_type);
            return E_INVALIDARG;
    }
}

BOOL WINAPI D3D10StateBlockMaskGetSetting(D3D10_STATE_BLOCK_MASK *mask,
        D3D10_DEVICE_STATE_TYPES state_type, UINT idx)
{
    TRACE("mask %p state_type %s, idx %u.\n",
            mask, debug_d3d10_device_state_types(state_type), idx);

    if (!mask)
        return FALSE;

    switch (state_type)
    {
        case D3D10_DST_SO_BUFFERS:
            return stateblock_mask_get_bit(&mask->SOBuffers, 1, idx);
        case D3D10_DST_OM_RENDER_TARGETS:
            return stateblock_mask_get_bit(&mask->OMRenderTargets, 1, idx);
        case D3D10_DST_DEPTH_STENCIL_STATE:
            return stateblock_mask_get_bit(&mask->OMDepthStencilState, 1, idx);
        case D3D10_DST_BLEND_STATE:
            return stateblock_mask_get_bit(&mask->OMBlendState, 1, idx);
        case D3D10_DST_VS:
            return stateblock_mask_get_bit(&mask->VS, 1, idx);
        case D3D10_DST_VS_SAMPLERS:
            return stateblock_mask_get_bit(mask->VSSamplers,
                    D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, idx);
        case D3D10_DST_VS_SHADER_RESOURCES:
            return stateblock_mask_get_bit(mask->VSShaderResources,
                    D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, idx);
        case D3D10_DST_VS_CONSTANT_BUFFERS:
            return stateblock_mask_get_bit(mask->VSConstantBuffers,
                    D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, idx);
        case D3D10_DST_GS:
            return stateblock_mask_get_bit(&mask->GS, 1, idx);
        case D3D10_DST_GS_SAMPLERS:
            return stateblock_mask_get_bit(mask->GSSamplers,
                    D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, idx);
        case D3D10_DST_GS_SHADER_RESOURCES:
            return stateblock_mask_get_bit(mask->GSShaderResources,
                    D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, idx);
        case D3D10_DST_GS_CONSTANT_BUFFERS:
            return stateblock_mask_get_bit(mask->GSConstantBuffers,
                    D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, idx);
        case D3D10_DST_PS:
            return stateblock_mask_get_bit(&mask->PS, 1, idx);
        case D3D10_DST_PS_SAMPLERS:
            return stateblock_mask_get_bit(mask->PSSamplers,
                    D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, idx);
        case D3D10_DST_PS_SHADER_RESOURCES:
            return stateblock_mask_get_bit(mask->PSShaderResources,
                    D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, idx);
        case D3D10_DST_PS_CONSTANT_BUFFERS:
            return stateblock_mask_get_bit(mask->PSConstantBuffers,
                    D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, idx);
        case D3D10_DST_IA_VERTEX_BUFFERS:
            return stateblock_mask_get_bit(mask->IAVertexBuffers,
                    D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, idx);
        case D3D10_DST_IA_INDEX_BUFFER:
            return stateblock_mask_get_bit(&mask->IAIndexBuffer, 1, idx);
        case D3D10_DST_IA_INPUT_LAYOUT:
            return stateblock_mask_get_bit(&mask->IAInputLayout, 1, idx);
        case D3D10_DST_IA_PRIMITIVE_TOPOLOGY:
            return stateblock_mask_get_bit(&mask->IAPrimitiveTopology, 1, idx);
        case D3D10_DST_RS_VIEWPORTS:
            return stateblock_mask_get_bit(&mask->RSViewports, 1, idx);
        case D3D10_DST_RS_SCISSOR_RECTS:
            return stateblock_mask_get_bit(&mask->RSScissorRects, 1, idx);
        case D3D10_DST_RS_RASTERIZER_STATE:
            return stateblock_mask_get_bit(&mask->RSRasterizerState, 1, idx);
        case D3D10_DST_PREDICATION:
            return stateblock_mask_get_bit(&mask->Predication, 1, idx);
        default:
            FIXME("Unhandled state_type %#x.\n", state_type);
            return FALSE;
    }
}

HRESULT WINAPI D3D10StateBlockMaskIntersect(D3D10_STATE_BLOCK_MASK *mask_x,
        D3D10_STATE_BLOCK_MASK *mask_y, D3D10_STATE_BLOCK_MASK *result)
{
    UINT count = sizeof(*result) / sizeof(DWORD);
    UINT i;

    TRACE("mask_x %p, mask_y %p, result %p.\n", mask_x, mask_y, result);

    if (!mask_x || !mask_y || !result)
        return E_INVALIDARG;

    for (i = 0; i < count; ++i)
    {
        ((DWORD *)result)[i] = ((DWORD *)mask_x)[i] & ((DWORD *)mask_y)[i];
    }
    for (i = count * sizeof(DWORD); i < sizeof(*result); ++i)
    {
        ((BYTE *)result)[i] = ((BYTE *)mask_x)[i] & ((BYTE *)mask_y)[i];
    }

    return S_OK;
}

HRESULT WINAPI D3D10StateBlockMaskUnion(D3D10_STATE_BLOCK_MASK *mask_x,
        D3D10_STATE_BLOCK_MASK *mask_y, D3D10_STATE_BLOCK_MASK *result)
{
    UINT count = sizeof(*result) / sizeof(DWORD);
    UINT i;

    TRACE("mask_x %p, mask_y %p, result %p.\n", mask_x, mask_y, result);

    if (!mask_x || !mask_y || !result)
        return E_INVALIDARG;

    for (i = 0; i < count; ++i)
    {
        ((DWORD *)result)[i] = ((DWORD *)mask_x)[i] | ((DWORD *)mask_y)[i];
    }
    for (i = count * sizeof(DWORD); i < sizeof(*result); ++i)
    {
        ((BYTE *)result)[i] = ((BYTE *)mask_x)[i] | ((BYTE *)mask_y)[i];
    }

    return S_OK;
}
