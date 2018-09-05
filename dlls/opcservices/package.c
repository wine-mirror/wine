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
#include "ntsecapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "opc_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msopc);

struct opc_package
{
    IOpcPackage IOpcPackage_iface;
    LONG refcount;

    IOpcPartSet *part_set;
    IOpcRelationshipSet *relationship_set;
};

struct opc_part
{
    IOpcPart IOpcPart_iface;
    LONG refcount;

    IOpcPartUri *name;
    WCHAR *content_type;
    DWORD compression_options;
    IOpcRelationshipSet *relationship_set;
};

struct opc_part_set
{
    IOpcPartSet IOpcPartSet_iface;
    LONG refcount;

    struct opc_part **parts;
    size_t size;
    size_t count;
};

struct opc_relationship
{
    IOpcRelationship IOpcRelationship_iface;
    LONG refcount;

    WCHAR *id;
    WCHAR *type;
    IUri *target;
    OPC_URI_TARGET_MODE target_mode;
};

struct opc_relationship_set
{
    IOpcRelationshipSet IOpcRelationshipSet_iface;
    LONG refcount;

    struct opc_relationship **relationships;
    size_t size;
    size_t count;
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

static inline struct opc_relationship_set *impl_from_IOpcRelationshipSet(IOpcRelationshipSet *iface)
{
    return CONTAINING_RECORD(iface, struct opc_relationship_set, IOpcRelationshipSet_iface);
}

static inline struct opc_relationship *impl_from_IOpcRelationship(IOpcRelationship *iface)
{
    return CONTAINING_RECORD(iface, struct opc_relationship, IOpcRelationship_iface);
}

static HRESULT opc_relationship_set_create(IOpcRelationshipSet **relationship_set);

static WCHAR *opc_strdupW(const WCHAR *str)
{
    WCHAR *ret = NULL;

    if (str)
    {
        size_t size;

        size = (strlenW(str) + 1) * sizeof(WCHAR);
        ret = CoTaskMemAlloc(size);
        if (ret)
            memcpy(ret, str, size);
    }

    return ret;
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
    {
        if (part->relationship_set)
            IOpcRelationshipSet_Release(part->relationship_set);
        IOpcPartUri_Release(part->name);
        CoTaskMemFree(part->content_type);
        heap_free(part);
    }

    return refcount;
}

static HRESULT WINAPI opc_part_GetRelationshipSet(IOpcPart *iface, IOpcRelationshipSet **relationship_set)
{
    struct opc_part *part = impl_from_IOpcPart(iface);
    HRESULT hr;

    TRACE("iface %p, relationship_set %p.\n", iface, relationship_set);

    if (!part->relationship_set && FAILED(hr = opc_relationship_set_create(&part->relationship_set)))
        return hr;

    *relationship_set = part->relationship_set;
    IOpcRelationshipSet_AddRef(*relationship_set);

    return S_OK;
}

static HRESULT WINAPI opc_part_GetContentStream(IOpcPart *iface, IStream **stream)
{
    FIXME("iface %p, stream %p stub!\n", iface, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_part_GetName(IOpcPart *iface, IOpcPartUri **name)
{
    struct opc_part *part = impl_from_IOpcPart(iface);

    TRACE("iface %p, name %p.\n", iface, name);

    *name = part->name;
    IOpcPartUri_AddRef(*name);

    return S_OK;
}

static HRESULT WINAPI opc_part_GetContentType(IOpcPart *iface, LPWSTR *type)
{
    struct opc_part *part = impl_from_IOpcPart(iface);

    TRACE("iface %p, type %p.\n", iface, type);

    *type = opc_strdupW(part->content_type);
    return *type ? S_OK : E_OUTOFMEMORY;
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

static HRESULT opc_part_create(struct opc_part_set *set, IOpcPartUri *name, const WCHAR *content_type,
        DWORD compression_options, IOpcPart **out)
{
    struct opc_part *part;

    if (!name)
        return E_POINTER;

    if (!opc_array_reserve((void **)&set->parts, &set->size, set->count + 1, sizeof(*set->parts)))
        return E_OUTOFMEMORY;

    if (!(part = heap_alloc_zero(sizeof(*part))))
        return E_OUTOFMEMORY;

    part->IOpcPart_iface.lpVtbl = &opc_part_vtbl;
    part->refcount = 1;
    part->name = name;
    IOpcPartUri_AddRef(name);
    part->compression_options = compression_options;
    if (!(part->content_type = opc_strdupW(content_type)))
    {
        IOpcPart_Release(&part->IOpcPart_iface);
        return E_OUTOFMEMORY;
    }

    set->parts[set->count++] = part;
    IOpcPart_AddRef(&part->IOpcPart_iface);

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
    {
        size_t i;

        for (i = 0; i < part_set->count; ++i)
            IOpcPart_Release(&part_set->parts[i]->IOpcPart_iface);
        heap_free(part_set->parts);
        heap_free(part_set);
    }

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
    struct opc_part_set *part_set = impl_from_IOpcPartSet(iface);

    TRACE("iface %p, name %p, content_type %s, compression_options %#x, part %p.\n", iface, name,
            debugstr_w(content_type), compression_options, part);

    return opc_part_create(part_set, name, content_type, compression_options, part);
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

static HRESULT WINAPI opc_relationship_QueryInterface(IOpcRelationship *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IOpcRelationship) ||
            IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IOpcRelationship_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_relationship_AddRef(IOpcRelationship *iface)
{
    struct opc_relationship *relationship = impl_from_IOpcRelationship(iface);
    ULONG refcount = InterlockedIncrement(&relationship->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_relationship_Release(IOpcRelationship *iface)
{
    struct opc_relationship *relationship = impl_from_IOpcRelationship(iface);
    ULONG refcount = InterlockedDecrement(&relationship->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        CoTaskMemFree(relationship->id);
        CoTaskMemFree(relationship->type);
        IUri_Release(relationship->target);
        heap_free(relationship);
    }

    return refcount;
}

static HRESULT WINAPI opc_relationship_GetId(IOpcRelationship *iface, WCHAR **id)
{
    struct opc_relationship *relationship = impl_from_IOpcRelationship(iface);

    TRACE("iface %p, id %p.\n", iface, id);

    *id = opc_strdupW(relationship->id);
    return *id ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI opc_relationship_GetRelationshipType(IOpcRelationship *iface, WCHAR **type)
{
    struct opc_relationship *relationship = impl_from_IOpcRelationship(iface);

    TRACE("iface %p, type %p.\n", iface, type);

    *type = opc_strdupW(relationship->type);
    return *type ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI opc_relationship_GetSourceUri(IOpcRelationship *iface, IOpcUri **uri)
{
    FIXME("iface %p, uri %p stub!\n", iface, uri);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_relationship_GetTargetUri(IOpcRelationship *iface, IUri **target)
{
    struct opc_relationship *relationship = impl_from_IOpcRelationship(iface);

    TRACE("iface %p, target %p.\n", iface, target);

    *target = relationship->target;
    IUri_AddRef(*target);

    return S_OK;
}

static HRESULT WINAPI opc_relationship_GetTargetMode(IOpcRelationship *iface, OPC_URI_TARGET_MODE *target_mode)
{
    struct opc_relationship *relationship = impl_from_IOpcRelationship(iface);

    TRACE("iface %p, target_mode %p.\n", iface, target_mode);

    *target_mode = relationship->target_mode;

    return S_OK;
}

static const IOpcRelationshipVtbl opc_relationship_vtbl =
{
    opc_relationship_QueryInterface,
    opc_relationship_AddRef,
    opc_relationship_Release,
    opc_relationship_GetId,
    opc_relationship_GetRelationshipType,
    opc_relationship_GetSourceUri,
    opc_relationship_GetTargetUri,
    opc_relationship_GetTargetMode,
};

static HRESULT opc_relationship_create(struct opc_relationship_set *set, const WCHAR *id, const WCHAR *type,
        IUri *target_uri, OPC_URI_TARGET_MODE target_mode, IOpcRelationship **out)
{
    struct opc_relationship *relationship;

    if (!opc_array_reserve((void **)&set->relationships, &set->size, set->count + 1, sizeof(*set->relationships)))
        return E_OUTOFMEMORY;

    if (!(relationship = heap_alloc_zero(sizeof(*relationship))))
        return E_OUTOFMEMORY;

    relationship->IOpcRelationship_iface.lpVtbl = &opc_relationship_vtbl;
    relationship->refcount = 1;

    relationship->target = target_uri;
    IUri_AddRef(relationship->target);

    /* FIXME: test that id is unique */
    if (id)
        relationship->id = opc_strdupW(id);
    else
    {
        relationship->id = CoTaskMemAlloc(10 * sizeof(WCHAR));
        if (relationship->id)
        {
            static const WCHAR fmtW[] = {'R','%','0','8','X',0};
            DWORD generated;

            RtlGenRandom(&generated, sizeof(generated));
            sprintfW(relationship->id, fmtW, generated);
        }
    }

    relationship->type = opc_strdupW(type);
    if (!relationship->id || !relationship->type)
    {
        IOpcRelationship_Release(&relationship->IOpcRelationship_iface);
        return E_OUTOFMEMORY;
    }

    set->relationships[set->count++] = relationship;
    IOpcRelationship_AddRef(&relationship->IOpcRelationship_iface);

    *out = &relationship->IOpcRelationship_iface;
    TRACE("Created relationship %p.\n", *out);
    return S_OK;
}

static HRESULT WINAPI opc_relationship_set_QueryInterface(IOpcRelationshipSet *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IOpcRelationshipSet) ||
            IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IOpcRelationshipSet_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_relationship_set_AddRef(IOpcRelationshipSet *iface)
{
    struct opc_relationship_set *relationship_set = impl_from_IOpcRelationshipSet(iface);
    ULONG refcount = InterlockedIncrement(&relationship_set->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_relationship_set_Release(IOpcRelationshipSet *iface)
{
    struct opc_relationship_set *relationship_set = impl_from_IOpcRelationshipSet(iface);
    ULONG refcount = InterlockedDecrement(&relationship_set->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        size_t i;

        for (i = 0; i < relationship_set->count; ++i)
            IOpcRelationship_Release(&relationship_set->relationships[i]->IOpcRelationship_iface);
        heap_free(relationship_set->relationships);
        heap_free(relationship_set);
    }

    return refcount;
}

static HRESULT WINAPI opc_relationship_set_GetRelationship(IOpcRelationshipSet *iface, const WCHAR *id,
        IOpcRelationship **relationship)
{
    FIXME("iface %p, id %s, relationship %p stub!\n", iface, debugstr_w(id), relationship);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_relationship_set_CreateRelationship(IOpcRelationshipSet *iface, const WCHAR *id,
        const WCHAR *type, IUri *target_uri, OPC_URI_TARGET_MODE target_mode, IOpcRelationship **relationship)
{
    struct opc_relationship_set *relationship_set = impl_from_IOpcRelationshipSet(iface);

    TRACE("iface %p, id %s, type %s, target_uri %p, target_mode %d, relationship %p.\n", iface, debugstr_w(id),
            debugstr_w(type), target_uri, target_mode, relationship);

    if (!type || !target_uri)
        return E_POINTER;

    return opc_relationship_create(relationship_set, id, type, target_uri, target_mode, relationship);
}

static HRESULT WINAPI opc_relationship_set_DeleteRelationship(IOpcRelationshipSet *iface, const WCHAR *id)
{
    FIXME("iface %p, id %s stub!\n", iface, debugstr_w(id));

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_relationship_set_RelationshipExists(IOpcRelationshipSet *iface, const WCHAR *id, BOOL *exists)
{
    FIXME("iface %p, id %s, exists %p stub!\n", iface, debugstr_w(id), exists);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_relationship_set_GetEnumerator(IOpcRelationshipSet *iface,
        IOpcRelationshipEnumerator **enumerator)
{
    FIXME("iface %p, enumerator %p stub!\n", iface, enumerator);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_relationship_set_GetEnumeratorForType(IOpcRelationshipSet *iface, const WCHAR *type,
        IOpcRelationshipEnumerator **enumerator)
{
    FIXME("iface %p, type %s, enumerator %p stub!\n", iface, debugstr_w(type), enumerator);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_relationship_set_GetRelationshipsContentStream(IOpcRelationshipSet *iface, IStream **stream)
{
    FIXME("iface %p, stream %p stub!\n", iface, stream);

    return E_NOTIMPL;
}

static const IOpcRelationshipSetVtbl opc_relationship_set_vtbl =
{
    opc_relationship_set_QueryInterface,
    opc_relationship_set_AddRef,
    opc_relationship_set_Release,
    opc_relationship_set_GetRelationship,
    opc_relationship_set_CreateRelationship,
    opc_relationship_set_DeleteRelationship,
    opc_relationship_set_RelationshipExists,
    opc_relationship_set_GetEnumerator,
    opc_relationship_set_GetEnumeratorForType,
    opc_relationship_set_GetRelationshipsContentStream,
};

static HRESULT opc_relationship_set_create(IOpcRelationshipSet **out)
{
    struct opc_relationship_set *relationship_set;

    if (!(relationship_set = heap_alloc_zero(sizeof(*relationship_set))))
        return E_OUTOFMEMORY;

    relationship_set->IOpcRelationshipSet_iface.lpVtbl = &opc_relationship_set_vtbl;
    relationship_set->refcount = 1;

    *out = &relationship_set->IOpcRelationshipSet_iface;
    TRACE("Created relationship set %p.\n", *out);
    return S_OK;
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
    {
        if (package->part_set)
            IOpcPartSet_Release(package->part_set);
        if (package->relationship_set)
            IOpcRelationshipSet_Release(package->relationship_set);
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
    struct opc_package *package = impl_from_IOpcPackage(iface);
    HRESULT hr;

    TRACE("iface %p, relationship_set %p.\n", iface, relationship_set);

    if (!package->relationship_set && FAILED(hr = opc_relationship_set_create(&package->relationship_set)))
            return hr;

    *relationship_set = package->relationship_set;
    IOpcRelationshipSet_AddRef(*relationship_set);

    return S_OK;
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
