/*
 * HTTP network byte stream implementation
 *
 * Copyright 2026 Reyka Matthies for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "nserror.h"
#include "mfidl.h"
#include "rpcproxy.h"
#include "rtworkq.h"
#include "bcrypt.h"
#include "pathcch.h"

#include "mf_private.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/mfinternal.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct bytestream_http;

#define MINIMUM_SEEK_TO_RECONNECT 12288

static BOOL is_range_available(struct bytestream_http *object, ULONGLONG start, ULONGLONG count);
static ULONGLONG find_range_end(struct bytestream_http *object, ULONGLONG start);
static ULONGLONG find_hole_end(struct bytestream_http *object, ULONGLONG start);
static void mark_range_valid(struct bytestream_http *object, ULONGLONG start, ULONGLONG count);

static HRESULT write_to_cache_file(struct bytestream_http *object, BYTE *buf, DWORD buflen);
static void read_complete(struct bytestream_http *object, ULONG size);
static HRESULT download_range_to_cache_file(struct bytestream_http *object, ULONGLONG pos, ULONG size);

static HRESULT do_read(struct bytestream_http *object, BYTE *buffer, ULONG size, ULONG *read_len);

static inline void handle_wininet_error(struct bytestream_http *object, DWORD err);
static inline void handle_http_error(struct bytestream_http *object, DWORD status);

static BOOL parse_content_range(struct bytestream_http *object, WCHAR *value);
static BOOL parse_headers(struct bytestream_http *object, HINTERNET handle);

static void CALLBACK progress_callback(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID info, DWORD infolen);

static void close_connection(struct bytestream_http *object, BOOL wait);
static HRESULT open_connection(struct bytestream_http *object, ULONGLONG range_start, ULONGLONG range_end);

static WCHAR *recreate_url(const WCHAR *url);

struct downloaded_range
{
    struct list entry;
    ULONGLONG start;
    ULONGLONG end;
};

struct bytestream_http
{
    IMFByteStream IMFByteStream_iface;
    IMFAttributes IMFAttributes_iface;
    LONG ref;

    IMFAttributes *attributes;

    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cv;

    HRESULT error;

    IMFAsyncCallback read_callback;
    struct list pending;
    ULONGLONG position;
    BOOL has_pending_read;

    WCHAR *url;
    BOOL secure;

    BOOL closing_connnection;
    HINTERNET session;
    HINTERNET request;

    BOOL parsed_headers;
    WCHAR *content_type;
    BOOL has_length;
    ULONGLONG content_length;
    ULONGLONG last_modified;
    ULONGLONG content_range_start;
    ULONGLONG content_range_end;

    BOOL request_complete;
    DWORD last_error;
    DWORD last_result;

    struct list downloaded_ranges;
    IMFByteStream *cache_file;

    ULONGLONG current_write_pos;
    DWORD bytes_read;
    BYTE buffer[1024*1024];
};

static struct bytestream_http *impl_from_IMFByteStream(IMFByteStream *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_http, IMFByteStream_iface);
}

static struct bytestream_http *impl_from_IMFAttributes(IMFAttributes *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_http, IMFAttributes_iface);
}

enum async_stream_op_type
{
    ASYNC_STREAM_OP_READ,
};

struct async_stream_op
{
    IUnknown IUnknown_iface;
    LONG refcount;
    union
    {
        const BYTE *src;
        BYTE *dest;
    } u;
    QWORD position;
    ULONG requested_length;
    ULONG actual_length;
    IMFAsyncResult *caller;
    struct list entry;
    enum async_stream_op_type type;
};

static struct async_stream_op *impl_async_stream_op_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct async_stream_op, IUnknown_iface);
}

static HRESULT WINAPI async_stream_op_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_stream_op_AddRef(IUnknown *iface)
{
    struct async_stream_op *op = impl_async_stream_op_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&op->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI async_stream_op_Release(IUnknown *iface)
{
    struct async_stream_op *op = impl_async_stream_op_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&op->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        if (op->caller)
            IMFAsyncResult_Release(op->caller);
        free(op);
    }

    return refcount;
}

static const IUnknownVtbl async_stream_op_vtbl =
{
    async_stream_op_QueryInterface,
    async_stream_op_AddRef,
    async_stream_op_Release,
};

static HRESULT WINAPI bytestream_http_QueryInterface(IMFByteStream *iface, REFIID riid, void **out)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFByteStream) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &object->IMFByteStream_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFAttributes))
    {
        *out = &object->IMFAttributes_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI bytestream_http_AddRef(IMFByteStream *iface)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);
    ULONG refcount = InterlockedIncrement(&object->ref);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI bytestream_http_Release(IMFByteStream *iface)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);
    ULONG refcount = InterlockedDecrement(&object->ref);
    struct async_stream_op *cur, *cur2;

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        close_connection(object, FALSE);
        IMFAttributes_Release(object->attributes);
        LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &object->pending, struct async_stream_op, entry)
        {
            list_remove(&cur->entry);
            IUnknown_Release(&cur->IUnknown_iface);
        }
        DeleteCriticalSection(&object->cs);
        free(object->content_type);
        free(object->url);
        free(object);
    }

    return refcount;
}

static HRESULT WINAPI bytestream_http_attributes_QueryInterface(IMFAttributes *iface, REFIID riid, void **obj)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);
    return IMFByteStream_QueryInterface(&object->IMFByteStream_iface, riid, obj);
}

static ULONG WINAPI bytestream_http_attributes_AddRef(IMFAttributes *iface)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);
    return IMFByteStream_AddRef(&object->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_http_attributes_Release(IMFAttributes *iface)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);
    return IMFByteStream_Release(&object->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_http_attributes_GetItem(IMFAttributes *iface, REFGUID key, PROPVARIANT *value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetItem(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_GetItemType(IMFAttributes *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), type);

    return IMFAttributes_GetItemType(object->attributes, key, type);
}

static HRESULT WINAPI bytestream_http_attributes_CompareItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(key), debugstr_propvar(value), result);

    return IMFAttributes_CompareItem(object->attributes, key, value, result);
}

static HRESULT WINAPI bytestream_http_attributes_Compare(IMFAttributes *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE match_type, BOOL *ret)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, match_type, ret);

    return IMFAttributes_Compare(object->attributes, theirs, match_type, ret);
}

static HRESULT WINAPI bytestream_http_attributes_GetUINT32(IMFAttributes *iface, REFGUID key, UINT32 *value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT32(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_GetUINT64(IMFAttributes *iface, REFGUID key, UINT64 *value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT64(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_GetDouble(IMFAttributes *iface, REFGUID key, double *value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetDouble(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_GetGUID(IMFAttributes *iface, REFGUID key, GUID *value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetGUID(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_GetStringLength(IMFAttributes *iface, REFGUID key, UINT32 *length)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), length);

    return IMFAttributes_GetStringLength(object->attributes, key, length);
}

static HRESULT WINAPI bytestream_http_attributes_GetString(IMFAttributes *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), value, size, length);

    return IMFAttributes_GetString(object->attributes, key, value, size, length);
}

static HRESULT WINAPI bytestream_http_attributes_GetAllocatedString(IMFAttributes *iface, REFGUID key, WCHAR **value, UINT32 *length)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, length);

    return IMFAttributes_GetAllocatedString(object->attributes, key, value, length);
}

static HRESULT WINAPI bytestream_http_attributes_GetBlobSize(IMFAttributes *iface, REFGUID key, UINT32 *size)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), size);

    return IMFAttributes_GetBlobSize(object->attributes, key, size);
}

static HRESULT WINAPI bytestream_http_attributes_GetBlob(IMFAttributes *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(object->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI bytestream_http_attributes_GetAllocatedBlob(IMFAttributes *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(object->attributes, key, buf, size);
}

static HRESULT WINAPI bytestream_http_attributes_GetUnknown(IMFAttributes *iface, REFGUID key, REFIID riid, void **out)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(key), debugstr_guid(riid), out);

    return IMFAttributes_GetUnknown(object->attributes, key, riid, out);
}

static HRESULT WINAPI bytestream_http_attributes_SetItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_propvar(value));

    return IMFAttributes_SetItem(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_DeleteItem(IMFAttributes *iface, REFGUID key)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s.\n", iface, debugstr_guid(key));

    return IMFAttributes_DeleteItem(object->attributes, key);
}

static HRESULT WINAPI bytestream_http_attributes_DeleteAllItems(IMFAttributes *iface)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_DeleteAllItems(object->attributes);
}

static HRESULT WINAPI bytestream_http_attributes_SetUINT32(IMFAttributes *iface, REFGUID key, UINT32 value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetUINT32(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_SetUINT64(IMFAttributes *iface, REFGUID key, UINT64 value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_SetDouble(IMFAttributes *iface, REFGUID key, double value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetDouble(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_SetGUID(IMFAttributes *iface, REFGUID key, REFGUID value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_mf_guid(value));

    return IMFAttributes_SetGUID(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_SetString(IMFAttributes *iface, REFGUID key, const WCHAR *value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFAttributes_SetString(object->attributes, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_SetBlob(IMFAttributes *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_SetBlob(object->attributes, key, buf, size);
}

static HRESULT WINAPI bytestream_http_attributes_SetUnknown(IMFAttributes *iface, REFGUID key, IUnknown *unknown)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), unknown);

    return IMFAttributes_SetUnknown(object->attributes, key, unknown);
}

static HRESULT WINAPI bytestream_http_attributes_LockStore(IMFAttributes *iface)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_LockStore(object->attributes);
}

static HRESULT WINAPI bytestream_http_attributes_UnlockStore(IMFAttributes *iface)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_UnlockStore(object->attributes);
}

static HRESULT WINAPI bytestream_http_attributes_GetCount(IMFAttributes *iface, UINT32 *count)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFAttributes_GetCount(object->attributes, count);
}

static HRESULT WINAPI bytestream_http_attributes_GetItemByIndex(IMFAttributes *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(object->attributes, index, key, value);
}

static HRESULT WINAPI bytestream_http_attributes_CopyAllItems(IMFAttributes *iface, IMFAttributes *dest)
{
    struct bytestream_http *object = impl_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, dest);

    return IMFAttributes_CopyAllItems(object->attributes, dest);
}

static const IMFAttributesVtbl bytestream_http_attributes_vtbl =
{
    bytestream_http_attributes_QueryInterface,
    bytestream_http_attributes_AddRef,
    bytestream_http_attributes_Release,
    bytestream_http_attributes_GetItem,
    bytestream_http_attributes_GetItemType,
    bytestream_http_attributes_CompareItem,
    bytestream_http_attributes_Compare,
    bytestream_http_attributes_GetUINT32,
    bytestream_http_attributes_GetUINT64,
    bytestream_http_attributes_GetDouble,
    bytestream_http_attributes_GetGUID,
    bytestream_http_attributes_GetStringLength,
    bytestream_http_attributes_GetString,
    bytestream_http_attributes_GetAllocatedString,
    bytestream_http_attributes_GetBlobSize,
    bytestream_http_attributes_GetBlob,
    bytestream_http_attributes_GetAllocatedBlob,
    bytestream_http_attributes_GetUnknown,
    bytestream_http_attributes_SetItem,
    bytestream_http_attributes_DeleteItem,
    bytestream_http_attributes_DeleteAllItems,
    bytestream_http_attributes_SetUINT32,
    bytestream_http_attributes_SetUINT64,
    bytestream_http_attributes_SetDouble,
    bytestream_http_attributes_SetGUID,
    bytestream_http_attributes_SetString,
    bytestream_http_attributes_SetBlob,
    bytestream_http_attributes_SetUnknown,
    bytestream_http_attributes_LockStore,
    bytestream_http_attributes_UnlockStore,
    bytestream_http_attributes_GetCount,
    bytestream_http_attributes_GetItemByIndex,
    bytestream_http_attributes_CopyAllItems
};

static struct bytestream_http *impl_from_read_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct bytestream_http, read_callback);
}

static HRESULT bytestream_http_create_io_request(struct bytestream_http *object, enum async_stream_op_type type,
        const BYTE *data, ULONG size, IMFAsyncCallback *callback, IUnknown *state)
{
    struct async_stream_op *op;
    IMFAsyncResult *request;
    HRESULT hr;

    if (type != ASYNC_STREAM_OP_READ)
        return E_NOTIMPL;

    op = malloc(sizeof(*op));
    if (!op)
        return E_OUTOFMEMORY;

    op->IUnknown_iface.lpVtbl = &async_stream_op_vtbl;
    op->refcount = 1;
    op->u.src = data;
    op->position = object->position;
    op->requested_length = size;
    op->type = type;
    if (FAILED(hr = MFCreateAsyncResult((IUnknown *)&object->IMFByteStream_iface, callback, state, &op->caller)))
        goto failed;

    if (FAILED(hr = MFCreateAsyncResult(&op->IUnknown_iface, &object->read_callback, NULL, &request)))
        goto failed;

    MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_IO, 0, (IUnknown *)request);
    IMFAsyncResult_Release(request);

failed:
    IUnknown_Release(&op->IUnknown_iface);
    return hr;
}

static HRESULT bytestream_http_complete_io_request(struct bytestream_http *object, enum async_stream_op_type type,
        IMFAsyncResult *result, ULONG *actual_length)
{
    struct async_stream_op *op = NULL, *cur;
    HRESULT hr;

    EnterCriticalSection(&object->cs);
    LIST_FOR_EACH_ENTRY(cur, &object->pending, struct async_stream_op, entry)
    {
        if (cur->caller == result && cur->type == type)
        {
            op = cur;
            list_remove(&cur->entry);
            break;
        }
    }
    LeaveCriticalSection(&object->cs);

    if (!op)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = IMFAsyncResult_GetStatus(result)))
        *actual_length = op->actual_length;

    IUnknown_Release(&op->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI bytestream_http_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI bytestream_http_read_callback_AddRef(IMFAsyncCallback *iface)
{
    struct bytestream_http *object = impl_from_read_callback_IMFAsyncCallback(iface);
    return IMFByteStream_AddRef(&object->IMFByteStream_iface);
}

static ULONG WINAPI bytestream_http_read_callback_Release(IMFAsyncCallback *iface)
{
    struct bytestream_http *object = impl_from_read_callback_IMFAsyncCallback(iface);
    return IMFByteStream_Release(&object->IMFByteStream_iface);
}

static HRESULT WINAPI bytestream_http_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI bytestream_http_read_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct bytestream_http *object = impl_from_read_callback_IMFAsyncCallback(iface);
    struct async_stream_op *op;
    IUnknown *resobj;
    HRESULT hr;

    if (FAILED(hr = IMFAsyncResult_GetObject(result, &resobj)))
        return hr;

    op = impl_async_stream_op_from_IUnknown(resobj);

    EnterCriticalSection(&object->cs);
    hr = do_read(object, op->u.dest, op->requested_length, &op->actual_length);
    if(FAILED(hr)) TRACE("Read failed: %#lx\n", hr);
    IMFAsyncResult_SetStatus(op->caller, hr);
    list_add_tail(&object->pending, &op->entry);
    LeaveCriticalSection(&object->cs);

    MFInvokeCallback(op->caller);

    return S_OK;
}

static const IMFAsyncCallbackVtbl bytestream_http_read_callback_vtbl =
{
    bytestream_http_callback_QueryInterface,
    bytestream_http_read_callback_AddRef,
    bytestream_http_read_callback_Release,
    bytestream_http_callback_GetParameters,
    bytestream_http_read_callback_Invoke,
};

static HRESULT WINAPI bytestream_http_GetCapabilities(IMFByteStream *iface, DWORD *capabilities)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, capabilities);

    EnterCriticalSection(&object->cs);
    *capabilities = MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE;
    if (!object->has_length || !is_range_available(object, 0, object->content_length))
        *capabilities |= MFBYTESTREAM_IS_PARTIALLY_DOWNLOADED;
    LeaveCriticalSection(&object->cs);
    return S_OK;
}

static HRESULT WINAPI bytestream_http_SetLength(IMFByteStream *iface, QWORD length)
{
    TRACE("%p, %s\n", iface, wine_dbgstr_longlong(length));

    return E_NOTIMPL;
}

static HRESULT WINAPI bytestream_http_GetCurrentPosition(IMFByteStream *iface, QWORD *position)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, position);

    if (!position)
        return E_INVALIDARG;

    EnterCriticalSection(&object->cs);
    *position = object->position;
    LeaveCriticalSection(&object->cs);
    return S_OK;
}

static HRESULT WINAPI bytestream_http_GetLength(IMFByteStream *iface, QWORD *length)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, length);

    EnterCriticalSection(&object->cs);

    if (!object->has_length)
    {
        LeaveCriticalSection(&object->cs);
        return MF_E_BYTESTREAM_UNKNOWN_LENGTH;
    }

    *length = object->content_length;
    TRACE("returrning %I64u\n", *length);

    LeaveCriticalSection(&object->cs);
    return S_OK;
}

static HRESULT WINAPI bytestream_http_IsEndOfStream(IMFByteStream *iface, BOOL *ret)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);

    TRACE("%p, %p.\n", iface, ret);

    EnterCriticalSection(&object->cs);
    *ret = object->position >= object->content_length;
    LeaveCriticalSection(&object->cs);
    return S_OK;
}

static HRESULT WINAPI bytestream_http_Read(IMFByteStream *iface, BYTE *buffer, ULONG size, ULONG *read_len)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);
    HRESULT hr;

    TRACE("%p, %p, %lu, %p.\n", iface, buffer, size, read_len);

    EnterCriticalSection(&object->cs);
    hr = do_read(object, buffer, size, read_len);
    LeaveCriticalSection(&object->cs);
    return hr;
}

static HRESULT WINAPI bytestream_http_BeginRead(IMFByteStream *iface, BYTE *data, ULONG size, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);

    TRACE("%p, %p, %lu, %p, %p.\n", iface, data, size, callback, state);

    return bytestream_http_create_io_request(object, ASYNC_STREAM_OP_READ, data, size, callback, state);
}

static HRESULT WINAPI bytestream_http_EndRead(IMFByteStream *iface, IMFAsyncResult *result, ULONG *byte_read)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);

    TRACE("%p, %p, %p.\n", iface, result, byte_read);

    return bytestream_http_complete_io_request(object, ASYNC_STREAM_OP_READ, result, byte_read);
}

static HRESULT WINAPI bytestream_http_Write(IMFByteStream *iface, const BYTE *data, ULONG size, ULONG *written)
{
    TRACE("%p, %p, %lu, %p\n", iface, data, size, written);

    return E_NOTIMPL;
}

static HRESULT WINAPI bytestream_http_BeginWrite(IMFByteStream *iface, const BYTE *data, ULONG size,
        IMFAsyncCallback *callback, IUnknown *state)
{
    TRACE("%p, %p, %lu, %p, %p.\n", iface, data, size, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI bytestream_http_EndWrite(IMFByteStream *iface, IMFAsyncResult *result, ULONG *written)
{
    TRACE("%p, %p, %p.\n", iface, result, written);

    return E_NOTIMPL;
}

static HRESULT WINAPI bytestream_http_Seek(IMFByteStream *iface, MFBYTESTREAM_SEEK_ORIGIN origin, LONGLONG offset,
                                           DWORD flags, QWORD *current)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %s, %#lx, %p.\n", iface, origin, wine_dbgstr_longlong(offset), flags, current);

    EnterCriticalSection(&object->cs);

    switch (origin)
    {
        case msoBegin:
            object->position = offset;
            break;
        case msoCurrent:
            object->position += offset;
            break;
        default:
            WARN("Unknown origin mode %d.\n", origin);
            hr = E_INVALIDARG;
    }

    *current = object->position;

    LeaveCriticalSection(&object->cs);

    return hr;
}

static HRESULT WINAPI bytestream_http_Flush(IMFByteStream *iface)
{
    TRACE("%p\n", iface);

    return S_OK;
}

static HRESULT WINAPI bytestream_http_Close(IMFByteStream *iface)
{
    struct bytestream_http *object = impl_from_IMFByteStream(iface);
    struct async_stream_op *cur, *cur2;

    TRACE("%p\n", iface);

    EnterCriticalSection(&object->cs);
    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &object->pending, struct async_stream_op, entry)
    {
        list_remove(&cur->entry);
        IUnknown_Release(&cur->IUnknown_iface);
    }
    object->error = MF_E_SHUTDOWN;
    close_connection(object, TRUE);
    LeaveCriticalSection(&object->cs);
    return S_OK;
}

static HRESULT WINAPI bytestream_http_SetCurrentPosition(IMFByteStream *iface, QWORD position)
{
    QWORD new_pos;

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(position));

    return IMFByteStream_Seek(iface, msoBegin, position, 0, &new_pos);
}

static const IMFByteStreamVtbl bytestream_http_vtbl =
{
    bytestream_http_QueryInterface,
    bytestream_http_AddRef,
    bytestream_http_Release,
    bytestream_http_GetCapabilities,
    bytestream_http_GetLength,
    bytestream_http_SetLength,
    bytestream_http_GetCurrentPosition,
    bytestream_http_SetCurrentPosition,
    bytestream_http_IsEndOfStream,
    bytestream_http_Read,
    bytestream_http_BeginRead,
    bytestream_http_EndRead,
    bytestream_http_Write,
    bytestream_http_BeginWrite,
    bytestream_http_EndWrite,
    bytestream_http_Seek,
    bytestream_http_Flush,
    bytestream_http_Close,
};

static BOOL is_range_available(struct bytestream_http *object, ULONGLONG start, ULONGLONG count)
{
    struct downloaded_range *entry;
    ULONGLONG end = start + count;

    if (object->has_length && end > object->content_length)
        end = object->content_length;

    LIST_FOR_EACH_ENTRY(entry, &object->downloaded_ranges, struct downloaded_range, entry)
    {
        if (entry->start <= start && entry->end >= end)
            return TRUE;
        if (entry->start >= end)
            break;
    }
    return FALSE;
}

static ULONGLONG find_range_end(struct bytestream_http *object, ULONGLONG start)
{
    struct downloaded_range *entry;

    LIST_FOR_EACH_ENTRY(entry, &object->downloaded_ranges, struct downloaded_range, entry)
    {
        if (entry->start <= start && entry->end >= start)
            return entry->end;
    }
    return -1ULL;
}

static ULONGLONG find_hole_end(struct bytestream_http *object, ULONGLONG start)
{
    struct downloaded_range *entry;

    LIST_FOR_EACH_ENTRY(entry, &object->downloaded_ranges, struct downloaded_range, entry)
    {
        if (entry->start >= start)
            return entry->start;
    }
    return -1ULL;
}

static void mark_range_valid(struct bytestream_http *object, ULONGLONG start, ULONGLONG count)
{
    struct downloaded_range *entry, *new_entry;
    ULONGLONG end = start + count;

    TRACE("%p %I64u %I64u\n", object, start, end);

    LIST_FOR_EACH_ENTRY(entry, &object->downloaded_ranges, struct downloaded_range, entry)
    {
        struct downloaded_range *next = LIST_ENTRY(entry->entry.next, struct downloaded_range, entry);

        if (start >= entry->start && start <= entry->end)
        {
            if (end > entry->end)
                entry->end = end - entry->start;

            if (&next->entry != &object->downloaded_ranges && next->start <= entry->end)
            {
                entry->end = next->end;
                list_remove(&next->entry);
                free(next);
            }

            return;
        }
        else if (&next->entry != &object->downloaded_ranges && start < next->start)
            break;
    }

    new_entry = calloc(1, sizeof(*new_entry));
    list_init(&new_entry->entry);
    new_entry->start = start;
    new_entry->end = end;
    list_add_before(&entry->entry, &new_entry->entry);
}

static HRESULT write_to_cache_file(struct bytestream_http *object, BYTE *buf, DWORD buflen)
{
    DWORD written = 0;
    QWORD new_pos = 0;
    HRESULT hr;

    hr = IMFByteStream_Seek(object->cache_file, msoBegin, object->current_write_pos, 0, &new_pos);
    if (SUCCEEDED(hr) && new_pos != object->current_write_pos)
        hr = E_FAIL;
    if (FAILED(hr))
    {
        if (!object->error)
            object->error = hr;
        return hr;
    }

    hr = IMFByteStream_Write(object->cache_file, buf, buflen, &written);
    if (SUCCEEDED(hr) && written < buflen)
        hr = E_FAIL;
    if (FAILED(hr))
    {
        if (!object->error)
            object->error = hr;
        return hr;
    }

    mark_range_valid(object, object->current_write_pos, written);
    object->current_write_pos += written;
    return S_OK;
}

static void read_complete(struct bytestream_http *object, ULONG size)
{
    ULONGLONG pos = object->current_write_pos;
    if (size == 0 && object->content_range_end == -1ULL && find_hole_end(object, pos) == -1ULL &&
        (find_range_end(object, pos) == pos || find_range_end(object, pos) == -1ULL))
    {
        if (!object->has_length || object->current_write_pos > object->content_length)
            object->content_length = object->current_write_pos;
        if (!object->has_length)
            object->has_length = TRUE;
    }
}

static HRESULT download_range_to_cache_file(struct bytestream_http *object, ULONGLONG pos, ULONG size)
{
    HRESULT hr = S_OK;
    ULONGLONG end;
    DWORD err;

    while (!object->error && object->has_pending_read)
        SleepConditionVariableCS(&object->cv, &object->cs, INFINITE);

    object->has_pending_read = TRUE;

    if (!object->request || object->closing_connnection || object->current_write_pos > pos ||
        object->current_write_pos + MINIMUM_SEEK_TO_RECONNECT <= pos)
    {
        end = find_hole_end(object, pos);
        if (FAILED(hr = open_connection(object, pos, end)))
        {
            object->has_pending_read = FALSE;
            return hr;
        }
    }

    while (SUCCEEDED(hr) && !object->error && !is_range_available(object, pos, size))
    {
        DWORD to_read = pos + size - object->current_write_pos;
        if (to_read > ARRAY_SIZE(object->buffer))
            to_read = ARRAY_SIZE(object->buffer);
        if (to_read == 0)
            break;

        object->request_complete = FALSE;
        if (InternetReadFile(object->request, object->buffer, to_read, &object->bytes_read))
        {
            hr = write_to_cache_file(object, object->buffer, object->bytes_read);
            if (!hr && to_read > 0)
                read_complete(object, object->bytes_read);
            if (object->bytes_read == 0)
                break;
        }
        else
        {
            err = GetLastError();
            if (err != ERROR_IO_PENDING)
                hr = HRESULT_FROM_WIN32(err);
            else
            {
                while (!object->request_complete && !object->error)
                    SleepConditionVariableCS(&object->cv, &object->cs, INFINITE);

                if (object->error)
                    hr = object->error;
                else if (object->last_result)
                {
                    hr = write_to_cache_file(object, object->buffer, object->bytes_read);
                    if (!hr && to_read > 0)
                        read_complete(object, object->bytes_read);
                    if (object->bytes_read == 0)
                        break;
                }
                else
                    hr = HRESULT_FROM_WIN32(object->last_error);
            }
        }
    }

    object->has_pending_read = FALSE;
    object->error = hr;
    return hr;
}

static HRESULT do_read(struct bytestream_http *object, BYTE *buffer, ULONG size, ULONG *read_len)
{
    ULONGLONG pos = object->position;
    QWORD new_pos = 0;
    HRESULT hr = S_OK;

    if (!object->error && !is_range_available(object, pos, size))
    {
        hr = download_range_to_cache_file(object, pos, size);
        if (FAILED(hr))
        {
            object->error = hr;
            return hr;
        }
    }

    if (object->error)
    {
        hr = object->error;
        return hr;
    }

    if (object->has_length && pos + size > object->content_length)
        size = object->content_length - pos;

    if (SUCCEEDED(hr = IMFByteStream_Seek(object->cache_file, msoBegin, pos, 0, &new_pos)))
    {
        if (new_pos != pos)
            size = 0;

        if (SUCCEEDED(hr = IMFByteStream_Read(object->cache_file, buffer, size, read_len)))
        {
            if (object->position == pos)
                object->position += *read_len;
        }
    }

    return hr;
}

static inline HRESULT map_wininet_error(DWORD err)
{
    HRESULT hr = HRESULT_FROM_WIN32(err);

    if (err == ERROR_INTERNET_NAME_NOT_RESOLVED || err == ERROR_INTERNET_CANNOT_CONNECT)
        hr = NS_E_SERVER_NOT_FOUND;
    else if (err == ERROR_INTERNET_CONNECTION_ABORTED || err == ERROR_INTERNET_TIMEOUT)
        hr = NS_E_CONNECTION_FAILURE;

    return hr;
}

static inline void handle_wininet_error(struct bytestream_http *object, DWORD err)
{
    HRESULT hr = map_wininet_error(err);

    TRACE("error %#lx hr %#lx\n", err, hr);

    if (err == ERROR_INVALID_HANDLE && object->closing_connnection)
        return;

    if (!object->error && FAILED(hr))
    {
        object->error = hr;
        WakeAllConditionVariable(&object->cv);
    }
}

static inline HRESULT map_http_error(DWORD http_error)
{
    TRACE("%lu.\n", http_error);

    if ((http_error >= 100 && http_error <= 199 && http_error != 101) ||
        (http_error >= 301 && http_error <= 303) || http_error == 307)
        return NS_E_CONNECTION_FAILURE;

    if (http_error == 401 || http_error == 403 || http_error == 407)
        return E_ACCESSDENIED;
    else if (http_error == 404)
        return NS_E_FILE_NOT_FOUND;
    else if (http_error == 410)
        return NS_E_RESOURCE_GONE;
    else if (http_error >= 400 && http_error <= 499)
        return NS_E_BAD_REQUEST;

    if (http_error == 500)
        return NS_E_INTERNAL_SERVER_ERROR;
    else if (http_error == 502)
        return NS_E_ERROR_FROM_PROXY;
    else if (http_error == 503)
        return NS_E_SERVER_UNAVAILABLE;
    else if (http_error == 504)
        return NS_E_PROXY_TIMEOUT;
    else if (http_error >= 500 && http_error <= 599 && http_error != 505)
        return NS_E_BAD_REQUEST;

    /* Not an error, return response body. */
    return 0;
}

