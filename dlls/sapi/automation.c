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
    LONG ref;
};

static inline struct file_stream *impl_from_ISpeechFileStream(ISpeechFileStream *iface)
{
    return CONTAINING_RECORD(iface, struct file_stream, ISpeechFileStream_iface);
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

    TRACE("(%p): ref=%u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI file_stream_Release(ISpeechFileStream *iface)
{
    struct file_stream *This = impl_from_ISpeechFileStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%u.\n", iface, ref);

    if (!ref)
    {
        heap_free(This);
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
    FIXME("(%p, %u, %u, %p): stub.\n", iface, info, lcid, type_info);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_GetIDsOfNames(ISpeechFileStream *iface, REFIID riid, LPOLESTR *names,
                                                UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("(%p, %s, %p, %u, %u, %p): stub.\n", iface, debugstr_guid(riid), names, count, lcid, dispid);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_stream_Invoke(ISpeechFileStream *iface, DISPID dispid, REFIID riid, LCID lcid,
                                         WORD flags, DISPPARAMS *params, VARIANT *result,
                                         EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("(%p, %d, %s, %#x, %#x, %p, %p, %p, %p): stub.\n", iface, dispid, debugstr_guid(riid),
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
    FIXME("(%p, %p, %d, %p): stub.\n", iface, buffer, written, read);

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

HRESULT file_stream_create(IUnknown *outer, REFIID iid, void **obj)
{
    struct file_stream *This = heap_alloc(sizeof(*This));
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->ISpeechFileStream_iface.lpVtbl = &file_stream_vtbl;
    This->ref = 1;

    hr = ISpeechFileStream_QueryInterface(&This->ISpeechFileStream_iface, iid, obj);

    ISpeechFileStream_Release(&This->ISpeechFileStream_iface);
    return hr;
}
