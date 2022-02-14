/*
 * Copyright 2020 Nikolay Sivov for CodeWeavers
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
#define NONAMELESSUNION

#include "mfapi.h"
#include "mfidl.h"
#include "mfreadwrite.h"
#include "mf_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct sink_writer
{
    IMFSinkWriter IMFSinkWriter_iface;
    LONG refcount;
};

static struct sink_writer *impl_from_IMFSinkWriter(IMFSinkWriter *iface)
{
    return CONTAINING_RECORD(iface, struct sink_writer, IMFSinkWriter_iface);
}

static HRESULT WINAPI sink_writer_QueryInterface(IMFSinkWriter *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFSinkWriter) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFSinkWriter_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sink_writer_AddRef(IMFSinkWriter *iface)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    ULONG refcount = InterlockedIncrement(&writer->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sink_writer_Release(IMFSinkWriter *iface)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    ULONG refcount = InterlockedDecrement(&writer->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    if (!refcount)
    {
        free(writer);
    }

    return refcount;
}

static HRESULT WINAPI sink_writer_AddStream(IMFSinkWriter *iface, IMFMediaType *type, DWORD *index)
{
    FIXME("%p, %p, %p.\n", iface, type, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_SetInputMediaType(IMFSinkWriter *iface, DWORD index, IMFMediaType *type,
        IMFAttributes *parameters)
{
    FIXME("%p, %lu, %p, %p.\n", iface, index, type, parameters);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_BeginWriting(IMFSinkWriter *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_WriteSample(IMFSinkWriter *iface, DWORD index, IMFSample *sample)
{
    FIXME("%p, %lu, %p.\n", iface, index, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_SendStreamTick(IMFSinkWriter *iface, DWORD index, LONGLONG timestamp)
{
    FIXME("%p, %lu, %s.\n", iface, index, wine_dbgstr_longlong(timestamp));

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_PlaceMarker(IMFSinkWriter *iface, DWORD index, void *context)
{
    FIXME("%p, %lu, %p.\n", iface, index, context);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_NotifyEndOfSegment(IMFSinkWriter *iface, DWORD index)
{
    FIXME("%p, %lu.\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_Flush(IMFSinkWriter *iface, DWORD index)
{
    FIXME("%p, %lu.\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_Finalize(IMFSinkWriter *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_GetServiceForStream(IMFSinkWriter *iface, DWORD index, REFGUID service,
        REFIID riid, void **object)
{
    FIXME("%p, %lu, %s, %s, %p.\n", iface, index, debugstr_guid(service), debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_GetStatistics(IMFSinkWriter *iface, DWORD index, MF_SINK_WRITER_STATISTICS *stats)
{
    FIXME("%p, %lu, %p.\n", iface, index, stats);

    return E_NOTIMPL;
}

static const IMFSinkWriterVtbl sink_writer_vtbl =
{
    sink_writer_QueryInterface,
    sink_writer_AddRef,
    sink_writer_Release,
    sink_writer_AddStream,
    sink_writer_SetInputMediaType,
    sink_writer_BeginWriting,
    sink_writer_WriteSample,
    sink_writer_SendStreamTick,
    sink_writer_PlaceMarker,
    sink_writer_NotifyEndOfSegment,
    sink_writer_Flush,
    sink_writer_Finalize,
    sink_writer_GetServiceForStream,
    sink_writer_GetStatistics,
};

HRESULT create_sink_writer_from_sink(IMFMediaSink *sink, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    struct sink_writer *object;
    HRESULT hr;

    object = malloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSinkWriter_iface.lpVtbl = &sink_writer_vtbl;
    object->refcount = 1;

    hr = IMFSinkWriter_QueryInterface(&object->IMFSinkWriter_iface, riid, out);
    IMFSinkWriter_Release(&object->IMFSinkWriter_iface);
    return hr;
}

HRESULT create_sink_writer_from_stream(IMFByteStream *stream, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    struct sink_writer *object;
    HRESULT hr;

    object = malloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSinkWriter_iface.lpVtbl = &sink_writer_vtbl;
    object->refcount = 1;

    hr = IMFSinkWriter_QueryInterface(&object->IMFSinkWriter_iface, riid, out);
    IMFSinkWriter_Release(&object->IMFSinkWriter_iface);
    return hr;
}

/***********************************************************************
 *      MFCreateSinkWriterFromMediaSink (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSinkWriterFromMediaSink(IMFMediaSink *sink, IMFAttributes *attributes, IMFSinkWriter **writer)
{
    TRACE("%p, %p, %p.\n", sink, attributes, writer);

    return create_sink_writer_from_sink(sink, attributes, &IID_IMFSinkWriter, (void **)writer);
}

/***********************************************************************
 *      MFCreateSinkWriterFromURL (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSinkWriterFromURL(const WCHAR *url, IMFByteStream *bytestream, IMFAttributes *attributes,
        IMFSinkWriter **writer)
{
    FIXME("%s, %p, %p, %p.\n", debugstr_w(url), bytestream, attributes, writer);

    return E_NOTIMPL;
}
