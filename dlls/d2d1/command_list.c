/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include "d2d1_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

static inline struct d2d_command_list *impl_from_ID2D1CommandList(ID2D1CommandList *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_command_list, ID2D1CommandList_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_command_list_QueryInterface(ID2D1CommandList *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1CommandList)
            || IsEqualGUID(iid, &IID_ID2D1Image)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1CommandList_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_command_list_AddRef(ID2D1CommandList *iface)
{
    struct d2d_command_list *command_list = impl_from_ID2D1CommandList(iface);
    ULONG refcount = InterlockedIncrement(&command_list->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_command_list_Release(ID2D1CommandList *iface)
{
    struct d2d_command_list *command_list = impl_from_ID2D1CommandList(iface);
    ULONG refcount = InterlockedDecrement(&command_list->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID2D1Factory_Release(command_list->factory);
        free(command_list);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_command_list_GetFactory(ID2D1CommandList *iface, ID2D1Factory **factory)
{
    struct d2d_command_list *command_list = impl_from_ID2D1CommandList(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = command_list->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_command_list_Stream(ID2D1CommandList *iface, ID2D1CommandSink *sink)
{
    FIXME("iface %p, sink %p stub.\n", iface, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_command_list_Close(ID2D1CommandList *iface)
{
    FIXME("iface %p stub.\n", iface);

    return E_NOTIMPL;
}

static const ID2D1CommandListVtbl d2d_command_list_vtbl =
{
    d2d_command_list_QueryInterface,
    d2d_command_list_AddRef,
    d2d_command_list_Release,
    d2d_command_list_GetFactory,
    d2d_command_list_Stream,
    d2d_command_list_Close,
};

HRESULT d2d_command_list_create(ID2D1Factory *factory, struct d2d_command_list **command_list)
{
    if (!(*command_list = calloc(1, sizeof(**command_list))))
        return E_OUTOFMEMORY;

    (*command_list)->ID2D1CommandList_iface.lpVtbl = &d2d_command_list_vtbl;
    (*command_list)->refcount = 1;
    ID2D1Factory_AddRef((*command_list)->factory = factory);

    TRACE("Created command list %p.\n", *command_list);

    return S_OK;
}

struct d2d_command_list *unsafe_impl_from_ID2D1CommandList(ID2D1CommandList *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (ID2D1CommandListVtbl *)&d2d_command_list_vtbl);
    return CONTAINING_RECORD(iface, struct d2d_command_list, ID2D1CommandList_iface);
}