static inline void handle_http_error(struct bytestream_http *object, DWORD status)
{
    HRESULT hr = map_http_error(status);

    TRACE("status %ld hr %#lx\n", status, hr);

    if (!object->error && FAILED(hr))
    {
        object->error = hr;
        WakeAllConditionVariable(&object->cv);
    }
}

static BOOL parse_content_range(struct bytestream_http *object, WCHAR *value)
{
    static const WCHAR bytesW[] = L"bytes ";

    ULONGLONG start, end, len;
    WCHAR *ptr;

    if (wcsncmp(value, bytesW, ARRAY_SIZE(bytesW) - 1) || value[6] == '-')
        return FALSE;
    ptr = value + 6;
    if (!wcsncmp(ptr, L"*/", 2))
    {
        start = 0;
        end = -1ULL;
        ptr++;
    }
    else
    {
        start = wcstoull(ptr, &ptr, 10);
        if (*ptr != '-')
            return FALSE;
        if (ptr[1] == '/')
            end = -1ULL;
        else
            end = wcstoull(ptr + 1, &ptr, 10);
    }
    if (*ptr != '/' || ptr[1] == 0)
        return FALSE;
    if (ptr[1] != '*')
    {
        len = wcstoull(ptr + 1, &ptr, 10);
        if (*ptr != 0)
            return FALSE;
        object->has_length = TRUE;
        object->content_length = len;
    }
    object->content_range_start = start;
    object->content_range_end = end;
    return TRUE;
}

