/*
 * Copyright 2015 Michael Müller
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

#include "shobjidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

struct new_menu
{
    IShellExtInit IShellExtInit_iface;
    LONG refcount;
};

static struct new_menu *impl_from_IShellExtInit(IShellExtInit *iface)
{
    return CONTAINING_RECORD(iface, struct new_menu, IShellExtInit_iface);
}

static HRESULT WINAPI ext_init_QueryInterface(IShellExtInit *iface, REFIID iid, void **out)
{
    struct new_menu *menu = impl_from_IShellExtInit(iface);

    TRACE("menu %p, iid %s, out %p.\n", menu, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IShellExtInit))
        *out = &menu->IShellExtInit_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI ext_init_AddRef(IShellExtInit *iface)
{
    struct new_menu *menu = impl_from_IShellExtInit(iface);
    ULONG refcount = InterlockedIncrement(&menu->refcount);

    TRACE("%p increasing refcount to %lu.\n", menu, refcount);

    return refcount;
}

static ULONG WINAPI ext_init_Release(IShellExtInit *iface)
{
    struct new_menu *menu = impl_from_IShellExtInit(iface);
    ULONG refcount = InterlockedDecrement(&menu->refcount);

    TRACE("%p decreasing refcount to %lu.\n", menu, refcount);

    if (!refcount)
        free(menu);

    return refcount;
}

static HRESULT WINAPI ext_init_Initialize(IShellExtInit *iface, LPCITEMIDLIST pidl, IDataObject *obj, HKEY key)
{
    struct new_menu *menu = impl_from_IShellExtInit(iface);

    TRACE("menu %p, pidl %p, obj %p, key %p.\n", menu, pidl, obj, key);

    return S_OK;
}

static const IShellExtInitVtbl ext_init_vtbl =
{
    ext_init_QueryInterface,
    ext_init_AddRef,
    ext_init_Release,
    ext_init_Initialize,
};

HRESULT WINAPI new_menu_create(IUnknown *outer, REFIID iid, void **out)
{
    struct new_menu *menu;
    HRESULT hr;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(menu = malloc(sizeof(*menu))))
        return E_OUTOFMEMORY;

    menu->IShellExtInit_iface.lpVtbl = &ext_init_vtbl;
    menu->refcount = 1;

    TRACE("Created New menu %p.\n", menu);

    hr = IShellExtInit_QueryInterface(&menu->IShellExtInit_iface, iid, out);
    IShellExtInit_Release(&menu->IShellExtInit_iface);
    return hr;
}
