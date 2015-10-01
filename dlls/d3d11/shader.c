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

#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static HRESULT shdr_handler(const char *data, DWORD data_size, DWORD tag, void *ctx)
{
    struct d3d_shader_info *shader_info = ctx;
    HRESULT hr;

    switch (tag)
    {
        case TAG_ISGN:
            if (FAILED(hr = shader_parse_signature(data, data_size, shader_info->input_signature)))
                return hr;
            break;

        case TAG_OSGN:
            if (FAILED(hr = shader_parse_signature(data, data_size, shader_info->output_signature)))
                return hr;
            break;

        case TAG_SHDR:
            shader_info->shader_code = (const DWORD *)data;
            break;

        default:
            FIXME("Unhandled chunk %s\n", debugstr_an((const char *)&tag, 4));
            break;
    }

    return S_OK;
}

static HRESULT shader_extract_from_dxbc(const void *dxbc, SIZE_T dxbc_length, struct d3d_shader_info *shader_info)
{
    HRESULT hr;

    shader_info->shader_code = NULL;
    memset(shader_info->input_signature, 0, sizeof(*shader_info->input_signature));
    memset(shader_info->output_signature, 0, sizeof(*shader_info->output_signature));

    hr = parse_dxbc(dxbc, dxbc_length, shdr_handler, shader_info);
    if (!shader_info->shader_code) hr = E_INVALIDARG;

    if (FAILED(hr))
    {
        ERR("Failed to parse shader, hr %#x\n", hr);
        shader_free_signature(shader_info->input_signature);
        shader_free_signature(shader_info->output_signature);
    }

    return hr;
}

HRESULT shader_parse_signature(const char *data, DWORD data_size, struct wined3d_shader_signature *s)
{
    struct wined3d_shader_signature_element *e;
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
        read_dword(&ptr, &e[i].sysval_semantic);
        read_dword(&ptr, &e[i].component_type);
        read_dword(&ptr, &e[i].register_idx);
        read_dword(&ptr, &e[i].mask);

        TRACE("semantic: %s, semantic idx: %u, sysval_semantic %#x, "
                "type %u, register idx: %u, use_mask %#x, input_mask %#x\n",
                debugstr_a(e[i].semantic_name), e[i].semantic_idx, e[i].sysval_semantic,
                e[i].component_type, e[i].register_idx, (e[i].mask >> 8) & 0xff, e[i].mask & 0xff);
    }

    s->elements = e;
    s->element_count = count;

    return S_OK;
}

void shader_free_signature(struct wined3d_shader_signature *s)
{
    HeapFree(GetProcessHeap(), 0, s->elements);
}

/* ID3D11VertexShader methods */

