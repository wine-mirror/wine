/*
 * Speech API (SAPI) automation implementation.
 *
 * Copyright 2019 Jactry Zeng for CodeWeavers
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

#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

struct file_stream
{
    ISpeechFileStream ISpeechFileStream_iface;
    ISpStream ISpStream_iface;
    LONG ref;
};

static inline struct file_stream *impl_from_ISpeechFileStream(ISpeechFileStream *iface)
{
    return CONTAINING_RECORD(iface, struct file_stream, ISpeechFileStream_iface);
}

static inline struct file_stream *impl_from_ISpStream(ISpStream *iface)
{
    return CONTAINING_RECORD(iface, struct file_stream, ISpStream_iface);
}

/* ISpeechFileStream interface */
static HRESULT WINAPI file_stream_QueryInterface(ISpeechFileStream *iface, REFIID iid, void **obj)
{
    struct file_stream *This = impl_from_ISpeechFileStream(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(iid), obj);

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IDispatch) ||
        IsEqualIID(iid, &IID_ISpeechBaseStream) ||
        IsEqualIID(iid, &IID_ISpeechFileStream))
        *obj = &This->ISpeechFileStream_iface;
    else if (IsEqualIID(iid, &IID_ISpStream))
        *obj = &This->ISpStream_iface;
    else
    {
        *obj = NULL;
        FIXME("interface %s not implemented.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI file_stream_AddRef(ISpeechFileStream *iface)
{
    struct file_stream *This = impl_from_ISpeechFileStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI file_stream_Release(ISpeechFileStream *iface)
{
    struct file_stream *This = impl_from_ISpeechFileStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    if (!ref)
    {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI file_stream_GetTypeInfoCount(ISpeechFileStream *iface, UINT *info)
{
    FIXME("(%p, %p): stub.\n", iface, info);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_GetTypeInfo(ISpeechFileStream *iface, UINT info, LCID lcid,
                                              ITypeInfo **type_info)
{
    FIXME("(%p, %u, %lu, %p): stub.\n", iface, info, lcid, type_info);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_GetIDsOfNames(ISpeechFileStream *iface, REFIID riid, LPOLESTR *names,
                                                UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("(%p, %s, %p, %u, %lu, %p): stub.\n", iface, debugstr_guid(riid), names, count, lcid, dispid);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_Invoke(ISpeechFileStream *iface, DISPID dispid, REFIID riid, LCID lcid,
                                         WORD flags, DISPPARAMS *params, VARIANT *result,
                                         EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("(%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p): stub.\n", iface, dispid, debugstr_guid(riid),
          lcid, flags, params, result, excepinfo, argerr);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_get_Format(ISpeechFileStream *iface, ISpeechAudioFormat **format)
{
    FIXME("(%p, %p): stub.\n", iface, format);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_putref_Format(ISpeechFileStream *iface, ISpeechAudioFormat *format)
{
    FIXME("(%p, %p): stub.\n", iface, format);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_Read(ISpeechFileStream *iface, VARIANT *buffer, LONG written, LONG *read)
{
    FIXME("(%p, %p, %ld, %p): stub.\n", iface, buffer, written, read);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_Write(ISpeechFileStream *iface, VARIANT buffer, LONG *written)
{
    FIXME("(%p, %s, %p): stub.\n", iface, debugstr_variant(&buffer), written);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_Seek(ISpeechFileStream *iface, VARIANT position, SpeechStreamSeekPositionType origin,
                                       VARIANT *pos)
{
    FIXME("(%p, %s, %d, %p): stub.\n", iface, debugstr_variant(&position), origin, pos);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_Open(ISpeechFileStream *iface, BSTR filename, SpeechStreamFileMode mode, VARIANT_BOOL events)
{
    FIXME("(%p, %s, %d, %d): stub.\n", iface, debugstr_w(filename), mode, events);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_Close(ISpeechFileStream *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

const static ISpeechFileStreamVtbl file_stream_vtbl =
{
    file_stream_QueryInterface,
    file_stream_AddRef,
    file_stream_Release,
    file_stream_GetTypeInfoCount,
    file_stream_GetTypeInfo,
    file_stream_GetIDsOfNames,
    file_stream_Invoke,
    file_stream_get_Format,
    file_stream_putref_Format,
    file_stream_Read,
    file_stream_Write,
    file_stream_Seek,
    file_stream_Open,
    file_stream_Close
};

/* ISpStream interface */
static HRESULT WINAPI spstream_QueryInterface(ISpStream *iface, REFIID iid, void **obj)
{
    struct file_stream *This = impl_from_ISpStream(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(iid), obj);

    return ISpeechFileStream_QueryInterface(&This->ISpeechFileStream_iface, iid, obj);
}

static ULONG WINAPI spstream_AddRef(ISpStream *iface)
{
    struct file_stream *This = impl_from_ISpStream(iface);

    TRACE("(%p).\n", iface);

    return ISpeechFileStream_AddRef(&This->ISpeechFileStream_iface);
}

static ULONG WINAPI spstream_Release(ISpStream *iface)
{
    struct file_stream *This = impl_from_ISpStream(iface);

    TRACE("(%p).\n", iface);

    return ISpeechFileStream_Release(&This->ISpeechFileStream_iface);
}

static HRESULT WINAPI spstream_Read(ISpStream *iface, void *pv, ULONG cb, ULONG *read)
{
    FIXME("(%p, %p, %ld, %p): stub.\n", iface, pv, cb, read);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_Write(ISpStream *iface, const void *pv, ULONG cb, ULONG *written)
{
    FIXME("(%p, %p, %ld, %p): stub.\n", iface, pv, cb, written);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_Seek(ISpStream *iface, LARGE_INTEGER mode, DWORD origin, ULARGE_INTEGER *position)
{
    FIXME("(%p, %s, %ld, %p): stub.\n", iface, wine_dbgstr_longlong(mode.QuadPart), origin, position);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_SetSize(ISpStream *iface, ULARGE_INTEGER size)
{
    FIXME("(%p, %s): stub.\n", iface, wine_dbgstr_longlong(size.QuadPart));

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_CopyTo(ISpStream *iface, IStream *stream, ULARGE_INTEGER cb,
                                      ULARGE_INTEGER *read, ULARGE_INTEGER *written)
{
    FIXME("(%p, %p, %s, %p, %p): stub.\n", iface, stream, wine_dbgstr_longlong(cb.QuadPart),
          read, written);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_Commit(ISpStream *iface, DWORD flag)
{
    FIXME("(%p, %ld): stub.\n", iface, flag);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_Revert(ISpStream *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_LockRegion(ISpStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER cb, DWORD type)
{
    FIXME("(%p, %s, %s, %ld): stub.\n", iface, wine_dbgstr_longlong(offset.QuadPart),
          wine_dbgstr_longlong(cb.QuadPart), type);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_UnlockRegion(ISpStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER cb, DWORD type)
{
    FIXME("(%p, %s, %s, %ld): stub.\n", iface, wine_dbgstr_longlong(offset.QuadPart),
          wine_dbgstr_longlong(cb.QuadPart), type);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_Stat(ISpStream *iface, STATSTG *statstg, DWORD flag)
{
    FIXME("(%p, %p, %ld): stub.\n", iface, statstg, flag);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_Clone(ISpStream *iface, IStream **stream)
{
    FIXME("(%p, %p): stub.\n", iface, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_GetFormat(ISpStream *iface, GUID *format, WAVEFORMATEX **wave)
{
    FIXME("(%p, %p, %p): stub.\n", iface, format, wave);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_SetBaseStream(ISpStream *iface, IStream *stream, REFGUID format,
                                             const WAVEFORMATEX *wave)
{
    FIXME("(%p, %p, %s, %p): stub.\n", iface, stream, debugstr_guid(format), wave);

    return E_NOTIMPL;
}

static HRESULT WINAPI spstream_GetBaseStream(ISpStream *iface, IStream **stream)
{
    FIXME("(%p, %p): stub.\n", iface, stream);

    return E_NOTIMPL;
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
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
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

HRESULT file_stream_create(IUnknown *outer, REFIID iid, void **obj)
{
    struct file_stream *This = malloc(sizeof(*This));
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->ISpeechFileStream_iface.lpVtbl = &file_stream_vtbl;
    This->ISpStream_iface.lpVtbl = &spstream_vtbl;
    This->ref = 1;

    hr = ISpeechFileStream_QueryInterface(&This->ISpeechFileStream_iface, iid, obj);

    ISpeechFileStream_Release(&This->ISpeechFileStream_iface);
    return hr;
}
