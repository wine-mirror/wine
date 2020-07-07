/*
 * Graph builder
 *
 * Copyright 2020 Gijs Vermeulen
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

#include "qdvd_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(qdvd);

struct graph_builder
{
    IDvdGraphBuilder IDvdGraphBuilder_iface;
    LONG refcount;
};

static struct graph_builder *impl_from_IDvdGraphBuilder(IDvdGraphBuilder *iface)
{
    return CONTAINING_RECORD(iface, struct graph_builder, IDvdGraphBuilder_iface);
}

static ULONG WINAPI graph_builder_AddRef(IDvdGraphBuilder *iface)
{
    struct graph_builder *builder = impl_from_IDvdGraphBuilder(iface);
    ULONG refcount = InterlockedIncrement(&builder->refcount);
    TRACE("%p increasing refcount to %u.\n", builder, refcount);
    return refcount;
}

static ULONG WINAPI graph_builder_Release(IDvdGraphBuilder *iface)
{
    struct graph_builder *builder = impl_from_IDvdGraphBuilder(iface);
    ULONG refcount = InterlockedDecrement(&builder->refcount);
    TRACE("%p decreasing refcount to %u.\n", builder, refcount);
    if (!refcount)
        free(builder);
    return refcount;
}

static HRESULT WINAPI graph_builder_QueryInterface(IDvdGraphBuilder *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IDvdGraphBuilder) || IsEqualGUID(iid, &IID_IUnknown))
        *out = iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI graph_builder_GetFiltergraph(IDvdGraphBuilder *iface, IGraphBuilder **graph)
{
    FIXME("iface %p, graph %p, stub!\n", iface, graph);
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_builder_GetDvdInterface(IDvdGraphBuilder *iface, REFIID iid, void **out)
{
    FIXME("iface %p, iid %s, out %p, stub!\n", iface, debugstr_guid(iid), out);
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_builder_RenderDvdVideoVolume(IDvdGraphBuilder *iface, const WCHAR *path, DWORD flags, AM_DVD_RENDERSTATUS *status)
{
    FIXME("iface %p, path %s, flags %#x, status %p, stub!\n", iface, debugstr_w(path), flags, status);
    return E_NOTIMPL;
}

static const struct IDvdGraphBuilderVtbl graph_builder_vtbl =
{
    graph_builder_QueryInterface,
    graph_builder_AddRef,
    graph_builder_Release,
    graph_builder_GetFiltergraph,
    graph_builder_GetDvdInterface,
    graph_builder_RenderDvdVideoVolume,
};

HRESULT graph_builder_create(IUnknown **out)
{
    struct graph_builder *builder;

    if (!(builder = calloc(1, sizeof(*builder))))
        return E_OUTOFMEMORY;

    builder->IDvdGraphBuilder_iface.lpVtbl = &graph_builder_vtbl;
    builder->refcount = 1;

    TRACE("Created DVD graph builder %p.\n", builder);
    *out = (IUnknown *)&builder->IDvdGraphBuilder_iface;
    return S_OK;
}
