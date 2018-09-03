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

    IOpcPartSet *part_set;
};

struct opc_part
{
    IOpcPart IOpcPart_iface;
    LONG refcount;
};

struct opc_part_set
{
    IOpcPartSet IOpcPartSet_iface;
    LONG refcount;
};

static inline struct opc_package *impl_from_IOpcPackage(IOpcPackage *iface)
{
    return CONTAINING_RECORD(iface, struct opc_package, IOpcPackage_iface);
}

static inline struct opc_part_set *impl_from_IOpcPartSet(IOpcPartSet *iface)
{
    return CONTAINING_RECORD(iface, struct opc_part_set, IOpcPartSet_iface);
}

static inline struct opc_part *impl_from_IOpcPart(IOpcPart *iface)
{
    return CONTAINING_RECORD(iface, struct opc_part, IOpcPart_iface);
}

static HRESULT WINAPI opc_part_QueryInterface(IOpcPart *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IOpcPart) ||
            IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IOpcPart_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_part_AddRef(IOpcPart *iface)
{
    struct opc_part *part = impl_from_IOpcPart(iface);
    ULONG refcount = InterlockedIncrement(&part->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_part_Release(IOpcPart *iface)
{
    struct opc_part *part = impl_from_IOpcPart(iface);
    ULONG refcount = InterlockedDecrement(&part->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        heap_free(part);

    return refcount;
}

static HRESULT WINAPI opc_part_GetRelationshipSet(IOpcPart *iface, IOpcRelationshipSet **relationship_set)
{
    FIXME("iface %p, relationship_set %p stub!\n", iface, relationship_set);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_part_GetContentStream(IOpcPart *iface, IStream **stream)
{
    FIXME("iface %p, stream %p stub!\n", iface, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_part_GetName(IOpcPart *iface, IOpcPartUri **name)
{
    FIXME("iface %p, name %p stub!\n", iface, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_part_GetContentType(IOpcPart *iface, LPWSTR *type)
{
    FIXME("iface %p, type %p stub!\n", iface, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_part_GetCompressionOptions(IOpcPart *iface, OPC_COMPRESSION_OPTIONS *options)
{
    FIXME("iface %p, options %p stub!\n", iface, options);

    return E_NOTIMPL;
}

static const IOpcPartVtbl opc_part_vtbl =
{
    opc_part_QueryInterface,
    opc_part_AddRef,
    opc_part_Release,
    opc_part_GetRelationshipSet,
    opc_part_GetContentStream,
    opc_part_GetName,
    opc_part_GetContentType,
    opc_part_GetCompressionOptions,
};

static HRESULT opc_part_create(IOpcPart **out)
{
    struct opc_part *part;

    if (!(part = heap_alloc_zero(sizeof(*part))))
        return E_OUTOFMEMORY;

    part->IOpcPart_iface.lpVtbl = &opc_part_vtbl;
    part->refcount = 1;

    *out = &part->IOpcPart_iface;
    TRACE("Created part %p.\n", *out);
    return S_OK;
}

static HRESULT WINAPI opc_part_set_QueryInterface(IOpcPartSet *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IOpcPartSet) ||
            IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IOpcPartSet_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_part_set_AddRef(IOpcPartSet *iface)
{
    struct opc_part_set *part_set = impl_from_IOpcPartSet(iface);
    ULONG refcount = InterlockedIncrement(&part_set->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_part_set_Release(IOpcPartSet *iface)
{
    struct opc_part_set *part_set = impl_from_IOpcPartSet(iface);
    ULONG refcount = InterlockedDecrement(&part_set->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        heap_free(part_set);

    return refcount;
}

static HRESULT WINAPI opc_part_set_GetPart(IOpcPartSet *iface, IOpcPartUri *name, IOpcPart **part)
{
    FIXME("iface %p, name %p, part %p stub!\n", iface, name, part);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_part_set_CreatePart(IOpcPartSet *iface, IOpcPartUri *name, LPCWSTR content_type,
        OPC_COMPRESSION_OPTIONS compression_options, IOpcPart **part)
{
    FIXME("iface %p, name %p, content_type %s, compression_options %#x, part %p stub!\n", iface, name,
            debugstr_w(content_type), compression_options, part);

    return opc_part_create(part);
}

static HRESULT WINAPI opc_part_set_DeletePart(IOpcPartSet *iface, IOpcPartUri *name)
{
    FIXME("iface %p, name %p stub!\n", iface, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_part_set_PartExists(IOpcPartSet *iface, IOpcPartUri *name, BOOL *exists)
{
    FIXME("iface %p, name %p, exists %p stub!\n", iface, name, exists);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_part_set_GtEnumerator(IOpcPartSet *iface, IOpcPartEnumerator **enumerator)
{
    FIXME("iface %p, enumerator %p stub!\n", iface, enumerator);

    return E_NOTIMPL;
}

static const IOpcPartSetVtbl opc_part_set_vtbl =
{
    opc_part_set_QueryInterface,
    opc_part_set_AddRef,
    opc_part_set_Release,
    opc_part_set_GetPart,
    opc_part_set_CreatePart,
    opc_part_set_DeletePart,
    opc_part_set_PartExists,
    opc_part_set_GtEnumerator,
};

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
    {
        if (package->part_set)
            IOpcPartSet_Release(package->part_set);
        heap_free(package);
    }

    return refcount;
}

static HRESULT WINAPI opc_package_GetPartSet(IOpcPackage *iface, IOpcPartSet **part_set)
{
    struct opc_package *package = impl_from_IOpcPackage(iface);

    TRACE("iface %p, part_set %p.\n", iface, part_set);

    if (!package->part_set)
    {
        struct opc_part_set *part_set = heap_alloc_zero(sizeof(*part_set));
        if (!part_set)
            return E_OUTOFMEMORY;

        part_set->IOpcPartSet_iface.lpVtbl = &opc_part_set_vtbl;
        part_set->refcount = 1;

        package->part_set = &part_set->IOpcPartSet_iface;
    }

    *part_set = package->part_set;
    IOpcPartSet_AddRef(*part_set);

    return S_OK;
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

    if (!(package = heap_alloc_zero(sizeof(*package))))
        return E_OUTOFMEMORY;

    package->IOpcPackage_iface.lpVtbl = &opc_package_vtbl;
    package->refcount = 1;

    *out = &package->IOpcPackage_iface;
    TRACE("Created package %p.\n", *out);
    return S_OK;
}
