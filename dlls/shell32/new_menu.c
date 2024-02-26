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

#include "shell32_main.h"
#include "shresdef.h"
#include "pidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

struct new_menu
{
    IShellExtInit IShellExtInit_iface;
    IContextMenu3 IContextMenu3_iface;
    IObjectWithSite IObjectWithSite_iface;
    LONG refcount;

    ITEMIDLIST *pidl;
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
    else if (IsEqualGUID(iid, &IID_IContextMenu)
            || IsEqualGUID(iid, &IID_IContextMenu2)
            || IsEqualGUID(iid, &IID_IContextMenu3))
        *out = &menu->IContextMenu3_iface;
    else if (IsEqualGUID(iid, &IID_IObjectWithSite))
        *out = &menu->IObjectWithSite_iface;
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
    {
        ILFree(menu->pidl);
        free(menu);
    }

    return refcount;
}

static HRESULT WINAPI ext_init_Initialize(IShellExtInit *iface, LPCITEMIDLIST pidl, IDataObject *obj, HKEY key)
{
    struct new_menu *menu = impl_from_IShellExtInit(iface);

    TRACE("menu %p, pidl %p, obj %p, key %p.\n", menu, pidl, obj, key);

    if (!pidl)
        return E_INVALIDARG;

    menu->pidl = ILClone(pidl);
    return S_OK;
}

static const IShellExtInitVtbl ext_init_vtbl =
{
    ext_init_QueryInterface,
    ext_init_AddRef,
    ext_init_Release,
    ext_init_Initialize,
};

static struct new_menu *impl_from_IContextMenu3(IContextMenu3 *iface)
{
    return CONTAINING_RECORD(iface, struct new_menu, IContextMenu3_iface);
}

static HRESULT WINAPI context_menu_QueryInterface(IContextMenu3 *iface, REFIID iid, void **out)
{
    struct new_menu *menu = impl_from_IContextMenu3(iface);

    return IShellExtInit_QueryInterface(&menu->IShellExtInit_iface, iid, out);
}

static ULONG WINAPI context_menu_AddRef(IContextMenu3 *iface)
{
    struct new_menu *menu = impl_from_IContextMenu3(iface);

    return IShellExtInit_AddRef(&menu->IShellExtInit_iface);
}

static ULONG WINAPI context_menu_Release(IContextMenu3 *iface)
{
    struct new_menu *menu = impl_from_IContextMenu3(iface);

    return IShellExtInit_Release(&menu->IShellExtInit_iface);
}

static HRESULT WINAPI context_menu_QueryContextMenu(IContextMenu3 *iface,
        HMENU hmenu, UINT index, UINT min_id, UINT max_id, UINT flags)
{
    struct new_menu *menu = impl_from_IContextMenu3(iface);
    MENUITEMINFOW info;
    WCHAR *new_string;
    HMENU submenu;

    TRACE("menu %p, hmenu %p, index %u, min_id %u, max_id %u, flags %#x.\n",
            menu, hmenu, index, min_id, max_id, flags);

    if (!_ILIsFolder(ILFindLastID(menu->pidl)) && !_ILIsDrive(ILFindLastID(menu->pidl)))
    {
        TRACE("Not returning a New item for this pidl type.\n");
        return S_OK;
    }

    submenu = CreatePopupMenu();
    new_string = shell_get_resource_string(IDS_NEW_MENU);

    info.cbSize = sizeof(info);
    info.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STATE | MIIM_STRING | MIIM_SUBMENU;
    info.dwTypeData = new_string;
    info.fState = MFS_ENABLED;
    info.wID = min_id;
    info.fType = MFT_STRING;
    info.hSubMenu = submenu;
    InsertMenuItemW(hmenu, 0, MF_BYPOSITION, &info);

    free(new_string);

    /* Native doesn't return the highest ID; it instead just returns 0x40. */
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0x40);
}