static BOOL parse_headers(struct bytestream_http *object, HINTERNET handle)
{
    WCHAR buffer[1024];
    DWORD len;

    if (!object->content_type)
    {
        len = sizeof(buffer);
        if (HttpQueryInfoW(handle, HTTP_QUERY_CONTENT_TYPE, buffer, &len, 0))
        {
            object->content_type = wcsdup(buffer);
        }
        else if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND)
        {
            object->content_type = NULL;
        }
        else
        {
            handle_wininet_error(object, GetLastError());
            return FALSE;
        }
    }

    if (object->content_length == -1ULL)
    {
        len = sizeof(buffer);
        if (HttpQueryInfoW(handle, HTTP_QUERY_CONTENT_LENGTH, buffer, &len, 0))
        {
            WCHAR *end = buffer;
            object->content_length = wcstoull(buffer, &end, 10);
            if (end == buffer)
            {
                handle_wininet_error(object, ERROR_HTTP_INVALID_HEADER);
                WakeAllConditionVariable(&object->cv);
                return FALSE;
            }
            else
                object->has_length = TRUE;
        }
        else if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND)
        {
            object->content_length = -1ULL;
        }
        else
        {
            handle_wininet_error(object, GetLastError());
            return FALSE;
        }
    }

    if (object->last_modified == -1ULL)
    {
        SYSTEMTIME st = {0};
        len = sizeof(st);
        if (HttpQueryInfoW(handle, HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME, &st, &len, 0))
        {
            ULARGE_INTEGER ul = {0};
            FILETIME ft = {0};

            if (!SystemTimeToFileTime(&st, &ft))
            {
                handle_wininet_error(object, ERROR_HTTP_INVALID_HEADER);
                return FALSE;
            }
            ul.u.LowPart = ft.dwLowDateTime;
            ul.u.HighPart = ft.dwHighDateTime;
            object->last_modified = ul.QuadPart;
        }
        else if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND)
        {
            object->last_modified = -1ULL;
        }
        else
        {
            handle_wininet_error(object, GetLastError());
            return FALSE;
        }
    }

    len = sizeof(buffer);
    if (HttpQueryInfoW(handle, HTTP_QUERY_CONTENT_RANGE, buffer, &len, 0))
    {
        parse_content_range(object, buffer);
        object->current_write_pos = object->content_range_start;
    }
    else if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND)
    {
        object->content_range_start = 0;
        object->content_range_end = -1ULL;
        object->current_write_pos = 0;
    }
    else
    {
        handle_wininet_error(object, GetLastError());
        return FALSE;
    }
    return TRUE;
}

