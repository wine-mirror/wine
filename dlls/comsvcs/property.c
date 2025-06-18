/*
 * Copyright 2021 Jactry Zeng for CodeWeavers
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

#include "comsvcs_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(comsvcs);

struct group_manager
{
    ISharedPropertyGroupManager ISharedPropertyGroupManager_iface;
    LONG refcount;
};

static struct group_manager *group_manager = NULL;

static inline struct group_manager *impl_from_ISharedPropertyGroupManager(ISharedPropertyGroupManager *iface)
{
    return CONTAINING_RECORD(iface, struct group_manager, ISharedPropertyGroupManager_iface);
}

static HRESULT WINAPI group_manager_QueryInterface(ISharedPropertyGroupManager *iface, REFIID riid, void **out)
{
    struct group_manager *manager = impl_from_ISharedPropertyGroupManager(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDispatch)
            || IsEqualGUID(riid, &IID_ISharedPropertyGroupManager))
    {
        *out = &manager->ISharedPropertyGroupManager_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI group_manager_AddRef(ISharedPropertyGroupManager *iface)
{
    struct group_manager *manager = impl_from_ISharedPropertyGroupManager(iface);
    ULONG refcount = InterlockedIncrement(&manager->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI group_manager_Release(ISharedPropertyGroupManager *iface)
{
    struct group_manager *manager = impl_from_ISharedPropertyGroupManager(iface);
    ULONG refcount = InterlockedDecrement(&manager->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI group_manager_GetTypeInfoCount(ISharedPropertyGroupManager *iface, UINT *info)
{
    FIXME("iface %p, info %p: stub.\n", iface, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI group_manager_GetTypeInfo(ISharedPropertyGroupManager *iface, UINT index, LCID lcid,
        ITypeInfo **info)
{
    FIXME("iface %p, index %u, lcid %lu, info %p: stub.\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI group_manager_GetIDsOfNames(ISharedPropertyGroupManager *iface, REFIID riid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("iface %p, riid %s, names %p, count %u, lcid %lu, dispid %p: stub.\n",
            iface, debugstr_guid(riid), names, count, lcid, dispid);

    return E_NOTIMPL;
}

static HRESULT WINAPI group_manager_Invoke(ISharedPropertyGroupManager *iface, DISPID member, REFIID riid,
        LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *except, UINT *argerr)
{
    FIXME("iface %p, member %lu, riid %s, lcid %lu, flags %x, params %p, result %p, except %p, argerr %p: stub.\n",
            iface, member, debugstr_guid(riid), lcid, flags, params, result, except, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI group_manager_CreatePropertyGroup(ISharedPropertyGroupManager *iface, BSTR name,
        LONG *isolation, LONG *release, VARIANT_BOOL *exists, ISharedPropertyGroup **group)
{
    FIXME("iface %p, name %s, isolation %p, release %p, exists %p, group %p: stub.\n",
            iface, debugstr_w(name), isolation, release, exists, group);
    return E_NOTIMPL;
}

static HRESULT WINAPI group_manager_get_Group(ISharedPropertyGroupManager *iface, BSTR name,
        ISharedPropertyGroup **group)
{
    FIXME("iface %p, name %s, group %p: stub.\n", iface, debugstr_w(name), group);
    return E_NOTIMPL;
}

static HRESULT WINAPI group_manager_get__NewEnum(ISharedPropertyGroupManager *iface, IUnknown **retval)
{
    FIXME("iface %p, retval %p: stub.\n", iface, retval);
    return E_NOTIMPL;
}

static const ISharedPropertyGroupManagerVtbl group_manager_vtbl =
{
    group_manager_QueryInterface,
    group_manager_AddRef,
    group_manager_Release,
    group_manager_GetTypeInfoCount,
    group_manager_GetTypeInfo,
    group_manager_GetIDsOfNames,
    group_manager_Invoke,
    group_manager_CreatePropertyGroup,
    group_manager_get_Group,
    group_manager_get__NewEnum
};

HRESULT WINAPI group_manager_create(IClassFactory *iface, IUnknown *outer, REFIID riid, void **out)
{
    struct group_manager *manager;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!group_manager)
    {
        manager = malloc(sizeof(*manager));
        if (!manager)
        {
            *out = NULL;
            return E_OUTOFMEMORY;
        }
        manager->ISharedPropertyGroupManager_iface.lpVtbl = &group_manager_vtbl;
        manager->refcount = 1;

        if (InterlockedCompareExchangePointer((void **)&group_manager, manager, NULL))
        {
            free(manager);
        }
    }
    return ISharedPropertyGroupManager_QueryInterface(&group_manager->ISharedPropertyGroupManager_iface, riid, out);
}