static HRESULT WINAPI context_menu_InvokeCommand(IContextMenu3 *iface, CMINVOKECOMMANDINFO *info)
{
    FIXME("iface %p, info %p, stub!\n", iface, info);

    return E_NOTIMPL;
}

static HRESULT WINAPI context_menu_GetCommandString(IContextMenu3 *iface,
        UINT_PTR id, UINT type, UINT *reserved, char *string, UINT size)
{
    FIXME("iface %p, id %Iu, type %#x, reserved %p, string %p, size %u, stub!\n",
            iface, id, type, reserved, string, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI context_menu_HandleMenuMsg(IContextMenu3 *iface, UINT msg, WPARAM wparam, LPARAM lparam)
{
    FIXME("iface %p, msg %#x, wparam %#Ix, lparam %#Ix, stub!\n", iface, msg, wparam, lparam);

    return E_NOTIMPL;
}

static HRESULT WINAPI context_menu_HandleMenuMsg2(IContextMenu3 *iface,
        UINT msg, WPARAM wparam, LPARAM lparam, LRESULT *result)
{
    FIXME("iface %p, msg %#x, wparam %#Ix, lparam %#Ix, result %p, stub!\n",
            iface, msg, wparam, lparam, result);

    return E_NOTIMPL;
}

static const IContextMenu3Vtbl context_menu_vtbl =
{
    context_menu_QueryInterface,
    context_menu_AddRef,
    context_menu_Release,
    context_menu_QueryContextMenu,
    context_menu_InvokeCommand,
    context_menu_GetCommandString,
    context_menu_HandleMenuMsg,
    context_menu_HandleMenuMsg2,
};

static struct new_menu *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, struct new_menu, IObjectWithSite_iface);
}

static HRESULT WINAPI object_with_site_QueryInterface(IObjectWithSite *iface, REFIID iid, void **out)
{
    struct new_menu *menu = impl_from_IObjectWithSite(iface);

    return IShellExtInit_QueryInterface(&menu->IShellExtInit_iface, iid, out);
}

static ULONG WINAPI object_with_site_AddRef(IObjectWithSite *iface)
{
    struct new_menu *menu = impl_from_IObjectWithSite(iface);

    return IShellExtInit_AddRef(&menu->IShellExtInit_iface);
}

static ULONG WINAPI object_with_site_Release(IObjectWithSite *iface)
{
    struct new_menu *menu = impl_from_IObjectWithSite(iface);

    return IShellExtInit_Release(&menu->IShellExtInit_iface);
}

static HRESULT WINAPI object_with_site_SetSite(IObjectWithSite *iface, IUnknown *site)
{
    FIXME("iface %p, site %p, stub!\n", iface, site);

    return E_NOTIMPL;
}

static HRESULT WINAPI object_with_site_GetSite(IObjectWithSite *iface, REFIID iid, void **out)
{
    FIXME("iface %p, iid %s, out %p, stub!\n", iface, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static const IObjectWithSiteVtbl object_with_site_vtbl =
{
    object_with_site_QueryInterface,
    object_with_site_AddRef,
    object_with_site_Release,
    object_with_site_SetSite,
    object_with_site_GetSite,
};

HRESULT WINAPI new_menu_create(IUnknown *outer, REFIID iid, void **out)
{
    struct new_menu *menu;
    HRESULT hr;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(menu = calloc(1, sizeof(*menu))))
        return E_OUTOFMEMORY;

    menu->IShellExtInit_iface.lpVtbl = &ext_init_vtbl;
    menu->IContextMenu3_iface.lpVtbl = &context_menu_vtbl;
    menu->IObjectWithSite_iface.lpVtbl = &object_with_site_vtbl;
    menu->refcount = 1;

    TRACE("Created New menu %p.\n", menu);

    hr = IShellExtInit_QueryInterface(&menu->IShellExtInit_iface, iid, out);
    IShellExtInit_Release(&menu->IShellExtInit_iface);
    return hr;
}