static void CALLBACK progress_callback(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID info, DWORD infolen)
{
    struct bytestream_http *object = (void *)context;

    TRACE("%p, %p, %lx, %p, %lu\n", handle, (void *)context, status, info, infolen);

    switch (status)
    {
    case INTERNET_STATUS_HANDLE_CREATED:
    {
        HINTERNET *new_handle = info;

        EnterCriticalSection(&object->cs);

        object->request = *new_handle;

        LeaveCriticalSection(&object->cs);
        WakeAllConditionVariable(&object->cv);
        break;
    }

    case INTERNET_STATUS_RESPONSE_RECEIVED:
    {
        DWORD status = 0, len = sizeof(status), avail = 0;

        EnterCriticalSection(&object->cs);

        if (object->error || object->closing_connnection || object->parsed_headers)
        {
            LeaveCriticalSection(&object->cs);
            return;
        }

        if (!HttpQueryInfoW(handle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &len, 0))
            handle_wininet_error(object, GetLastError());
        else if (status < 200 || status >= 300)
            handle_http_error(object, status);
        else if (parse_headers(object, handle) && !InternetQueryDataAvailable(handle, &avail, 0, 0))
            handle_wininet_error(object, GetLastError());

        object->parsed_headers = TRUE;

        LeaveCriticalSection(&object->cs);
        WakeAllConditionVariable(&object->cv);
        break;
    }

    case INTERNET_STATUS_REQUEST_COMPLETE:
    {
        INTERNET_ASYNC_RESULT *result = info;

        EnterCriticalSection(&object->cs);

        object->request_complete = TRUE;
        object->last_result = result->dwResult;
        object->last_error = result->dwError;

        LeaveCriticalSection(&object->cs);
        WakeAllConditionVariable(&object->cv);
        break;
    }

    case INTERNET_STATUS_CLOSING_CONNECTION:
    case INTERNET_STATUS_CONNECTION_CLOSED:
    {
        EnterCriticalSection(&object->cs);

        object->request_complete = TRUE;
        object->last_result = ERROR_INVALID_HANDLE;
        object->last_error = ERROR_INVALID_HANDLE;

        read_complete(object, 0);

        LeaveCriticalSection(&object->cs);
        WakeAllConditionVariable(&object->cv);
    }

    case INTERNET_STATUS_HANDLE_CLOSING:
    {
        BOOL release = TRUE;

        EnterCriticalSection(&object->cs);

        if (handle == object->request)
        {
            object->request_complete = TRUE;
            object->last_result = ERROR_INVALID_HANDLE;
            object->last_error = ERROR_INVALID_HANDLE;
            object->request = NULL;
            read_complete(object, 0);
        }
        else if (handle == object->session)
            object->session = NULL;
        else
            release = FALSE;

        LeaveCriticalSection(&object->cs);
        WakeAllConditionVariable(&object->cv);

        if (release)
            IMFByteStream_Release(&object->IMFByteStream_iface);
        break;
    }
    }
}

