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

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct graph_builder
{
    IUnknown IUnknown_inner;
    IDvdGraphBuilder IDvdGraphBuilder_iface;

    IUnknown *outer_unk;
    LONG refcount;
};

static struct graph_builder *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct graph_builder, IUnknown_inner);
}

static ULONG WINAPI inner_AddRef(IUnknown *iface)
{
    struct graph_builder *builder = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&builder->refcount);
    TRACE("%p increasing refcount to %lu.\n", builder, refcount);
    return refcount;
}

static ULONG WINAPI inner_Release(IUnknown *iface)
{
    struct graph_builder *builder = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&builder->refcount);
    TRACE("%p decreasing refcount to %lu.\n", builder, refcount);
    if (!refcount)
        free(builder);
    return refcount;
}

static HRESULT WINAPI inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct graph_builder *builder = impl_from_IUnknown(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = &builder->IUnknown_inner;
    else if (IsEqualGUID(iid, &IID_IDvdGraphBuilder))
        *out = &builder->IDvdGraphBuilder_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const IUnknownVtbl inner_vtbl =
{
    inner_QueryInterface,
    inner_AddRef,
    inner_Release,
};

static struct graph_builder *impl_from_IDvdGraphBuilder(IDvdGraphBuilder *iface)
{
    return CONTAINING_RECORD(iface, struct graph_builder, IDvdGraphBuilder_iface);
}

static ULONG WINAPI graph_builder_AddRef(IDvdGraphBuilder *iface)
{
    struct graph_builder *builder = impl_from_IDvdGraphBuilder(iface);
    return IUnknown_AddRef(builder->outer_unk);
}

static ULONG WINAPI graph_builder_Release(IDvdGraphBuilder *iface)
{
    struct graph_builder *builder = impl_from_IDvdGraphBuilder(iface);
    return IUnknown_Release(builder->outer_unk);
}

static HRESULT WINAPI graph_builder_QueryInterface(IDvdGraphBuilder *iface, REFIID iid, void **out)
{
    struct graph_builder *builder = impl_from_IDvdGraphBuilder(iface);
    return IUnknown_QueryInterface(builder->outer_unk, iid, out);
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
    FIXME("iface %p, path %s, flags %#lx, status %p, stub!\n", iface, debugstr_w(path), flags, status);
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

HRESULT graph_builder_create(IUnknown *outer, IUnknown **out)
{
    struct graph_builder *builder;

    if (!(builder = calloc(1, sizeof(*builder))))
        return E_OUTOFMEMORY;

    builder->IDvdGraphBuilder_iface.lpVtbl = &graph_builder_vtbl;
    builder->IUnknown_inner.lpVtbl = &inner_vtbl;
    builder->refcount = 1;
    builder->outer_unk = outer ? outer : &builder->IUnknown_inner;

    TRACE("Created DVD graph builder %p.\n", builder);
    *out = &builder->IUnknown_inner;
    return S_OK;
}
