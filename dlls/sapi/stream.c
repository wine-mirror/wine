/*
 * Speech API (SAPI) stream implementation.
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "sapiddk.h"
#include "sperror.h"

#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

struct spstream
{
    ISpStream ISpStream_iface;
    LONG ref;

    IStream *base_stream;
    GUID format;
    WAVEFORMATEX *wfx;
    BOOL closed;
};

static inline struct spstream *impl_from_ISpStream(ISpStream *iface)
{
    return CONTAINING_RECORD(iface, struct spstream, ISpStream_iface);
}

static HRESULT WINAPI spstream_QueryInterface(ISpStream *iface, REFIID iid, void **obj)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(iid), obj);

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_ISequentialStream) ||
        IsEqualIID(iid, &IID_IStream) ||
        IsEqualIID(iid, &IID_ISpStreamFormat) ||
        IsEqualIID(iid, &IID_ISpStream))
        *obj = &This->ISpStream_iface;
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI spstream_AddRef(ISpStream *iface)
{
    struct spstream *This = impl_from_ISpStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI spstream_Release(ISpStream *iface)
{
    struct spstream *This = impl_from_ISpStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    if (!ref)
    {
        if (This->base_stream) IStream_Release(This->base_stream);
        free(This->wfx);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI spstream_Read(ISpStream *iface, void *pv, ULONG cb, ULONG *read)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %p, %ld, %p).\n", iface, pv, cb, read);

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    return IStream_Read(This->base_stream, pv, cb, read);
}

static HRESULT WINAPI spstream_Write(ISpStream *iface, const void *pv, ULONG cb, ULONG *written)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %p, %ld, %p).\n", iface, pv, cb, written);

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    return IStream_Write(This->base_stream, pv, cb, written);
}

static HRESULT WINAPI spstream_Seek(ISpStream *iface, LARGE_INTEGER mode, DWORD origin, ULARGE_INTEGER *position)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %s, %ld, %p).\n", iface, wine_dbgstr_longlong(mode.QuadPart), origin, position);

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    return IStream_Seek(This->base_stream, mode, origin, position);
}

static HRESULT WINAPI spstream_SetSize(ISpStream *iface, ULARGE_INTEGER size)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %s).\n", iface, wine_dbgstr_longlong(size.QuadPart));

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    return IStream_SetSize(This->base_stream, size);
}

static HRESULT WINAPI spstream_CopyTo(ISpStream *iface, IStream *stream, ULARGE_INTEGER cb,
                                      ULARGE_INTEGER *read, ULARGE_INTEGER *written)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %p, %s, %p, %p).\n", iface, stream, wine_dbgstr_longlong(cb.QuadPart), read, written);

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    return IStream_CopyTo(This->base_stream, stream, cb, read, written);
}

static HRESULT WINAPI spstream_Commit(ISpStream *iface, DWORD flag)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %ld).\n", iface, flag);

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    return IStream_Commit(This->base_stream, flag);
}

static HRESULT WINAPI spstream_Revert(ISpStream *iface)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p).\n", iface);

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    return IStream_Revert(This->base_stream);
}

static HRESULT WINAPI spstream_LockRegion(ISpStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER cb, DWORD type)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %s, %s, %ld).\n", iface, wine_dbgstr_longlong(offset.QuadPart),
          wine_dbgstr_longlong(cb.QuadPart), type);

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    return IStream_LockRegion(This->base_stream, offset, cb, type);
}

static HRESULT WINAPI spstream_UnlockRegion(ISpStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER cb, DWORD type)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %s, %s, %ld).\n", iface, wine_dbgstr_longlong(offset.QuadPart),
          wine_dbgstr_longlong(cb.QuadPart), type);

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    return IStream_UnlockRegion(This->base_stream, offset, cb, type);
}

static HRESULT WINAPI spstream_Stat(ISpStream *iface, STATSTG *statstg, DWORD flag)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %p, %ld).\n", iface, statstg, flag);

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    return IStream_Stat(This->base_stream, statstg, flag);
}

static HRESULT WINAPI spstream_Clone(ISpStream *iface, IStream **stream)
{
    TRACE("(%p, %p).\n", iface, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_GetFormat(ISpStream *iface, GUID *format, WAVEFORMATEX **wfx)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %p, %p).\n", iface, format, wfx);

    if (!format)
        return E_POINTER;

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    if (This->wfx)
    {
        if (!wfx)
            return E_POINTER;
        if (!(*wfx = CoTaskMemAlloc(sizeof(WAVEFORMATEX) + This->wfx->cbSize)))
            return E_OUTOFMEMORY;
        memcpy(*wfx, This->wfx, sizeof(WAVEFORMATEX) + This->wfx->cbSize);
    }

    *format = This->format;

    return S_OK;
}

static HRESULT WINAPI spstream_SetBaseStream(ISpStream *iface, IStream *stream, REFGUID format,
                                             const WAVEFORMATEX *wfx)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %p, %s, %p).\n", iface, stream, debugstr_guid(format), wfx);

    if (!stream || !format)
        return E_INVALIDARG;

    if (This->base_stream || This->closed)
        return SPERR_ALREADY_INITIALIZED;

    This->format = *format;
    if (IsEqualGUID(format, &SPDFID_WaveFormatEx))
    {
        if (!wfx)
            return E_INVALIDARG;
        if (!(This->wfx = malloc(sizeof(WAVEFORMATEX) + wfx->cbSize)))
            return E_OUTOFMEMORY;
        memcpy(This->wfx, wfx, sizeof(WAVEFORMATEX) + wfx->cbSize);
    }

    IStream_AddRef(stream);
    This->base_stream = stream;
    return S_OK;
}

static HRESULT WINAPI spstream_GetBaseStream(ISpStream *iface, IStream **stream)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %p).\n", iface, stream);

    if (!stream)
        return E_INVALIDARG;

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    *stream = This->base_stream;
    if (*stream)
        IStream_AddRef(*stream);
    return S_OK;
}

static HRESULT WINAPI spstream_BindToFile(ISpStream *iface, LPCWSTR filename, SPFILEMODE mode,
                                          const GUID *format, const WAVEFORMATEX* wave,
                                          ULONGLONG interest)
{
    FIXME("(%p, %s, %d, %s, %p, %s): stub.\n", iface, debugstr_w(filename), mode, debugstr_guid(format),
          wave, wine_dbgstr_longlong(interest));

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_Close(ISpStream *iface)
{
    struct spstream *This = impl_from_ISpStream(iface);

    TRACE("(%p).\n", iface);

    if (This->closed)
        return SPERR_STREAM_CLOSED;
    else if (!This->base_stream)
        return SPERR_UNINITIALIZED;

    IStream_Release(This->base_stream);
    This->base_stream = NULL;
    This->closed = TRUE;
    return S_OK;
}

const static ISpStreamVtbl spstream_vtbl =
{
    spstream_QueryInterface,
    spstream_AddRef,
    spstream_Release,
    spstream_Read,
    spstream_Write,
    spstream_Seek,
    spstream_SetSize,
    spstream_CopyTo,
    spstream_Commit,
    spstream_Revert,
    spstream_LockRegion,
    spstream_UnlockRegion,
    spstream_Stat,
    spstream_Clone,
    spstream_GetFormat,
    spstream_SetBaseStream,
    spstream_GetBaseStream,
    spstream_BindToFile,
    spstream_Close
};

HRESULT speech_stream_create(IUnknown *outer, REFIID iid, void **obj)
{
    struct spstream *This = malloc(sizeof(*This));
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->ISpStream_iface.lpVtbl = &spstream_vtbl;
    This->ref = 1;

    This->base_stream = NULL;
    This->format = GUID_NULL;
    This->wfx = NULL;
    This->closed = FALSE;

    hr = ISpStream_QueryInterface(&This->ISpStream_iface, iid, obj);

    ISpStream_Release(&This->ISpStream_iface);
    return hr;
}
