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
#include "xmllite.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "opc_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msopc);

struct opc_content
{
    LONG refcount;
    BYTE *data;
    ULARGE_INTEGER size;
};

struct opc_content_stream
{
    IStream IStream_iface;
    LONG refcount;

    struct opc_content *content;
    ULARGE_INTEGER pos;
};

struct opc_package
{
    IOpcPackage IOpcPackage_iface;
    LONG refcount;

    IOpcPartSet *part_set;
    IOpcRelationshipSet *relationship_set;
    IOpcUri *source_uri;
};

struct opc_part_enum
{
    IOpcPartEnumerator IOpcPartEnumerator_iface;
    LONG refcount;

    struct opc_part_set *part_set;
    size_t pos;
    GUID id;
};

struct opc_part
{
    IOpcPart IOpcPart_iface;
    LONG refcount;

    IOpcPartUri *name;
    WCHAR *content_type;
    DWORD compression_options;
    IOpcRelationshipSet *relationship_set;
    struct opc_content *content;
};

struct opc_part_set
{
    IOpcPartSet IOpcPartSet_iface;
    LONG refcount;

    struct opc_part **parts;
    size_t size;
    size_t count;
    GUID id;
};

struct opc_rel_enum
{
    IOpcRelationshipEnumerator IOpcRelationshipEnumerator_iface;
    LONG refcount;

    struct opc_relationship_set *rel_set;
    size_t pos;
    GUID id;
};

struct opc_relationship
{
    IOpcRelationship IOpcRelationship_iface;
    LONG refcount;

    WCHAR *id;
    WCHAR *type;
    IUri *target;
    OPC_URI_TARGET_MODE target_mode;
    IOpcUri *source_uri;
};

struct opc_relationship_set
{
    IOpcRelationshipSet IOpcRelationshipSet_iface;
    LONG refcount;

    struct opc_relationship **relationships;
    size_t size;
    size_t count;
    IOpcUri *source_uri;
    GUID id;
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

static inline struct opc_content_stream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct opc_content_stream, IStream_iface);
}

static inline struct opc_part_enum *impl_from_IOpcPartEnumerator(IOpcPartEnumerator *iface)
{
    return CONTAINING_RECORD(iface, struct opc_part_enum, IOpcPartEnumerator_iface);
}

static inline struct opc_rel_enum *impl_from_IOpcRelationshipEnumerator(IOpcRelationshipEnumerator *iface)
{
    return CONTAINING_RECORD(iface, struct opc_rel_enum, IOpcRelationshipEnumerator_iface);
}

static void opc_content_release(struct opc_content *content)
{
    ULONG refcount = InterlockedDecrement(&content->refcount);

    if (!refcount)
    {
        free(content->data);
        free(content);
    }
}

static HRESULT opc_part_enum_create(struct opc_part_set *part_set, IOpcPartEnumerator **out);