static inline struct d3d_vertex_shader *impl_from_ID3D11VertexShader(ID3D11VertexShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_vertex_shader, ID3D11VertexShader_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_vertex_shader_QueryInterface(ID3D11VertexShader *iface,
        REFIID riid, void **object)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11VertexShader)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11VertexShader_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10VertexShader)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        IUnknown_AddRef(&shader->ID3D10VertexShader_iface);
        *object = &shader->ID3D10VertexShader_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_vertex_shader_AddRef(ID3D11VertexShader *iface)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %u.\n", shader, refcount);

    if (refcount == 1)
    {
        ID3D11Device_AddRef(shader->device);
        wined3d_mutex_lock();
        wined3d_shader_incref(shader->wined3d_shader);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_vertex_shader_Release(ID3D11VertexShader *iface)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %u.\n", shader, refcount);

    if (!refcount)
    {
        ID3D11Device *device = shader->device;

        wined3d_mutex_lock();
        wined3d_shader_decref(shader->wined3d_shader);
        wined3d_mutex_unlock();
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_vertex_shader_GetDevice(ID3D11VertexShader *iface,
        ID3D11Device **device)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = shader->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_vertex_shader_GetPrivateData(ID3D11VertexShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_vertex_shader_SetPrivateData(ID3D11VertexShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_vertex_shader_SetPrivateDataInterface(ID3D11VertexShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D11VertexShaderVtbl d3d11_vertex_shader_vtbl =
{
    /* IUnknown methods */
    d3d11_vertex_shader_QueryInterface,
    d3d11_vertex_shader_AddRef,
    d3d11_vertex_shader_Release,
    /* ID3D11DeviceChild methods */
    d3d11_vertex_shader_GetDevice,
    d3d11_vertex_shader_GetPrivateData,
    d3d11_vertex_shader_SetPrivateData,
    d3d11_vertex_shader_SetPrivateDataInterface,
};

/* ID3D10VertexShader methods */

static inline struct d3d_vertex_shader *impl_from_ID3D10VertexShader(ID3D10VertexShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_vertex_shader, ID3D10VertexShader_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_vertex_shader_QueryInterface(ID3D10VertexShader *iface,
        REFIID riid, void **object)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_vertex_shader_QueryInterface(&shader->ID3D11VertexShader_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_vertex_shader_AddRef(ID3D10VertexShader *iface)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_vertex_shader_AddRef(&shader->ID3D11VertexShader_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_vertex_shader_Release(ID3D10VertexShader *iface)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_vertex_shader_Release(&shader->ID3D11VertexShader_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_vertex_shader_GetDevice(ID3D10VertexShader *iface, ID3D10Device **device)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_QueryInterface(shader->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_vertex_shader_GetPrivateData(ID3D10VertexShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_vertex_shader_SetPrivateData(ID3D10VertexShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_vertex_shader_SetPrivateDataInterface(ID3D10VertexShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D10VertexShaderVtbl d3d10_vertex_shader_vtbl =
{
    /* IUnknown methods */
    d3d10_vertex_shader_QueryInterface,
    d3d10_vertex_shader_AddRef,
    d3d10_vertex_shader_Release,
    /* ID3D10DeviceChild methods */
    d3d10_vertex_shader_GetDevice,
    d3d10_vertex_shader_GetPrivateData,
    d3d10_vertex_shader_SetPrivateData,
    d3d10_vertex_shader_SetPrivateDataInterface,
};

static void STDMETHODCALLTYPE d3d_vertex_shader_wined3d_object_destroyed(void *parent)
{
    struct d3d_vertex_shader *shader = parent;

    wined3d_private_store_cleanup(&shader->private_store);
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d_vertex_shader_wined3d_parent_ops =
{
    d3d_vertex_shader_wined3d_object_destroyed,
};

static HRESULT d3d_vertex_shader_init(struct d3d_vertex_shader *shader, struct d3d_device *device,
        const void *byte_code, SIZE_T byte_code_length)
{
    struct wined3d_shader_signature output_signature;
    struct wined3d_shader_signature input_signature;
    struct d3d_shader_info shader_info;
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->ID3D11VertexShader_iface.lpVtbl = &d3d11_vertex_shader_vtbl;
    shader->ID3D10VertexShader_iface.lpVtbl = &d3d10_vertex_shader_vtbl;
    shader->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&shader->private_store);

    shader_info.input_signature = &input_signature;
    shader_info.output_signature = &output_signature;
    if (FAILED(hr = shader_extract_from_dxbc(byte_code, byte_code_length, &shader_info)))
    {
        ERR("Failed to extract shader, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return hr;
    }

    desc.byte_code = shader_info.shader_code;
    desc.input_signature = &input_signature;
    desc.output_signature = &output_signature;
    desc.max_version = 4;

    hr = wined3d_shader_create_vs(device->wined3d_device, &desc, shader,
            &d3d_vertex_shader_wined3d_parent_ops, &shader->wined3d_shader);
    shader_free_signature(&input_signature);
    shader_free_signature(&output_signature);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d vertex shader, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    shader->device = &device->ID3D11Device_iface;
    ID3D11Device_AddRef(shader->device);

    return S_OK;
}

HRESULT d3d_vertex_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d_vertex_shader **shader)
{
    struct d3d_vertex_shader *object;
    HRESULT hr;

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_vertex_shader_init(object, device, byte_code, byte_code_length)))
    {
        WARN("Failed to initialize vertex shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created vertex shader %p.\n", object);
    *shader = object;

    return S_OK;
}

struct d3d_vertex_shader *unsafe_impl_from_ID3D10VertexShader(ID3D10VertexShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_vertex_shader_vtbl);

    return impl_from_ID3D10VertexShader(iface);
}

/* ID3D11GeometryShader methods */

static inline struct d3d_geometry_shader *impl_from_ID3D11GeometryShader(ID3D11GeometryShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_geometry_shader, ID3D11GeometryShader_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_geometry_shader_QueryInterface(ID3D11GeometryShader *iface,
        REFIID riid, void **object)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11GeometryShader)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11GeometryShader_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10GeometryShader)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10GeometryShader_AddRef(&shader->ID3D10GeometryShader_iface);
        *object = &shader->ID3D10GeometryShader_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_geometry_shader_AddRef(ID3D11GeometryShader *iface)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %u.\n", shader, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_geometry_shader_Release(ID3D11GeometryShader *iface)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %u.\n", shader, refcount);

    if (!refcount)
    {
        ID3D11Device *device = shader->device;

        wined3d_mutex_lock();
        wined3d_shader_decref(shader->wined3d_shader);
        wined3d_mutex_unlock();

        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_geometry_shader_GetDevice(ID3D11GeometryShader *iface,
        ID3D11Device **device)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = shader->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_geometry_shader_GetPrivateData(ID3D11GeometryShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_geometry_shader_SetPrivateData(ID3D11GeometryShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_geometry_shader_SetPrivateDataInterface(ID3D11GeometryShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D11GeometryShaderVtbl d3d11_geometry_shader_vtbl =
{
    /* IUnknown methods */
    d3d11_geometry_shader_QueryInterface,
    d3d11_geometry_shader_AddRef,
    d3d11_geometry_shader_Release,
    /* ID3D11DeviceChild methods */
    d3d11_geometry_shader_GetDevice,
    d3d11_geometry_shader_GetPrivateData,
    d3d11_geometry_shader_SetPrivateData,
    d3d11_geometry_shader_SetPrivateDataInterface,
};

/* ID3D10GeometryShader methods */

static inline struct d3d_geometry_shader *impl_from_ID3D10GeometryShader(ID3D10GeometryShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_geometry_shader, ID3D10GeometryShader_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_geometry_shader_QueryInterface(ID3D10GeometryShader *iface,
        REFIID riid, void **object)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_geometry_shader_QueryInterface(&shader->ID3D11GeometryShader_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_geometry_shader_AddRef(ID3D10GeometryShader *iface)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_geometry_shader_AddRef(&shader->ID3D11GeometryShader_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_geometry_shader_Release(ID3D10GeometryShader *iface)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_geometry_shader_Release(&shader->ID3D11GeometryShader_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_geometry_shader_GetDevice(ID3D10GeometryShader *iface, ID3D10Device **device)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_QueryInterface(shader->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_geometry_shader_GetPrivateData(ID3D10GeometryShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_geometry_shader_SetPrivateData(ID3D10GeometryShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_geometry_shader_SetPrivateDataInterface(ID3D10GeometryShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D10GeometryShaderVtbl d3d10_geometry_shader_vtbl =
{
    /* IUnknown methods */
    d3d10_geometry_shader_QueryInterface,
    d3d10_geometry_shader_AddRef,
    d3d10_geometry_shader_Release,
    /* ID3D10DeviceChild methods */
    d3d10_geometry_shader_GetDevice,
    d3d10_geometry_shader_GetPrivateData,
    d3d10_geometry_shader_SetPrivateData,
    d3d10_geometry_shader_SetPrivateDataInterface,
};

static void STDMETHODCALLTYPE d3d_geometry_shader_wined3d_object_destroyed(void *parent)
{
    struct d3d_geometry_shader *shader = parent;

    wined3d_private_store_cleanup(&shader->private_store);
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d_geometry_shader_wined3d_parent_ops =
{
    d3d_geometry_shader_wined3d_object_destroyed,
};

static HRESULT d3d_geometry_shader_init(struct d3d_geometry_shader *shader, struct d3d_device *device,
        const void *byte_code, SIZE_T byte_code_length)
{
    struct wined3d_shader_signature output_signature;
    struct wined3d_shader_signature input_signature;
    struct d3d_shader_info shader_info;
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->ID3D11GeometryShader_iface.lpVtbl = &d3d11_geometry_shader_vtbl;
    shader->ID3D10GeometryShader_iface.lpVtbl = &d3d10_geometry_shader_vtbl;
    shader->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&shader->private_store);

    shader_info.input_signature = &input_signature;
    shader_info.output_signature = &output_signature;
    if (FAILED(hr = shader_extract_from_dxbc(byte_code, byte_code_length, &shader_info)))
    {
        ERR("Failed to extract shader, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return hr;
    }

    desc.byte_code = shader_info.shader_code;
    desc.input_signature = &input_signature;
    desc.output_signature = &output_signature;
    desc.max_version = 4;

    hr = wined3d_shader_create_gs(device->wined3d_device, &desc, shader,
            &d3d_geometry_shader_wined3d_parent_ops, &shader->wined3d_shader);
    shader_free_signature(&input_signature);
    shader_free_signature(&output_signature);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d geometry shader, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    shader->device = &device->ID3D11Device_iface;
    ID3D11Device_AddRef(shader->device);

    return S_OK;
}

HRESULT d3d_geometry_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d_geometry_shader **shader)
{
    struct d3d_geometry_shader *object;
    HRESULT hr;

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_geometry_shader_init(object, device, byte_code, byte_code_length)))
    {
        WARN("Failed to initialize geometry shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created geometry shader %p.\n", object);
    *shader = object;

    return S_OK;
}

struct d3d_geometry_shader *unsafe_impl_from_ID3D10GeometryShader(ID3D10GeometryShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_geometry_shader_vtbl);

    return impl_from_ID3D10GeometryShader(iface);
}

/* ID3D11PixelShader methods */

static inline struct d3d_pixel_shader *impl_from_ID3D11PixelShader(ID3D11PixelShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_pixel_shader, ID3D11PixelShader_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_pixel_shader_QueryInterface(ID3D11PixelShader *iface,
        REFIID riid, void **object)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11PixelShader)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11PixelShader_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10PixelShader)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        IUnknown_AddRef(&shader->ID3D10PixelShader_iface);
        *object = &shader->ID3D10PixelShader_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_pixel_shader_AddRef(ID3D11PixelShader *iface)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %u.\n", shader, refcount);

    if (refcount == 1)
    {
        ID3D11Device_AddRef(shader->device);
        wined3d_mutex_lock();
        wined3d_shader_incref(shader->wined3d_shader);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_pixel_shader_Release(ID3D11PixelShader *iface)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %u.\n", shader, refcount);

    if (!refcount)
    {
        ID3D11Device *device = shader->device;

        wined3d_mutex_lock();
        wined3d_shader_decref(shader->wined3d_shader);
        wined3d_mutex_unlock();
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_pixel_shader_GetDevice(ID3D11PixelShader *iface,
        ID3D11Device **device)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = shader->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_pixel_shader_GetPrivateData(ID3D11PixelShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_pixel_shader_SetPrivateData(ID3D11PixelShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_pixel_shader_SetPrivateDataInterface(ID3D11PixelShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D11PixelShaderVtbl d3d11_pixel_shader_vtbl =
{
    /* IUnknown methods */
    d3d11_pixel_shader_QueryInterface,
    d3d11_pixel_shader_AddRef,
    d3d11_pixel_shader_Release,
    /* ID3D11DeviceChild methods */
    d3d11_pixel_shader_GetDevice,
    d3d11_pixel_shader_GetPrivateData,
    d3d11_pixel_shader_SetPrivateData,
    d3d11_pixel_shader_SetPrivateDataInterface,
};

/* ID3D10PixelShader methods */

static inline struct d3d_pixel_shader *impl_from_ID3D10PixelShader(ID3D10PixelShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_pixel_shader, ID3D10PixelShader_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_pixel_shader_QueryInterface(ID3D10PixelShader *iface,
        REFIID riid, void **object)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_pixel_shader_QueryInterface(&shader->ID3D11PixelShader_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_pixel_shader_AddRef(ID3D10PixelShader *iface)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_pixel_shader_AddRef(&shader->ID3D11PixelShader_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_pixel_shader_Release(ID3D10PixelShader *iface)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_pixel_shader_Release(&shader->ID3D11PixelShader_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_pixel_shader_GetDevice(ID3D10PixelShader *iface, ID3D10Device **device)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_QueryInterface(shader->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_pixel_shader_GetPrivateData(ID3D10PixelShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_pixel_shader_SetPrivateData(ID3D10PixelShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_pixel_shader_SetPrivateDataInterface(ID3D10PixelShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D10PixelShaderVtbl d3d10_pixel_shader_vtbl =
{
    /* IUnknown methods */
    d3d10_pixel_shader_QueryInterface,
    d3d10_pixel_shader_AddRef,
    d3d10_pixel_shader_Release,
    /* ID3D10DeviceChild methods */
    d3d10_pixel_shader_GetDevice,
    d3d10_pixel_shader_GetPrivateData,
    d3d10_pixel_shader_SetPrivateData,
    d3d10_pixel_shader_SetPrivateDataInterface,
};

static void STDMETHODCALLTYPE d3d_pixel_shader_wined3d_object_destroyed(void *parent)
{
    struct d3d_pixel_shader *shader = parent;

    wined3d_private_store_cleanup(&shader->private_store);
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d_pixel_shader_wined3d_parent_ops =
{
    d3d_pixel_shader_wined3d_object_destroyed,
};

static HRESULT d3d_pixel_shader_init(struct d3d_pixel_shader *shader, struct d3d_device *device,
        const void *byte_code, SIZE_T byte_code_length)
{
    struct wined3d_shader_signature output_signature;
    struct wined3d_shader_signature input_signature;
    struct d3d_shader_info shader_info;
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->ID3D11PixelShader_iface.lpVtbl = &d3d11_pixel_shader_vtbl;
    shader->ID3D10PixelShader_iface.lpVtbl = &d3d10_pixel_shader_vtbl;
    shader->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&shader->private_store);

    shader_info.input_signature = &input_signature;
    shader_info.output_signature = &output_signature;
    if (FAILED(hr = shader_extract_from_dxbc(byte_code, byte_code_length, &shader_info)))
    {
        ERR("Failed to extract shader, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return hr;
    }

    desc.byte_code = shader_info.shader_code;
    desc.input_signature = &input_signature;
    desc.output_signature = &output_signature;
    desc.max_version = 4;

    hr = wined3d_shader_create_ps(device->wined3d_device, &desc, shader,
            &d3d_pixel_shader_wined3d_parent_ops, &shader->wined3d_shader);
    shader_free_signature(&input_signature);
    shader_free_signature(&output_signature);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d pixel shader, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    shader->device = &device->ID3D11Device_iface;
    ID3D11Device_AddRef(shader->device);

    return S_OK;
}

HRESULT d3d_pixel_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d_pixel_shader **shader)
{
    struct d3d_pixel_shader *object;
    HRESULT hr;

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_pixel_shader_init(object, device, byte_code, byte_code_length)))
    {
        WARN("Failed to initialize pixel shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created pixel shader %p.\n", object);
    *shader = object;

    return S_OK;
}

struct d3d_pixel_shader *unsafe_impl_from_ID3D10PixelShader(ID3D10PixelShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_pixel_shader_vtbl);

    return impl_from_ID3D10PixelShader(iface);
}