static void close_connection(struct bytestream_http *object, BOOL wait)
{
    if (wait)
    {
        while (object->closing_connnection)
            SleepConditionVariableCS(&object->cv, &object->cs, INFINITE);
        object->closing_connnection = TRUE;
    }
    if (object->request && (InternetCloseHandle(object->request) || GetLastError() == ERROR_IO_PENDING))
        object->request = NULL;
    if (object->session && (InternetCloseHandle(object->session) || GetLastError() == ERROR_IO_PENDING))
        object->session = NULL;
    if (wait)
    {
        while (object->request || object->session)
            SleepConditionVariableCS(&object->cv, &object->cs, INFINITE);
        object->closing_connnection = FALSE;
    }
    object->current_write_pos = 0;
    object->content_range_start = -1ULL;
    object->content_range_end = -1ULL;
}

static HRESULT open_connection(struct bytestream_http *object, ULONGLONG range_start, ULONGLONG range_end)
{
    WCHAR host[MAX_PATH], headers[1024], range[256];
    URL_COMPONENTSW uc;
    DWORD flags;

    close_connection(object, TRUE);

    range_start -= range_start % 1024;
    if (range_end != -1ULL) range_end -= 1;

    memset(&uc, 0, sizeof(uc));
    uc.dwStructSize      = sizeof(uc);
    uc.lpszHostName      = host;
    uc.dwHostNameLength  = ARRAY_SIZE(host);
    uc.dwUrlPathLength   = ~0U;
    uc.dwSchemeLength    = ~0U;
    if (!InternetCrackUrlW(object->url, 0, 0, &uc))
    {
        uc.lpszHostName = malloc(uc.dwHostNameLength * sizeof(WCHAR));
        if (!uc.lpszHostName)
            return E_OUTOFMEMORY;
        if (!InternetCrackUrlW(object->url, 0, 0, &uc))
            goto error;
    }

    object->session = InternetOpenW(L"NSPlayer/12.00.19041.4894 WMFSDK/12.00.19041.4894",
                                    INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC);
    if (!object->session)
        goto error;
    IMFByteStream_AddRef(&object->IMFByteStream_iface);
    InternetSetStatusCallbackW(object->session, progress_callback);

    flags = INTERNET_FLAG_RELOAD;
    if (object->secure)
        flags |= INTERNET_FLAG_SECURE;

    headers[0] = 0;
    wcscat(headers, L"GetContentFeatures.DLNA.ORG: 1\r\n");
    wcscat(headers, L"Accept: */*\r\n");

    if (range_end == -1ULL)
        swprintf(range, ARRAY_SIZE(range), L"Range: bytes=%I64u-", range_start);
    else
        swprintf(range, ARRAY_SIZE(range), L"Range: bytes=%I64u-%I64u", range_start, range_end);
    wcscat(headers, range);

    object->parsed_headers = FALSE;
    object->request_complete = FALSE;
    object->request = InternetOpenUrlW(object->session, object->url, headers, (DWORD)-1L, flags, (DWORD_PTR)object);
    if (!object->request)
    {
        if (GetLastError() != ERROR_IO_PENDING)
            goto error;
        else
        {
            while (!object->request && !object->error)
                SleepConditionVariableCS(&object->cv, &object->cs, INFINITE);
            if (object->error)
            {
                SetLastError(object->error);
                goto error;
            }

            while (!object->request_complete && !object->error)
                SleepConditionVariableCS(&object->cv, &object->cs, INFINITE);
            if (object->last_error)
            {
                SetLastError(object->last_error);
                goto error;
            }
        }
    }
    IMFByteStream_AddRef(&object->IMFByteStream_iface);

    while (!object->parsed_headers && !object->error)
        SleepConditionVariableCS(&object->cv, &object->cs, INFINITE);

    if (uc.lpszHostName != host)
        free(uc.lpszHostName);
    return S_OK;

error:
    TRACE("error: %p, %lx\n", object, GetLastError());

    handle_wininet_error(object, GetLastError());
    if (uc.lpszHostName != host)
        free(uc.lpszHostName);
    close_connection(object, TRUE);
    WakeAllConditionVariable(&object->cv);
    return object->error;
}