static HRESULT WINAPI opc_part_enum_QueryInterface(IOpcPartEnumerator *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(&IID_IOpcPartEnumerator, iid) ||
            IsEqualIID(&IID_IUnknown, iid))
    {
        *out = iface;
        IOpcPartEnumerator_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_part_enum_AddRef(IOpcPartEnumerator *iface)
{
    struct opc_part_enum *part_enum = impl_from_IOpcPartEnumerator(iface);
    ULONG refcount = InterlockedIncrement(&part_enum->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_part_enum_Release(IOpcPartEnumerator *iface)
{
    struct opc_part_enum *part_enum = impl_from_IOpcPartEnumerator(iface);
    ULONG refcount = InterlockedDecrement(&part_enum->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IOpcPartSet_Release(&part_enum->part_set->IOpcPartSet_iface);
        free(part_enum);
    }

    return refcount;
}

static BOOL has_part_collection_changed(const struct opc_part_enum *part_enum)
{
    return !IsEqualGUID(&part_enum->id, &part_enum->part_set->id);
}

static HRESULT WINAPI opc_part_enum_MoveNext(IOpcPartEnumerator *iface, BOOL *has_next)
{
    struct opc_part_enum *part_enum = impl_from_IOpcPartEnumerator(iface);

    TRACE("iface %p, has_next %p.\n", iface, has_next);

    if (!has_next)
        return E_POINTER;

    if (has_part_collection_changed(part_enum))
        return OPC_E_ENUM_COLLECTION_CHANGED;

    if (part_enum->pos == part_enum->part_set->count)
        return OPC_E_ENUM_CANNOT_MOVE_NEXT;

    if (part_enum->pos == ~(size_t)0 || part_enum->pos < part_enum->part_set->count)
        part_enum->pos++;

    *has_next = part_enum->pos < part_enum->part_set->count;

    return S_OK;
}

static HRESULT WINAPI opc_part_enum_MovePrevious(IOpcPartEnumerator *iface, BOOL *has_previous)
{
    struct opc_part_enum *part_enum = impl_from_IOpcPartEnumerator(iface);

    TRACE("iface %p, has_previous %p.\n", iface, has_previous);

    if (!has_previous)
        return E_POINTER;

    if (has_part_collection_changed(part_enum))
        return OPC_E_ENUM_COLLECTION_CHANGED;

    if (part_enum->pos == ~(size_t)0)
        return OPC_E_ENUM_CANNOT_MOVE_PREVIOUS;

    *has_previous = --part_enum->pos != ~(size_t)0;

    return S_OK;
}

static HRESULT WINAPI opc_part_enum_GetCurrent(IOpcPartEnumerator *iface, IOpcPart **part)
{
    struct opc_part_enum *part_enum = impl_from_IOpcPartEnumerator(iface);

    TRACE("iface %p, part %p.\n", iface, part);

    if (!part)
        return E_POINTER;

    *part = NULL;

    if (has_part_collection_changed(part_enum))
        return OPC_E_ENUM_COLLECTION_CHANGED;

    if (part_enum->pos < part_enum->part_set->count)
    {
        *part = &part_enum->part_set->parts[part_enum->pos]->IOpcPart_iface;
        IOpcPart_AddRef(*part);
    }

    return *part ? S_OK : OPC_E_ENUM_INVALID_POSITION;
}

static HRESULT WINAPI opc_part_enum_Clone(IOpcPartEnumerator *iface, IOpcPartEnumerator **out)
{
    struct opc_part_enum *part_enum = impl_from_IOpcPartEnumerator(iface);

    TRACE("iface %p, out %p.\n", iface, out);

    if (!out)
        return E_POINTER;

    if (has_part_collection_changed(part_enum))
    {
        *out = NULL;
        return OPC_E_ENUM_COLLECTION_CHANGED;
    }

    return opc_part_enum_create(part_enum->part_set, out);
}

static const IOpcPartEnumeratorVtbl opc_part_enum_vtbl =
{
    opc_part_enum_QueryInterface,
    opc_part_enum_AddRef,
    opc_part_enum_Release,
    opc_part_enum_MoveNext,
    opc_part_enum_MovePrevious,
    opc_part_enum_GetCurrent,
    opc_part_enum_Clone,
};

static HRESULT opc_part_enum_create(struct opc_part_set *part_set, IOpcPartEnumerator **out)
{
    struct opc_part_enum *part_enum;

    if (!(part_enum = calloc(1, sizeof(*part_enum))))
        return E_OUTOFMEMORY;

    part_enum->IOpcPartEnumerator_iface.lpVtbl = &opc_part_enum_vtbl;
    part_enum->refcount = 1;
    part_enum->part_set = part_set;
    IOpcPartSet_AddRef(&part_set->IOpcPartSet_iface);
    part_enum->pos = ~(size_t)0;
    part_enum->id = part_set->id;

    *out = &part_enum->IOpcPartEnumerator_iface;
    TRACE("Created part enumerator %p.\n", *out);
    return S_OK;
}

static HRESULT opc_rel_enum_create(struct opc_relationship_set *rel_set, IOpcRelationshipEnumerator **out);

static HRESULT WINAPI opc_rel_enum_QueryInterface(IOpcRelationshipEnumerator *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(&IID_IOpcRelationshipEnumerator, iid) ||
            IsEqualIID(&IID_IUnknown, iid))
    {
        *out = iface;
        IOpcRelationshipEnumerator_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_rel_enum_AddRef(IOpcRelationshipEnumerator *iface)
{
    struct opc_rel_enum *rel_enum = impl_from_IOpcRelationshipEnumerator(iface);
    ULONG refcount = InterlockedIncrement(&rel_enum->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_rel_enum_Release(IOpcRelationshipEnumerator *iface)
{
    struct opc_rel_enum *rel_enum = impl_from_IOpcRelationshipEnumerator(iface);
    ULONG refcount = InterlockedDecrement(&rel_enum->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IOpcRelationshipSet_Release(&rel_enum->rel_set->IOpcRelationshipSet_iface);
        free(rel_enum);
    }

    return refcount;
}

static BOOL has_rel_collection_changed(const struct opc_rel_enum *rel_enum)
{
    return !IsEqualGUID(&rel_enum->id, &rel_enum->rel_set->id);
}

static HRESULT WINAPI opc_rel_enum_MoveNext(IOpcRelationshipEnumerator *iface, BOOL *has_next)
{
    struct opc_rel_enum *rel_enum = impl_from_IOpcRelationshipEnumerator(iface);

    TRACE("iface %p, has_next %p.\n", iface, has_next);

    if (!has_next)
        return E_POINTER;

    if (has_rel_collection_changed(rel_enum))
        return OPC_E_ENUM_COLLECTION_CHANGED;

    if (rel_enum->rel_set->count && (rel_enum->pos == ~(size_t)0 || rel_enum->pos < rel_enum->rel_set->count))
        rel_enum->pos++;

    *has_next = rel_enum->pos < rel_enum->rel_set->count;

    return S_OK;
}

static HRESULT WINAPI opc_rel_enum_MovePrevious(IOpcRelationshipEnumerator *iface, BOOL *has_previous)
{
    struct opc_rel_enum *rel_enum = impl_from_IOpcRelationshipEnumerator(iface);

    TRACE("iface %p, has_previous %p.\n", iface, has_previous);

    if (!has_previous)
        return E_POINTER;

    if (has_rel_collection_changed(rel_enum))
        return OPC_E_ENUM_COLLECTION_CHANGED;

    if (rel_enum->pos != ~(size_t)0)
        rel_enum->pos--;

    *has_previous = rel_enum->pos != ~(size_t)0;

    return S_OK;
}

static HRESULT WINAPI opc_rel_enum_GetCurrent(IOpcRelationshipEnumerator *iface, IOpcRelationship **rel)
{
    struct opc_rel_enum *rel_enum = impl_from_IOpcRelationshipEnumerator(iface);

    TRACE("iface %p, rel %p.\n", iface, rel);

    if (!rel)
        return E_POINTER;

    *rel = NULL;

    if (has_rel_collection_changed(rel_enum))
        return OPC_E_ENUM_COLLECTION_CHANGED;

    if (rel_enum->pos < rel_enum->rel_set->count)
    {
        *rel = &rel_enum->rel_set->relationships[rel_enum->pos]->IOpcRelationship_iface;
        IOpcRelationship_AddRef(*rel);
    }

    return *rel ? S_OK : OPC_E_ENUM_INVALID_POSITION;
}

static HRESULT WINAPI opc_rel_enum_Clone(IOpcRelationshipEnumerator *iface, IOpcRelationshipEnumerator **out)
{
    struct opc_rel_enum *rel_enum = impl_from_IOpcRelationshipEnumerator(iface);

    TRACE("iface %p, out %p.\n", iface, out);

    if (!out)
        return E_POINTER;

    if (has_rel_collection_changed(rel_enum))
    {
        *out = NULL;
        return OPC_E_ENUM_COLLECTION_CHANGED;
    }

    return opc_rel_enum_create(rel_enum->rel_set, out);
}

static const IOpcRelationshipEnumeratorVtbl opc_rel_enum_vtbl =
{
    opc_rel_enum_QueryInterface,
    opc_rel_enum_AddRef,
    opc_rel_enum_Release,
    opc_rel_enum_MoveNext,
    opc_rel_enum_MovePrevious,
    opc_rel_enum_GetCurrent,
    opc_rel_enum_Clone,
};

static HRESULT opc_rel_enum_create(struct opc_relationship_set *rel_set, IOpcRelationshipEnumerator **out)
{
    struct opc_rel_enum *rel_enum;

    if (!(rel_enum = calloc(1, sizeof(*rel_enum))))
        return E_OUTOFMEMORY;

    rel_enum->IOpcRelationshipEnumerator_iface.lpVtbl = &opc_rel_enum_vtbl;
    rel_enum->refcount = 1;
    rel_enum->rel_set = rel_set;
    IOpcRelationshipSet_AddRef(&rel_set->IOpcRelationshipSet_iface);
    rel_enum->pos = ~(size_t)0;
    rel_enum->id = rel_set->id;

    *out = &rel_enum->IOpcRelationshipEnumerator_iface;
    TRACE("Created relationship enumerator %p.\n", *out);
    return S_OK;
}

static HRESULT WINAPI opc_content_stream_QueryInterface(IStream *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IStream) ||
            IsEqualIID(iid, &IID_ISequentialStream) ||
            IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IStream_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_content_stream_AddRef(IStream *iface)
{
    struct opc_content_stream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_content_stream_Release(IStream *iface)
{
    struct opc_content_stream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        opc_content_release(stream->content);
        free(stream);
    }

    return refcount;
}

static HRESULT WINAPI opc_content_stream_Read(IStream *iface, void *buff, ULONG size, ULONG *num_read)
{
    struct opc_content_stream *stream = impl_from_IStream(iface);
    DWORD read = 0;

    TRACE("%p, %p, %lu, %p.\n", iface, buff, size, num_read);

    if (!num_read)
        num_read = &read;

    if (stream->content->size.QuadPart - stream->pos.QuadPart < size)
        *num_read = stream->content->size.QuadPart - stream->pos.QuadPart;
    else
        *num_read = size;

    if (*num_read)
        memcpy(buff, stream->content->data + stream->pos.QuadPart, *num_read);

    stream->pos.QuadPart += *num_read;

    return S_OK;
}

static HRESULT WINAPI opc_content_stream_Write(IStream *iface, const void *data, ULONG size, ULONG *num_written)
{
    struct opc_content_stream *stream = impl_from_IStream(iface);
    DWORD written = 0;

    TRACE("%p, %p, %lu, %p.\n", iface, data, size, num_written);

    if (!num_written)
        num_written = &written;

    *num_written = 0;

    if (size > stream->content->size.QuadPart - stream->pos.QuadPart)
    {
        void *ptr = realloc(stream->content->data, stream->pos.QuadPart + size);
        if (!ptr)
            return E_OUTOFMEMORY;
        stream->content->data = ptr;
    }

    memcpy(stream->content->data + stream->pos.QuadPart, data, size);
    stream->pos.QuadPart += size;
    stream->content->size.QuadPart += size;
    *num_written = size;

    return S_OK;
}

static HRESULT WINAPI opc_content_stream_Seek(IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *newpos)
{
    struct opc_content_stream *stream = impl_from_IStream(iface);
    ULARGE_INTEGER pos;

    TRACE("%p, %s, %ld, %p.\n", iface, wine_dbgstr_longlong(move.QuadPart), origin, newpos);

    switch (origin)
    {
    case STREAM_SEEK_SET:
        pos.QuadPart = move.QuadPart;
        break;
    case STREAM_SEEK_CUR:
        pos.QuadPart = stream->pos.QuadPart + move.QuadPart;
        break;
    case STREAM_SEEK_END:
        pos.QuadPart = stream->content->size.QuadPart + move.QuadPart;
        break;
    default:
        WARN("Unknown origin mode %ld.\n", origin);
        return E_INVALIDARG;
    }

    stream->pos = pos;

    if (newpos)
        *newpos = stream->pos;

    return S_OK;
}

static HRESULT WINAPI opc_content_stream_SetSize(IStream *iface, ULARGE_INTEGER size)
{
    FIXME("iface %p, size %s stub!\n", iface, wine_dbgstr_longlong(size.QuadPart));

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_content_stream_CopyTo(IStream *iface, IStream *dest, ULARGE_INTEGER size,
        ULARGE_INTEGER *num_read, ULARGE_INTEGER *written)
{
    FIXME("iface %p, dest %p, size %s, num_read %p, written %p stub!\n", iface, dest,
            wine_dbgstr_longlong(size.QuadPart), num_read, written);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_content_stream_Commit(IStream *iface, DWORD flags)
{
    FIXME("iface %p, flags %#lx stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_content_stream_Revert(IStream *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_content_stream_LockRegion(IStream *iface, ULARGE_INTEGER offset,
        ULARGE_INTEGER size, DWORD lock_type)
{
    FIXME("iface %p, offset %s, size %s, lock_type %ld stub!\n", iface, wine_dbgstr_longlong(offset.QuadPart),
            wine_dbgstr_longlong(size.QuadPart), lock_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_content_stream_UnlockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size,
        DWORD lock_type)
{
    FIXME("iface %p, offset %s, size %s, lock_type %ld stub!\n", iface, wine_dbgstr_longlong(offset.QuadPart),
            wine_dbgstr_longlong(size.QuadPart), lock_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_content_stream_Stat(IStream *iface, STATSTG *statstg, DWORD flag)
{
    FIXME("iface %p, statstg %p, flag %ld stub!\n", iface, statstg, flag);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_content_stream_Clone(IStream *iface, IStream **result)
{
    FIXME("iface %p, result %p stub!\n", iface, result);

    return E_NOTIMPL;
}

static const IStreamVtbl opc_content_stream_vtbl =
{
    opc_content_stream_QueryInterface,
    opc_content_stream_AddRef,
    opc_content_stream_Release,
    opc_content_stream_Read,
    opc_content_stream_Write,
    opc_content_stream_Seek,
    opc_content_stream_SetSize,
    opc_content_stream_CopyTo,
    opc_content_stream_Commit,
    opc_content_stream_Revert,
    opc_content_stream_LockRegion,
    opc_content_stream_UnlockRegion,
    opc_content_stream_Stat,
    opc_content_stream_Clone,
};

static HRESULT opc_content_stream_create(struct opc_content *content, IStream **out)
{
    struct opc_content_stream *stream;

    if (!(stream = calloc(1, sizeof(*stream))))
        return E_OUTOFMEMORY;

    stream->IStream_iface.lpVtbl = &opc_content_stream_vtbl;
    stream->refcount = 1;
    stream->content = content;
    InterlockedIncrement(&content->refcount);

    *out = &stream->IStream_iface;

    TRACE("Created content stream %p.\n", *out);
    return S_OK;
}

static HRESULT opc_relationship_set_create(IOpcUri *source_uri, IOpcRelationshipSet **relationship_set);

static WCHAR *opc_strdupW(const WCHAR *str)
{
    WCHAR *ret = NULL;

    if (str)
    {
        size_t size;

        size = (lstrlenW(str) + 1) * sizeof(WCHAR);
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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_part_Release(IOpcPart *iface)
{
    struct opc_part *part = impl_from_IOpcPart(iface);
    ULONG refcount = InterlockedDecrement(&part->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (part->relationship_set)
            IOpcRelationshipSet_Release(part->relationship_set);
        IOpcPartUri_Release(part->name);
        CoTaskMemFree(part->content_type);
        opc_content_release(part->content);
        free(part);
    }

    return refcount;
}

static HRESULT WINAPI opc_part_GetRelationshipSet(IOpcPart *iface, IOpcRelationshipSet **relationship_set)
{
    struct opc_part *part = impl_from_IOpcPart(iface);
    HRESULT hr;

    TRACE("iface %p, relationship_set %p.\n", iface, relationship_set);

    if (!part->relationship_set && FAILED(hr = opc_relationship_set_create((IOpcUri *)part->name, &part->relationship_set)))
        return hr;

    *relationship_set = part->relationship_set;
    IOpcRelationshipSet_AddRef(*relationship_set);

    return S_OK;
}

static HRESULT WINAPI opc_part_GetContentStream(IOpcPart *iface, IStream **stream)
{
    struct opc_part *part = impl_from_IOpcPart(iface);

    TRACE("iface %p, stream %p.\n", iface, stream);

    if (!stream)
        return E_POINTER;

    return opc_content_stream_create(part->content, stream);
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
    struct opc_part *part = impl_from_IOpcPart(iface);

    TRACE("iface %p, options %p.\n", iface, options);

    *options = part->compression_options;
    return S_OK;
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

    if (!opc_array_reserve((void **)&set->parts, &set->size, set->count + 1, sizeof(*set->parts)))
        return E_OUTOFMEMORY;

    if (!(part = calloc(1, sizeof(*part))))
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

    part->content = calloc(1, sizeof(*part->content));
    if (!part->content)
    {
        IOpcPart_Release(&part->IOpcPart_iface);
        return E_OUTOFMEMORY;
    }
    part->content->refcount = 1;

    set->parts[set->count++] = part;
    IOpcPart_AddRef(&part->IOpcPart_iface);
    CoCreateGuid(&set->id);

    *out = &part->IOpcPart_iface;
    TRACE("Created part %p.\n", *out);
    return S_OK;
}

static struct opc_part *opc_part_set_get_part(const struct opc_part_set *part_set, IOpcPartUri *name)
{
    BOOL is_equal;
    size_t i;

    for (i = 0; i < part_set->count; ++i)
    {
        is_equal = FALSE;
        if (IOpcPartUri_IsEqual(part_set->parts[i]->name, (IUri *)name, &is_equal) == S_OK && is_equal)
            return part_set->parts[i];
    }

    return NULL;
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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_part_set_Release(IOpcPartSet *iface)
{
    struct opc_part_set *part_set = impl_from_IOpcPartSet(iface);
    ULONG refcount = InterlockedDecrement(&part_set->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        size_t i;

        for (i = 0; i < part_set->count; ++i)
            IOpcPart_Release(&part_set->parts[i]->IOpcPart_iface);
        free(part_set->parts);
        free(part_set);
    }

    return refcount;
}

static HRESULT WINAPI opc_part_set_GetPart(IOpcPartSet *iface, IOpcPartUri *name, IOpcPart **out)
{
    struct opc_part_set *part_set = impl_from_IOpcPartSet(iface);
    struct opc_part *part;

    TRACE("iface %p, name %p, out %p.\n", iface, name, out);

    if (!out)
        return E_POINTER;

    *out = NULL;

    if (!name)
        return E_POINTER;

    if ((part = opc_part_set_get_part(part_set, name)))
    {
        *out = &part->IOpcPart_iface;
        IOpcPart_AddRef(*out);
    }

    return *out ? S_OK : OPC_E_NO_SUCH_PART;
}

static HRESULT WINAPI opc_part_set_CreatePart(IOpcPartSet *iface, IOpcPartUri *name, LPCWSTR content_type,
        OPC_COMPRESSION_OPTIONS compression_options, IOpcPart **part)
{
    struct opc_part_set *part_set = impl_from_IOpcPartSet(iface);

    TRACE("iface %p, name %p, content_type %s, compression_options %#x, part %p.\n", iface, name,
            debugstr_w(content_type), compression_options, part);

    if (!part)
        return E_POINTER;

    *part = NULL;

    if (!name)
        return E_POINTER;

    if (opc_part_set_get_part(part_set, name))
        return OPC_E_DUPLICATE_PART;

    return opc_part_create(part_set, name, content_type, compression_options, part);
}

static HRESULT WINAPI opc_part_set_DeletePart(IOpcPartSet *iface, IOpcPartUri *name)
{
    FIXME("iface %p, name %p stub!\n", iface, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_part_set_PartExists(IOpcPartSet *iface, IOpcPartUri *name, BOOL *exists)
{
    struct opc_part_set *part_set = impl_from_IOpcPartSet(iface);

    TRACE("iface %p, name %p, exists %p.\n", iface, name, exists);

    if (!name || !exists)
        return E_POINTER;

    *exists = opc_part_set_get_part(part_set, name) != NULL;

    return S_OK;
}

static HRESULT WINAPI opc_part_set_GetEnumerator(IOpcPartSet *iface, IOpcPartEnumerator **enumerator)
{
    struct opc_part_set *part_set = impl_from_IOpcPartSet(iface);

    TRACE("iface %p, enumerator %p.\n", iface, enumerator);

    if (!enumerator)
        return E_POINTER;

    return opc_part_enum_create(part_set, enumerator);
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
    opc_part_set_GetEnumerator,
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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_relationship_Release(IOpcRelationship *iface)
{
    struct opc_relationship *relationship = impl_from_IOpcRelationship(iface);
    ULONG refcount = InterlockedDecrement(&relationship->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        CoTaskMemFree(relationship->id);
        CoTaskMemFree(relationship->type);
        IOpcUri_Release(relationship->source_uri);
        IUri_Release(relationship->target);
        free(relationship);
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
    struct opc_relationship *relationship = impl_from_IOpcRelationship(iface);

    TRACE("iface %p, uri %p.\n", iface, uri);

    *uri = relationship->source_uri;
    IOpcUri_AddRef(*uri);

    return S_OK;
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

static struct opc_relationship *opc_relationshipset_get_rel(struct opc_relationship_set *relationship_set,
        const WCHAR *id)
{
    size_t i;

    for (i = 0; i < relationship_set->count; i++)
    {
        if (!wcscmp(id, relationship_set->relationships[i]->id))
            return relationship_set->relationships[i];
    }

    return NULL;
}

static HRESULT opc_relationship_create(struct opc_relationship_set *set, const WCHAR *id, const WCHAR *type,
        IUri *target_uri, OPC_URI_TARGET_MODE target_mode, IOpcRelationship **out)
{
    struct opc_relationship *relationship;

    if (!opc_array_reserve((void **)&set->relationships, &set->size, set->count + 1, sizeof(*set->relationships)))
        return E_OUTOFMEMORY;

    if (!(relationship = calloc(1, sizeof(*relationship))))
        return E_OUTOFMEMORY;

    relationship->IOpcRelationship_iface.lpVtbl = &opc_relationship_vtbl;
    relationship->refcount = 1;

    relationship->target = target_uri;
    IUri_AddRef(relationship->target);
    relationship->source_uri = set->source_uri;
    IOpcUri_AddRef(relationship->source_uri);

    if (id)
        relationship->id = opc_strdupW(id);
    else
    {
        relationship->id = CoTaskMemAlloc(10 * sizeof(WCHAR));
        if (relationship->id)
        {
            DWORD generated;

            /* FIXME: test that generated id is unique */
            RtlGenRandom(&generated, sizeof(generated));
            swprintf(relationship->id, 10, L"R%08X", generated);

            if (opc_relationshipset_get_rel(set, relationship->id))
            {
                WARN("Newly generated id %s already exists.\n", debugstr_w(relationship->id));
                IOpcRelationship_Release(&relationship->IOpcRelationship_iface);
                return E_FAIL;
            }
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
    CoCreateGuid(&set->id);

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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_relationship_set_Release(IOpcRelationshipSet *iface)
{
    struct opc_relationship_set *relationship_set = impl_from_IOpcRelationshipSet(iface);
    ULONG refcount = InterlockedDecrement(&relationship_set->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        size_t i;

        for (i = 0; i < relationship_set->count; ++i)
            IOpcRelationship_Release(&relationship_set->relationships[i]->IOpcRelationship_iface);
        IOpcUri_Release(relationship_set->source_uri);
        free(relationship_set->relationships);
        free(relationship_set);
    }

    return refcount;
}

static HRESULT WINAPI opc_relationship_set_GetRelationship(IOpcRelationshipSet *iface, const WCHAR *id,
        IOpcRelationship **relationship)
{
    struct opc_relationship_set *relationship_set = impl_from_IOpcRelationshipSet(iface);
    struct opc_relationship *ret;

    TRACE("iface %p, id %s, relationship %p.\n", iface, debugstr_w(id), relationship);

    if (!relationship)
        return E_POINTER;

    *relationship = NULL;

    if (!id)
        return E_POINTER;

    if ((ret = opc_relationshipset_get_rel(relationship_set, id)))
    {
        *relationship = &ret->IOpcRelationship_iface;
        IOpcRelationship_AddRef(*relationship);
    }

    return *relationship ? S_OK : OPC_E_NO_SUCH_RELATIONSHIP;
}

static HRESULT WINAPI opc_relationship_set_CreateRelationship(IOpcRelationshipSet *iface, const WCHAR *id,
        const WCHAR *type, IUri *target_uri, OPC_URI_TARGET_MODE target_mode, IOpcRelationship **relationship)
{
    struct opc_relationship_set *relationship_set = impl_from_IOpcRelationshipSet(iface);
    DWORD length;

    TRACE("iface %p, id %s, type %s, target_uri %p, target_mode %d, relationship %p.\n", iface, debugstr_w(id),
            debugstr_w(type), target_uri, target_mode, relationship);

    if (!relationship)
        return E_POINTER;

    *relationship = NULL;

    if (!type || !target_uri)
        return E_POINTER;

    if (id && opc_relationshipset_get_rel(relationship_set, id))
        return OPC_E_DUPLICATE_RELATIONSHIP;

    if (IUri_GetPropertyLength(target_uri, Uri_PROPERTY_SCHEME_NAME, &length, 0) == S_OK && length != 0
            && target_mode == OPC_URI_TARGET_MODE_INTERNAL)
        return OPC_E_INVALID_RELATIONSHIP_TARGET;

    return opc_relationship_create(relationship_set, id, type, target_uri, target_mode, relationship);
}

static HRESULT WINAPI opc_relationship_set_DeleteRelationship(IOpcRelationshipSet *iface, const WCHAR *id)
{
    FIXME("iface %p, id %s stub!\n", iface, debugstr_w(id));

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_relationship_set_RelationshipExists(IOpcRelationshipSet *iface, const WCHAR *id, BOOL *exists)
{
    struct opc_relationship_set *relationship_set = impl_from_IOpcRelationshipSet(iface);

    TRACE("iface %p, id %s, exists %p.\n", iface, debugstr_w(id), exists);

    if (!id || !exists)
        return E_POINTER;

    *exists = opc_relationshipset_get_rel(relationship_set, id) != NULL;

    return S_OK;
}

static HRESULT WINAPI opc_relationship_set_GetEnumerator(IOpcRelationshipSet *iface,
        IOpcRelationshipEnumerator **enumerator)
{
    struct opc_relationship_set *relationship_set = impl_from_IOpcRelationshipSet(iface);

    TRACE("iface %p, enumerator %p.\n", iface, enumerator);

    if (!enumerator)
        return E_POINTER;

    return opc_rel_enum_create(relationship_set, enumerator);
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

static HRESULT opc_relationship_set_create(IOpcUri *source_uri, IOpcRelationshipSet **out)
{
    struct opc_relationship_set *relationship_set;

    if (!(relationship_set = calloc(1, sizeof(*relationship_set))))
        return E_OUTOFMEMORY;

    relationship_set->IOpcRelationshipSet_iface.lpVtbl = &opc_relationship_set_vtbl;
    relationship_set->refcount = 1;
    relationship_set->source_uri = source_uri;
    IOpcUri_AddRef(relationship_set->source_uri);

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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_package_Release(IOpcPackage *iface)
{
    struct opc_package *package = impl_from_IOpcPackage(iface);
    ULONG refcount = InterlockedDecrement(&package->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (package->part_set)
            IOpcPartSet_Release(package->part_set);
        if (package->relationship_set)
            IOpcRelationshipSet_Release(package->relationship_set);
        if (package->source_uri)
            IOpcUri_Release(package->source_uri);
        free(package);
    }

    return refcount;
}

static HRESULT WINAPI opc_package_GetPartSet(IOpcPackage *iface, IOpcPartSet **part_set)
{
    struct opc_package *package = impl_from_IOpcPackage(iface);

    TRACE("iface %p, part_set %p.\n", iface, part_set);

    if (!package->part_set)
    {
        struct opc_part_set *part_set = calloc(1, sizeof(*part_set));
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

    if (!package->relationship_set)
    {
        if (FAILED(hr = opc_relationship_set_create(package->source_uri, &package->relationship_set)))
            return hr;
    }

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

HRESULT opc_package_create(IOpcFactory *factory, IOpcPackage **out)
{
    struct opc_package *package;
    HRESULT hr;

    if (!(package = calloc(1, sizeof(*package))))
        return E_OUTOFMEMORY;

    package->IOpcPackage_iface.lpVtbl = &opc_package_vtbl;
    package->refcount = 1;

    if (FAILED(hr = IOpcFactory_CreatePackageRootUri(factory, &package->source_uri)))
    {
        free(package);
        return hr;
    }

    *out = &package->IOpcPackage_iface;
    TRACE("Created package %p.\n", *out);
    return S_OK;
}

struct content_types
{
    struct list types;
    BOOL has_rels_part;
};

enum content_type_element
{
    CONTENT_TYPE_DEFAULT,
    CONTENT_TYPE_OVERRIDE,
};

struct content_type
{
    struct list entry;
    enum content_type_element element;
    union
    {
        struct default_type
        {
            WCHAR *ext;
            WCHAR *type;
        } def;
        struct override_type
        {
            IOpcPart *part;
        } override;
    } u;
};

static HRESULT opc_package_add_override_content_type(struct content_types *types, IOpcPart *part)
{
    struct content_type *type;

    if (!(type = malloc(sizeof(*type))))
        return E_OUTOFMEMORY;

    type->element = CONTENT_TYPE_OVERRIDE;
    type->u.override.part = part;
    IOpcPart_AddRef(part);

    list_add_tail(&types->types, &type->entry);

    return S_OK;
}

static HRESULT opc_package_add_default_content_type(struct content_types *types,
        const WCHAR *ext, const WCHAR *content_type)
{
    struct content_type *type;

    if (!(type = malloc(sizeof(*type))))
        return E_OUTOFMEMORY;

    type->element = CONTENT_TYPE_DEFAULT;
    type->u.def.ext = opc_strdupW(ext);
    type->u.def.type = opc_strdupW(content_type);
    if (!type->u.def.ext || !type->u.def.type)
    {
        CoTaskMemFree(type->u.def.ext);
        CoTaskMemFree(type->u.def.type);
        free(type);
        return E_OUTOFMEMORY;
    }

    list_add_tail(&types->types, &type->entry);

    return S_OK;
}

static HRESULT opc_package_add_content_type(struct content_types *types, IOpcPart *part)
{
    struct content_type *cur;
    BSTR ext, content_type;
    BOOL added = FALSE;
    IOpcPartUri *name;
    HRESULT hr;

    if (FAILED(hr = IOpcPart_GetName(part, &name)))
        return hr;

    hr = IOpcPartUri_GetExtension(name, &ext);
    IOpcPartUri_Release(name);
    if (hr == S_FALSE)
    {
        hr = opc_package_add_override_content_type(types, part);
        SysFreeString(ext);
        return hr;
    }

    if (FAILED(hr))
        return hr;

    if (FAILED(hr = IOpcPart_GetContentType(part, &content_type)))
        return hr;

    LIST_FOR_EACH_ENTRY(cur, &types->types, struct content_type, entry)
    {
        if (cur->element == CONTENT_TYPE_OVERRIDE)
            continue;

        if (!wcsicmp(cur->u.def.ext, ext))
        {
            added = TRUE;

            if (!wcscmp(cur->u.def.type, content_type))
                break;

            hr = opc_package_add_override_content_type(types, part);
            break;
        }
    }

    if (!added)
        hr = opc_package_add_default_content_type(types, ext, content_type);

    SysFreeString(ext);
    SysFreeString(content_type);

    return hr;
}

static BOOL opc_package_has_rels_part(IOpcRelationshipSet *rel_set)
{
    IOpcRelationshipEnumerator *enumerator;
    BOOL has_next;
    HRESULT hr;

    if (FAILED(hr = IOpcRelationshipSet_GetEnumerator(rel_set, &enumerator)))
        return FALSE;

    has_next = FALSE;
    IOpcRelationshipEnumerator_MoveNext(enumerator, &has_next);
    IOpcRelationshipEnumerator_Release(enumerator);

    return has_next;
}

static HRESULT opc_package_collect_content_types(IOpcPackage *package, struct content_types *types)
{
    IOpcPartEnumerator *enumerator;
    IOpcRelationshipSet *rel_set;
    IOpcPartSet *parts;
    BOOL has_next;
    HRESULT hr;

    if (FAILED(hr = IOpcPackage_GetPartSet(package, &parts)))
        return hr;

    hr = IOpcPackage_GetRelationshipSet(package, &rel_set);
    if (SUCCEEDED(hr))
    {
        types->has_rels_part |= opc_package_has_rels_part(rel_set);
        IOpcRelationshipSet_Release(rel_set);
    }

    hr = IOpcPartSet_GetEnumerator(parts, &enumerator);
    IOpcPartSet_Release(parts);
    if (FAILED(hr))
        return hr;

    if (FAILED(hr = IOpcPartEnumerator_MoveNext(enumerator, &has_next)) || !has_next)
    {
        IOpcPartEnumerator_Release(enumerator);
        return hr;
    }

    while (has_next)
    {
        IOpcPart *part;

        if (FAILED(hr = IOpcPartEnumerator_GetCurrent(enumerator, &part)))
            break;

        if (!types->has_rels_part)
        {
            hr = IOpcPart_GetRelationshipSet(part, &rel_set);
            if (SUCCEEDED(hr))
            {
                types->has_rels_part |= opc_package_has_rels_part(rel_set);
                IOpcRelationshipSet_Release(rel_set);
            }
        }

        hr = opc_package_add_content_type(types, part);
        IOpcPart_Release(part);
        if (FAILED(hr))
            break;

        if (FAILED(hr = IOpcPartEnumerator_MoveNext(enumerator, &has_next)))
            break;
    }

    IOpcPartEnumerator_Release(enumerator);

    return hr;
}

static HRESULT opc_package_write_default_type(const WCHAR *ext, const WCHAR *type, IXmlWriter *writer)
{
    HRESULT hr;

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"Default", NULL);
    if (SUCCEEDED(hr))
        hr = IXmlWriter_WriteAttributeString(writer, NULL, L"Extension", NULL, ext);
    if (SUCCEEDED(hr))
        hr = IXmlWriter_WriteAttributeString(writer, NULL, L"ContentType", NULL, type);
    return hr;
}

static HRESULT opc_package_write_contenttypes(IOpcPackage *package, struct zip_archive *archive, IXmlWriter *writer)
{
    struct content_type *content_type, *content_type2;
    struct content_types types;
    IStream *content = NULL;
    HRESULT hr;

    list_init(&types.types);
    types.has_rels_part = FALSE;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &content);
    if (SUCCEEDED(hr))
        hr = opc_package_collect_content_types(package, &types);
    if (SUCCEEDED(hr))
        hr = IXmlWriter_SetOutput(writer, (IUnknown *)content);
    if (SUCCEEDED(hr))
        hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    if (SUCCEEDED(hr))
        hr = IXmlWriter_WriteStartElement(writer, NULL, L"Types",
                L"http://schemas.openxmlformats.org/package/2006/content-types");

    if (SUCCEEDED(hr) && types.has_rels_part)
    {
        hr = opc_package_write_default_type(L"rels",
                L"application/vnd.openxmlformats-package.relationships+xml", writer);
        if (SUCCEEDED(hr))
            hr = IXmlWriter_WriteEndElement(writer);
    }

    LIST_FOR_EACH_ENTRY_SAFE(content_type, content_type2, &types.types, struct content_type, entry)
    {
        if (content_type->element == CONTENT_TYPE_DEFAULT)
        {
            hr = opc_package_write_default_type(content_type->u.def.ext + 1, content_type->u.def.type, writer);

            CoTaskMemFree(content_type->u.def.ext);
            CoTaskMemFree(content_type->u.def.type);
        }
        else
        {
            IOpcPartUri *uri = NULL;
            WCHAR *type = NULL;
            BSTR name = NULL;

            if (SUCCEEDED(hr))
                hr = IXmlWriter_WriteStartElement(writer, NULL, L"Override", NULL);
            if (SUCCEEDED(hr))
                hr = IOpcPart_GetName(content_type->u.override.part, &uri);
            if (SUCCEEDED(hr))
                hr = IOpcPartUri_GetRawUri(uri, &name);
            if (SUCCEEDED(hr))
                hr = IXmlWriter_WriteAttributeString(writer, NULL, L"PartName", NULL, name);
            if (SUCCEEDED(hr))
                hr = IOpcPart_GetContentType(content_type->u.override.part, &type);
            if (SUCCEEDED(hr))
                hr = IXmlWriter_WriteAttributeString(writer, NULL, L"ContentType", NULL, type);

            if (uri)
                IOpcPartUri_Release(uri);
            SysFreeString(name);
            CoTaskMemFree(type);

            IOpcPart_Release(content_type->u.override.part);
        }
        if (SUCCEEDED(hr))
            hr = IXmlWriter_WriteEndElement(writer);

        list_remove(&content_type->entry);
        free(content_type);
    }

    if (SUCCEEDED(hr))
        hr = IXmlWriter_WriteEndDocument(writer);
    if (SUCCEEDED(hr))
        hr = IXmlWriter_Flush(writer);

    if (SUCCEEDED(hr))
        hr = compress_add_file(archive, L"[Content_Types].xml", content, OPC_COMPRESSION_NORMAL);

    if (content)
        IStream_Release(content);

    return hr;
}

static HRESULT opc_package_write_rel(IOpcRelationship *rel, IXmlWriter *writer)
{
    BSTR target_uri;
    HRESULT hr;
    WCHAR *str;
    IUri *uri;

    if (FAILED(hr = IXmlWriter_WriteStartElement(writer, NULL, L"Relationship", NULL)))
        return hr;

    if (FAILED(hr = IOpcRelationship_GetTargetUri(rel, &uri)))
        return hr;

    IUri_GetRawUri(uri, &target_uri);
    IUri_Release(uri);

    hr = IXmlWriter_WriteAttributeString(writer, NULL, L"Target", NULL, target_uri);
    SysFreeString(target_uri);
    if (FAILED(hr))
        return hr;

    if (FAILED(hr = IOpcRelationship_GetId(rel, &str)))
        return hr;

    hr = IXmlWriter_WriteAttributeString(writer, NULL, L"Id", NULL, str);
    CoTaskMemFree(str);
    if (FAILED(hr))
        return hr;

    if (FAILED(hr = IOpcRelationship_GetRelationshipType(rel, &str)))
        return hr;

    hr = IXmlWriter_WriteAttributeString(writer, NULL, L"Type", NULL, str);
    CoTaskMemFree(str);
    if (FAILED(hr))
        return hr;

    return IXmlWriter_WriteEndElement(writer);
}

static HRESULT opc_package_write_rels(struct zip_archive *archive, IOpcRelationshipSet *rels,
        IOpcUri *uri, IXmlWriter *writer)
{
    IOpcRelationshipEnumerator *enumerator;
    BSTR rels_part_uri = NULL;
    IOpcPartUri *rels_uri;
    IStream *content;
    BOOL has_next;
    HRESULT hr;

    if (FAILED(hr = IOpcRelationshipSet_GetEnumerator(rels, &enumerator)))
        return hr;

    hr = IOpcRelationshipEnumerator_MoveNext(enumerator, &has_next);
    if (!has_next)
    {
        IOpcRelationshipEnumerator_Release(enumerator);
        return hr;
    }

    if (FAILED(hr = CreateStreamOnHGlobal(NULL, TRUE, &content)))
    {
        IOpcRelationshipEnumerator_Release(enumerator);
        return hr;
    }

    hr = IXmlWriter_SetOutput(writer, (IUnknown *)content);
    if (SUCCEEDED(hr))
        hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    if (SUCCEEDED(hr))
        hr = IXmlWriter_WriteStartElement(writer, NULL, L"Relationships",
                L"http://schemas.openxmlformats.org/package/2006/relationships");

    while (has_next)
    {
        IOpcRelationship *rel;

        if (FAILED(hr = IOpcRelationshipEnumerator_GetCurrent(enumerator, &rel)))
            break;

        hr = opc_package_write_rel(rel, writer);
        IOpcRelationship_Release(rel);
        if (FAILED(hr))
            break;

        IOpcRelationshipEnumerator_MoveNext(enumerator, &has_next);
    }

    IOpcRelationshipEnumerator_Release(enumerator);

    if (SUCCEEDED(hr))
        hr = IXmlWriter_WriteEndDocument(writer);
    if (SUCCEEDED(hr))
        hr = IXmlWriter_Flush(writer);

    if (SUCCEEDED(hr))
        hr = IOpcUri_GetRelationshipsPartUri(uri, &rels_uri);
    if (SUCCEEDED(hr))
        hr = IOpcPartUri_GetRawUri(rels_uri, &rels_part_uri);
    if (SUCCEEDED(hr))
    {
        /* Relationship part names always start with root '/', skip it. */
        hr = compress_add_file(archive, rels_part_uri + 1, content, OPC_COMPRESSION_NORMAL);
    }

    SysFreeString(rels_part_uri);
    IStream_Release(content);

    return hr;
}

static HRESULT opc_package_write_part(struct zip_archive *archive, IOpcPart *part, IXmlWriter *writer)
{
    OPC_COMPRESSION_OPTIONS options = OPC_COMPRESSION_NORMAL;
    IOpcRelationshipSet *rels = NULL;
    IStream *content = NULL;
    IOpcPartUri *name;
    BSTR uri = NULL;
    HRESULT hr;

    if (FAILED(hr = IOpcPart_GetName(part, &name)))
        return hr;

    hr = IOpcPartUri_GetRawUri(name, &uri);
    if (SUCCEEDED(hr))
        hr = IOpcPart_GetCompressionOptions(part, &options);
    if (SUCCEEDED(hr))
        hr = IOpcPart_GetContentStream(part, &content);
    if (SUCCEEDED(hr))
    {
        /* Part names always start with root '/', skip it. */
        hr = compress_add_file(archive, uri + 1, content, options);
    }
    if (SUCCEEDED(hr))
        hr = IOpcPart_GetRelationshipSet(part, &rels);
    if (SUCCEEDED(hr))
        hr = opc_package_write_rels(archive, rels, (IOpcUri *)name, writer);

    IOpcPartUri_Release(name);
    SysFreeString(uri);
    if (content)
        IStream_Release(content);
    if (rels)
        IOpcRelationshipSet_Release(rels);

    return hr;
}

static HRESULT opc_package_write_parts(struct zip_archive *archive, IOpcPackage *package, IXmlWriter *writer)
{
    IOpcPartEnumerator *parts;
    IOpcPartSet *part_set;
    BOOL got_next;
    HRESULT hr;

    if (FAILED(hr = IOpcPackage_GetPartSet(package, &part_set)))
        return hr;

    hr = IOpcPartSet_GetEnumerator(part_set, &parts);
    IOpcPartSet_Release(part_set);
    if (FAILED(hr))
        return hr;

    while (IOpcPartEnumerator_MoveNext(parts, &got_next) == S_OK && got_next)
    {
        IOpcPart *part;

        if (FAILED(hr = IOpcPartEnumerator_GetCurrent(parts, &part)))
            break;

        hr = opc_package_write_part(archive, part, writer);
        IOpcPart_Release(part);
        if (FAILED(hr))
            break;
    }

    IOpcPartEnumerator_Release(parts);

    return hr;
}

HRESULT opc_package_write(IOpcPackage *package, OPC_WRITE_FLAGS flags, IStream *stream)
{
    IOpcRelationshipSet *rels = NULL;
    struct zip_archive *archive;
    IOpcUri *uri = NULL;
    IXmlWriter *writer;
    HRESULT hr;

    if (FAILED(hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL)))
        return hr;

    if (FAILED(hr = compress_create_archive(stream, flags != OPC_WRITE_FORCE_ZIP32, &archive)))
    {
        IXmlWriter_Release(writer);
        return hr;
    }

    /* [Content_Types].xml */
    hr = opc_package_write_contenttypes(package, archive, writer);
    /* Package relationships. */
    if (SUCCEEDED(hr))
        hr = IOpcPackage_GetRelationshipSet(package, &rels);
    if (SUCCEEDED(hr))
        hr = opc_root_uri_create(&uri);
    if (SUCCEEDED(hr))
        hr = opc_package_write_rels(archive, rels, uri, writer);
    /* Parts. */
    if (SUCCEEDED(hr))
        hr = opc_package_write_parts(archive, package, writer);
    if (SUCCEEDED(hr))
        hr = compress_finalize_archive(archive);

    if (rels)
        IOpcRelationshipSet_Release(rels);
    if (uri)
        IOpcUri_Release(uri);

    compress_release_archive(archive);
    IXmlWriter_Release(writer);

    return hr;
}
