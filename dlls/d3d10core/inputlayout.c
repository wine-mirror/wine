/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include "d3d10core_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10core);

struct input_signature_element
{
    const char *semantic_name;
    UINT semantic_idx;
    DWORD unknown; /* system value semantic? */
    DWORD component_type;
    UINT register_idx;
    DWORD mask;
};

struct input_signature
{
    struct input_signature_element *elements;
    UINT element_count;
};

static HRESULT parse_isgn(const char *data, struct input_signature *is)
{
    struct input_signature_element *e;
    const char *ptr = data;
    unsigned int i;
    DWORD count;

    read_dword(&ptr, &count);
    TRACE("%u elements\n", count);

    skip_dword_unknown(&ptr, 1);

    e = HeapAlloc(GetProcessHeap(), 0, count * sizeof(*e));
    if (!e)
    {
        ERR("Failed to allocate input signature memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < count; ++i)
    {
        UINT name_offset;

        read_dword(&ptr, &name_offset);
        e[i].semantic_name = data + name_offset;
        read_dword(&ptr, &e[i].semantic_idx);
        read_dword(&ptr, &e[i].unknown);
        read_dword(&ptr, &e[i].component_type);
        read_dword(&ptr, &e[i].register_idx);
        read_dword(&ptr, &e[i].mask);

        TRACE("semantic: %s, semantic idx: %u, unknown %#x, type %u, register idx: %u, use_mask %#x, input_mask %#x\n",
                e[i].semantic_name, e[i].semantic_idx, e[i].unknown, e[i].component_type,
                e[i].register_idx, (e[i].mask >> 8) & 0xff, e[i].mask & 0xff);
    }

    is->elements = e;
    is->element_count = count;

    return S_OK;
}

static HRESULT isgn_handler(const char *data, DWORD data_size, DWORD tag, void *ctx)
{
    struct input_signature *is = ctx;
    const char *ptr = data;
    char tag_str[5];

    switch(tag)
    {
        case TAG_ISGN:
            return parse_isgn(ptr, is);

        default:
            memcpy(tag_str, &tag, 4);
            tag_str[4] = '\0';
            FIXME("Unhandled chunk %s\n", tag_str);
            return S_OK;
    }
}

HRESULT d3d10_input_layout_to_wined3d_declaration(const D3D10_INPUT_ELEMENT_DESC *element_descs,
        UINT element_count, const void *shader_byte_code, SIZE_T shader_byte_code_length,
        WINED3DVERTEXELEMENT **wined3d_elements, UINT *wined3d_element_count)
{
    struct input_signature is;
    HRESULT hr;
    UINT i;

    hr = parse_dxbc(shader_byte_code, shader_byte_code_length, isgn_handler, &is);
    if (FAILED(hr))
    {
        ERR("Failed to parse input signature.\n");
        return E_FAIL;
    }

    *wined3d_elements = HeapAlloc(GetProcessHeap(), 0, element_count * sizeof(**wined3d_elements));
    if (!*wined3d_elements)
    {
        ERR("Failed to allocate wined3d vertex element array memory.\n");
        HeapFree(GetProcessHeap(), 0, is.elements);
        return E_OUTOFMEMORY;
    }
    *wined3d_element_count = 0;

    for (i = 0; i < element_count; ++i)
    {
        UINT j;

        for (j = 0; j < is.element_count; ++j)
        {
            if (!strcmp(element_descs[i].SemanticName, is.elements[j].semantic_name)
                    && element_descs[i].SemanticIndex == is.elements[j].semantic_idx)
            {
                WINED3DVERTEXELEMENT *e = &(*wined3d_elements)[(*wined3d_element_count)++];
                const D3D10_INPUT_ELEMENT_DESC *f = &element_descs[i];

                e->format = wined3dformat_from_dxgi_format(f->Format);
                e->input_slot = f->InputSlot;
                e->offset = f->AlignedByteOffset;
                e->output_slot = is.elements[j].register_idx;
                e->method = WINED3DDECLMETHOD_DEFAULT;
                e->usage = 0;
                e->usage_idx = 0;

                if (f->AlignedByteOffset == D3D10_APPEND_ALIGNED_ELEMENT)
                    FIXME("D3D10_APPEND_ALIGNED_ELEMENT not supported\n");
                if (f->InputSlotClass != D3D10_INPUT_PER_VERTEX_DATA)
                    FIXME("Ignoring input slot class (%#x)\n", f->InputSlotClass);
                if (f->InstanceDataStepRate)
                    FIXME("Ignoring instace data step rate (%#x)\n", f->InstanceDataStepRate);

                break;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, is.elements);

    return S_OK;
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_input_layout_QueryInterface(ID3D10InputLayout *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10InputLayout)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d10_input_layout_AddRef(ID3D10InputLayout *iface)
{
    struct d3d10_input_layout *This = (struct d3d10_input_layout *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_input_layout_Release(ID3D10InputLayout *iface)
{
    struct d3d10_input_layout *This = (struct d3d10_input_layout *)iface;
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        IWineD3DVertexDeclaration_Release(This->wined3d_decl);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_input_layout_GetDevice(ID3D10InputLayout *iface, ID3D10Device **device)
{
    FIXME("iface %p, device %p stub!\n", iface, device);
}

static HRESULT STDMETHODCALLTYPE d3d10_input_layout_GetPrivateData(ID3D10InputLayout *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_input_layout_SetPrivateData(ID3D10InputLayout *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_input_layout_SetPrivateDataInterface(ID3D10InputLayout *iface,
        REFGUID guid, const IUnknown *data)
{
    FIXME("iface %p, guid %s, data %p stub!\n", iface, debugstr_guid(guid), data);

    return E_NOTIMPL;
}

const struct ID3D10InputLayoutVtbl d3d10_input_layout_vtbl =
{
    /* IUnknown methods */
    d3d10_input_layout_QueryInterface,
    d3d10_input_layout_AddRef,
    d3d10_input_layout_Release,
    /* ID3D10DeviceChild methods */
    d3d10_input_layout_GetDevice,
    d3d10_input_layout_GetPrivateData,
    d3d10_input_layout_SetPrivateData,
    d3d10_input_layout_SetPrivateDataInterface,
};