static WCHAR *recreate_url(const WCHAR *url)
{
    WCHAR scheme[MAX_PATH], host[MAX_PATH + 6], port_str[6];
    URL_COMPONENTSW uc;
    WCHAR *out = NULL;
    DWORD len;

    memset(&uc, 0, sizeof(uc));
    uc.dwStructSize      = sizeof(uc);
    uc.lpszScheme        = scheme;
    uc.dwSchemeLength    = ARRAY_SIZE(scheme);
    uc.lpszHostName      = host;
    uc.dwHostNameLength  = ARRAY_SIZE(host);
    uc.dwUrlPathLength   = ~0U;
    uc.dwSchemeLength    = ~0U;

    if (!InternetCrackUrlW(url, 0, 0, &uc))
    {
        uc.lpszScheme = malloc((uc.dwSchemeLength + 6) * sizeof(WCHAR));
        if (!uc.lpszScheme)
            goto error;
        uc.lpszHostName = malloc((uc.dwHostNameLength + 6) * sizeof(WCHAR));
        if (!uc.lpszHostName)
            goto error;

        if (!InternetCrackUrlW(url, 0, 0, &uc))
            goto error;
    }

    if (!wcscmp(uc.lpszScheme, L"httpd"))
    {
        uc.dwSchemeLength -= 1;
        uc.nPort = 80;
    }
    else if (!wcscmp(uc.lpszScheme, L"httpsd"))
    {
        uc.dwSchemeLength -= 1;
        uc.nPort = 443;
    }
    else if (!wcscmp(uc.lpszScheme, L"mms"))
    {
        wcscpy(scheme, L"http");
        uc.lpszScheme = scheme;
        uc.dwSchemeLength = wcslen(uc.lpszScheme);
        uc.nPort = 80;
    }

    swprintf(port_str, ARRAY_SIZE(port_str), L"%u", uc.nPort);
    wcscat(uc.lpszHostName, L":");
    wcscat(uc.lpszHostName, port_str);
    uc.dwHostNameLength += 1 + wcslen(port_str);

    len = uc.dwSchemeLength + 3 + uc.dwHostNameLength + uc.dwUrlPathLength + 1;
    out = malloc(len * sizeof(WCHAR));
    if (!out)
        goto error;

    if (!InternetCreateUrlW(&uc, 0, out, &len))
    {
        free(out);
        out = malloc(len * sizeof(WCHAR));
        if (!out)
            goto error;

        if (!InternetCreateUrlW(&uc, 0, out, &len))
            goto error;
    }
    return out;

error:
    free(out);
    if (uc.lpszHostName != host)
        free(uc.lpszHostName);
    if (uc.lpszScheme != scheme)
        free(uc.lpszScheme);
    return NULL;
}

