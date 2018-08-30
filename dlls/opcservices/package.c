/*
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

#include "opc_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msopc);

struct opc_package
{
    IOpcPackage IOpcPackage_iface;
    LONG refcount;
};

static inline struct opc_package *impl_from_IOpcPackage(IOpcPackage *iface)
{
    return CONTAINING_RECORD(iface, struct opc_package, IOpcPackage_iface);
}

static HRESULT WINAPI opc_package_QueryInterface(IOpcPackage *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IOpcPackage) ||
            IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IOpcPackage_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_package_AddRef(IOpcPackage *iface)
{
    struct opc_package *package = impl_from_IOpcPackage(iface);
    ULONG refcount = InterlockedIncrement(&package->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_package_Release(IOpcPackage *iface)
{
    struct opc_package *package = impl_from_IOpcPackage(iface);
    ULONG refcount = InterlockedDecrement(&package->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        heap_free(package);

    return refcount;
}

static HRESULT WINAPI opc_package_GetPartSet(IOpcPackage *iface, IOpcPartSet **part_set)
{
    FIXME("iface %p, part_set %p stub!\n", iface, part_set);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_package_GetRelationshipSet(IOpcPackage *iface, IOpcRelationshipSet **relationship_set)
{
    FIXME("iface %p, relationship_set %p stub!\n", iface, relationship_set);

    return E_NOTIMPL;
}

static const IOpcPackageVtbl opc_package_vtbl =
{
    opc_package_QueryInterface,
    opc_package_AddRef,
    opc_package_Release,
    opc_package_GetPartSet,
    opc_package_GetRelationshipSet,
};

HRESULT opc_package_create(IOpcPackage **out)
{
    struct opc_package *package;

    if (!(package = heap_alloc(sizeof(*package))))
        return E_OUTOFMEMORY;

    package->IOpcPackage_iface.lpVtbl = &opc_package_vtbl;
    package->refcount = 1;

    *out = &package->IOpcPackage_iface;
    TRACE("Created package %p.\n", *out);
    return S_OK;
}
