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
#include "pathcch.h"
#include "wine/mfinternal.h"
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

/***********************************************************************
 *      MFCreateSinkWriterFromMediaSink (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSinkWriterFromMediaSink(IMFMediaSink *sink, IMFAttributes *attributes, IMFSinkWriter **writer)
{
    TRACE("%p, %p, %p.\n", sink, attributes, writer);

    return create_sink_writer_from_sink(sink, attributes, &IID_IMFSinkWriter, (void **)writer);
}

static HRESULT sink_writer_get_sink_factory_class(const WCHAR *url, IMFAttributes *attributes, CLSID *clsid)
{
    static const struct extension_map
    {
        const WCHAR *ext;
        const GUID *guid;
    } ext_map[] =
    {
        { L".mp4", &MFTranscodeContainerType_MPEG4 },
        { L".mp3", &MFTranscodeContainerType_MP3 },
        { L".wav", &MFTranscodeContainerType_WAVE },
        { L".avi", &MFTranscodeContainerType_AVI },
    };
    static const struct
    {
        const GUID *container;
        const CLSID *clsid;
    } class_map[] =
    {
        { &MFTranscodeContainerType_MPEG4, &CLSID_MFMPEG4SinkClassFactory },
        { &MFTranscodeContainerType_MP3, &CLSID_MFMP3SinkClassFactory },
        { &MFTranscodeContainerType_WAVE, &CLSID_MFWAVESinkClassFactory },
        { &MFTranscodeContainerType_AVI, &CLSID_MFAVISinkClassFactory },
    };
    const WCHAR *extension;
    GUID container;
    unsigned int i;

    if (url)
    {
        if (!attributes || FAILED(IMFAttributes_GetGUID(attributes, &MF_TRANSCODE_CONTAINERTYPE, &container)))
        {
            const struct extension_map *map = NULL;

            if (FAILED(PathCchFindExtension(url, PATHCCH_MAX_CCH, &extension))) return E_INVALIDARG;
            if (!extension || !*extension) return E_INVALIDARG;

            for (i = 0; i < ARRAY_SIZE(ext_map); ++i)
            {
                map = &ext_map[i];

                if (!wcsicmp(map->ext, extension))
                    break;
            }

            if (!map)
            {
                WARN("Couldn't find container type for extension %s.\n", debugstr_w(extension));
                return E_INVALIDARG;
            }

            container = *map->guid;
        }
    }
    else
    {
        if (!attributes) return E_INVALIDARG;
        if (FAILED(IMFAttributes_GetGUID(attributes, &MF_TRANSCODE_CONTAINERTYPE, &container))) return E_INVALIDARG;
    }

    for (i = 0; i < ARRAY_SIZE(class_map); ++i)
    {
        if (IsEqualGUID(&container, &class_map[i].container))
        {
            *clsid = *class_map[i].clsid;
            return S_OK;
        }
    }

    WARN("Couldn't find factory class for container %s.\n", debugstr_guid(&container));
    return E_INVALIDARG;
}

HRESULT create_sink_writer_from_url(const WCHAR *url, IMFByteStream *bytestream, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    IMFSinkClassFactory *factory;
    IMFMediaSink *sink;
    CLSID clsid;
    HRESULT hr;

    if (!url && !bytestream)
        return E_INVALIDARG;

    if (FAILED(hr = sink_writer_get_sink_factory_class(url, attributes, &clsid))) return hr;

    if (FAILED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFSinkClassFactory, (void **)&factory)))
    {
        WARN("Failed to create a sink factory, hr %#lx.\n", hr);
        return hr;
    }

    if (bytestream)
        IMFByteStream_AddRef(bytestream);
    else if (FAILED(hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_DELETE_IF_EXIST, 0, url, &bytestream)))
    {
        WARN("Failed to create output file stream, hr %#lx.\n", hr);
        IMFSinkClassFactory_Release(factory);
        return hr;
    }

    hr = IMFSinkClassFactory_CreateMediaSink(factory, bytestream, NULL, NULL, &sink);
    IMFSinkClassFactory_Release(factory);
    IMFByteStream_Release(bytestream);
    if (FAILED(hr))
    {
        WARN("Failed to create a sink, hr %#lx.\n", hr);
        return hr;
    }

    hr = create_sink_writer_from_sink(sink, attributes, riid, out);
    IMFMediaSink_Release(sink);

    return hr;
}

/***********************************************************************
 *      MFCreateSinkWriterFromURL (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSinkWriterFromURL(const WCHAR *url, IMFByteStream *bytestream, IMFAttributes *attributes,
        IMFSinkWriter **writer)
{
    TRACE("%s, %p, %p, %p.\n", debugstr_w(url), bytestream, attributes, writer);

    return create_sink_writer_from_url(url, bytestream, attributes, &IID_IMFSinkWriter, (void **)writer);
}