HRESULT create_http_bytestream(const WCHAR *url, BOOL secure, void **out)
{
    struct bytestream_http *object;
    ULARGE_INTEGER ul = {0};
    FILETIME ft = {0};
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_w(url), out);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = MFCreateAttributes(&object->attributes, 0)))
    {
        free(object);
        return hr;
    }
    object->IMFByteStream_iface.lpVtbl = &bytestream_http_vtbl;
    object->IMFAttributes_iface.lpVtbl = &bytestream_http_attributes_vtbl;
    object->ref = 1;
    InitializeCriticalSection(&object->cs);
    InitializeConditionVariable(&object->cv);
    object->read_callback.lpVtbl = &bytestream_http_read_callback_vtbl;
    list_init(&object->pending);
    object->url = recreate_url(url);
    object->secure = secure;
    object->content_length = -1ULL;
    object->last_modified = -1ULL;
    object->content_range_end = -1ULL;
    list_init(&object->downloaded_ranges);

    if (!object->url)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    if (FAILED(hr = MFCreateTempFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE,
                                     &object->cache_file)))
    {
        ERR("Couldn't create temp file: %lx\n", hr);
        goto error;
    }

    EnterCriticalSection(&object->cs);

    if (FAILED(hr = open_connection(object, 0ULL, -1ULL)))
        goto error;

    while (!object->parsed_headers && !object->error)
        SleepConditionVariableCS(&object->cv, &object->cs, INFINITE);

    if (object->error)
    {
        hr = object->error;
        goto error;
    }

    IMFAttributes_SetString(object->attributes, &MF_BYTESTREAM_EFFECTIVE_URL, object->url);
    IMFAttributes_SetString(object->attributes, &MF_BYTESTREAM_CONTENT_TYPE,
                            object->content_type ? object->content_type : L"application/octet-stream");
    if (object->last_modified != -1ULL)
    {
        ul.QuadPart = object->last_modified;
        ft.dwLowDateTime = ul.u.LowPart;
        ft.dwHighDateTime = ul.u.HighPart;
        IMFAttributes_SetBlob(object->attributes, &MF_BYTESTREAM_LAST_MODIFIED_TIME, (void *)&ft, sizeof(ft));
    }

    LeaveCriticalSection(&object->cs);

    *out = &object->IMFByteStream_iface;
    return S_OK;

error:
    LeaveCriticalSection(&object->cs);
    IMFByteStream_Release(&object->IMFByteStream_iface);
    return hr;
}
