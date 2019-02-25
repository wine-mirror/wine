/*
 *
 * Copyright 2014 Austin English
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
#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "mfidl.h"
#include "mfreadwrite.h"

#include "wine/debug.h"
#include "wine/heap.h"

DEFINE_GUID(CLSID_MFReadWriteClassFactory, 0x48e2ed0f, 0x98c2, 0x4a37, 0xbe, 0xd5, 0x16, 0x63, 0x12, 0xdd, 0xd8, 0x3f);
DEFINE_GUID(CLSID_MFSourceReader, 0x1777133c, 0x0881, 0x411b, 0xa5, 0x77, 0xad, 0x54, 0x5f, 0x07, 0x14, 0xc4);
DEFINE_GUID(CLSID_MFSinkWriter, 0xa3bbfb17, 0x8273, 0x4e52, 0x9e, 0x0e, 0x97, 0x39, 0xdc, 0x88, 0x79, 0x90);

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static HINSTANCE mfinstance;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            mfinstance = instance;
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( mfinstance );
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( mfinstance );
}

typedef struct _srcreader
{
    IMFSourceReader IMFSourceReader_iface;
    LONG ref;
} srcreader;

struct sink_writer
{
    IMFSinkWriter IMFSinkWriter_iface;
    LONG refcount;
};

static inline srcreader *impl_from_IMFSourceReader(IMFSourceReader *iface)
{
    return CONTAINING_RECORD(iface, srcreader, IMFSourceReader_iface);
}

static inline struct sink_writer *impl_from_IMFSinkWriter(IMFSinkWriter *iface)
{
    return CONTAINING_RECORD(iface, struct sink_writer, IMFSinkWriter_iface);
}

static HRESULT WINAPI src_reader_QueryInterface(IMFSourceReader *iface, REFIID riid, void **out)
{
    srcreader *This = impl_from_IMFSourceReader(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFSourceReader))
    {
        *out = &This->IMFSourceReader_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI src_reader_AddRef(IMFSourceReader *iface)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI src_reader_Release(IMFSourceReader *iface)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI src_reader_GetStreamSelection(IMFSourceReader *iface, DWORD index, BOOL *selected)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %p\n", This, index, selected);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_SetStreamSelection(IMFSourceReader *iface, DWORD index, BOOL selected)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %d\n", This, index, selected);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_GetNativeMediaType(IMFSourceReader *iface, DWORD index, DWORD typeindex,
            IMFMediaType **type)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %d, %p\n", This, index, typeindex, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_GetCurrentMediaType(IMFSourceReader *iface, DWORD index, IMFMediaType **type)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %p\n", This, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_SetCurrentMediaType(IMFSourceReader *iface, DWORD index, DWORD *reserved,
        IMFMediaType *type)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %p, %p\n", This, index, reserved, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_SetCurrentPosition(IMFSourceReader *iface, REFGUID format, REFPROPVARIANT position)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, %s, %p\n", This, debugstr_guid(format), position);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_ReadSample(IMFSourceReader *iface, DWORD index,
        DWORD flags, DWORD *actualindex, DWORD *sampleflags, LONGLONG *timestamp,
        IMFSample **sample)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, 0x%08x, %p, %p, %p, %p\n", This, index, flags, actualindex,
          sampleflags, timestamp, sample);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_Flush(IMFSourceReader *iface, DWORD index)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_GetServiceForStream(IMFSourceReader *iface, DWORD index, REFGUID service,
        REFIID riid, void **object)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %s, %s, %p\n", This, index, debugstr_guid(service), debugstr_guid(riid), object);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_GetPresentationAttribute(IMFSourceReader *iface, DWORD index,
        REFGUID guid, PROPVARIANT *attr)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %s, %p\n", This, index, debugstr_guid(guid), attr);
    return E_NOTIMPL;
}

struct IMFSourceReaderVtbl srcreader_vtbl =
{
    src_reader_QueryInterface,
    src_reader_AddRef,
    src_reader_Release,
    src_reader_GetStreamSelection,
    src_reader_SetStreamSelection,
    src_reader_GetNativeMediaType,
    src_reader_GetCurrentMediaType,
    src_reader_SetCurrentMediaType,
    src_reader_SetCurrentPosition,
    src_reader_ReadSample,
    src_reader_Flush,
    src_reader_GetServiceForStream,
    src_reader_GetPresentationAttribute
};

static HRESULT create_source_reader_from_source(IMFMediaSource *source, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    srcreader *object;
    HRESULT hr;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSourceReader_iface.lpVtbl = &srcreader_vtbl;
    object->ref = 1;

    hr = IMFSourceReader_QueryInterface(&object->IMFSourceReader_iface, riid, out);
    IMFSourceReader_Release(&object->IMFSourceReader_iface);
    return hr;
}

static HRESULT create_source_reader_from_stream(IMFByteStream *stream, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    /* FIXME: resolve bytestream to media source */

    return create_source_reader_from_source(NULL, attributes, riid, out);
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

    TRACE("%p, %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sink_writer_Release(IMFSinkWriter *iface)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    ULONG refcount = InterlockedDecrement(&writer->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    if (!refcount)
    {
        heap_free(writer);
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
    FIXME("%p, %u, %p, %p.\n", iface, index, type, parameters);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_BeginWriting(IMFSinkWriter *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_WriteSample(IMFSinkWriter *iface, DWORD index, IMFSample *sample)
{
    FIXME("%p, %u, %p.\n", iface, index, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_SendStreamTick(IMFSinkWriter *iface, DWORD index, LONGLONG timestamp)
{
    FIXME("%p, %u, %s.\n", iface, index, wine_dbgstr_longlong(timestamp));

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_PlaceMarker(IMFSinkWriter *iface, DWORD index, void *context)
{
    FIXME("%p, %u, %p.\n", iface, index, context);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_NotifyEndOfSegment(IMFSinkWriter *iface, DWORD index)
{
    FIXME("%p, %u.\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_Flush(IMFSinkWriter *iface, DWORD index)
{
    FIXME("%p, %u.\n", iface, index);

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
    FIXME("%p, %u, %s, %s, %p.\n", iface, index, debugstr_guid(service), debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_GetStatistics(IMFSinkWriter *iface, DWORD index, MF_SINK_WRITER_STATISTICS *stats)
{
    FIXME("%p, %u, %p.\n", iface, index, stats);

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

static HRESULT create_sink_writer_from_sink(IMFMediaSink *sink, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    struct sink_writer *object;
    HRESULT hr;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSinkWriter_iface.lpVtbl = &sink_writer_vtbl;
    object->refcount = 1;

    hr = IMFSinkWriter_QueryInterface(&object->IMFSinkWriter_iface, riid, out);
    IMFSinkWriter_Release(&object->IMFSinkWriter_iface);
    return hr;
}

static HRESULT create_sink_writer_from_stream(IMFByteStream *stream, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    struct sink_writer *object;
    HRESULT hr;

    object = heap_alloc(sizeof(*object));
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
 *      MFCreateSourceReaderFromByteStream (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSourceReaderFromByteStream(IMFByteStream *stream, IMFAttributes *attributes,
        IMFSourceReader **reader)
{
    TRACE("%p, %p, %p.\n", stream, attributes, reader);

    return create_source_reader_from_stream(stream, attributes, &IID_IMFSourceReader, (void **)reader);
}

/***********************************************************************
 *      MFCreateSourceReaderFromMediaSource (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSourceReaderFromMediaSource(IMFMediaSource *source, IMFAttributes *attributes,
                                                   IMFSourceReader **reader)
{
    TRACE("%p, %p, %p.\n", source, attributes, reader);

    return create_source_reader_from_source(source, attributes, &IID_IMFSourceReader, (void **)reader);
}

static HRESULT WINAPI readwrite_factory_QueryInterface(IMFReadWriteClassFactory *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFReadWriteClassFactory) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFReadWriteClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI readwrite_factory_AddRef(IMFReadWriteClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI readwrite_factory_Release(IMFReadWriteClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI readwrite_factory_CreateInstanceFromURL(IMFReadWriteClassFactory *iface, REFCLSID clsid,
        const WCHAR *url, IMFAttributes *attributes, REFIID riid, void **out)
{
    FIXME("%s, %s, %p, %s, %p.\n", debugstr_guid(clsid), debugstr_w(url), attributes, debugstr_guid(riid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI readwrite_factory_CreateInstanceFromObject(IMFReadWriteClassFactory *iface, REFCLSID clsid,
        IUnknown *unk, IMFAttributes *attributes, REFIID riid, void **out)
{
    HRESULT hr;

    TRACE("%s, %p, %p, %s, %p.\n", debugstr_guid(clsid), unk, attributes, debugstr_guid(riid), out);

    if (IsEqualGUID(clsid, &CLSID_MFSourceReader))
    {
        IMFMediaSource *source = NULL;
        IMFByteStream *stream = NULL;

        hr = IUnknown_QueryInterface(unk, &IID_IMFByteStream, (void **)&stream);
        if (FAILED(hr))
            hr = IUnknown_QueryInterface(unk, &IID_IMFMediaSource, (void **)&source);

        if (stream)
            hr = create_source_reader_from_stream(stream, attributes, riid, out);
        else if (source)
            hr = create_source_reader_from_source(source, attributes, riid, out);

        if (source)
            IMFMediaSource_Release(source);
        if (stream)
            IMFByteStream_Release(stream);

        return hr;
    }
    else if (IsEqualGUID(clsid, &CLSID_MFSinkWriter))
    {
        IMFByteStream *stream = NULL;
        IMFMediaSink *sink = NULL;

        hr = IUnknown_QueryInterface(unk, &IID_IMFByteStream, (void **)&stream);
        if (FAILED(hr))
            hr = IUnknown_QueryInterface(unk, &IID_IMFMediaSink, (void **)&sink);

        if (stream)
            hr = create_sink_writer_from_stream(stream, attributes, riid, out);
        else if (sink)
            hr = create_sink_writer_from_sink(sink, attributes, riid, out);

        if (sink)
            IMFMediaSink_Release(sink);
        if (stream)
            IMFByteStream_Release(stream);

        return hr;
    }
    else
    {
        WARN("Unsupported class %s.\n", debugstr_guid(clsid));
        *out = NULL;
        return E_FAIL;
    }
}

static const IMFReadWriteClassFactoryVtbl readwrite_factory_vtbl =
{
    readwrite_factory_QueryInterface,
    readwrite_factory_AddRef,
    readwrite_factory_Release,
    readwrite_factory_CreateInstanceFromURL,
    readwrite_factory_CreateInstanceFromObject,
};

static IMFReadWriteClassFactory readwrite_factory = { &readwrite_factory_vtbl };

static HRESULT WINAPI classfactory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    TRACE("%s, %p.\n", debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IClassFactory) ||
            IsEqualGUID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("interface %s not implemented\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI classfactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI classfactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI classfactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", outer, debugstr_guid(riid), out);

    *out = NULL;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return IMFReadWriteClassFactory_QueryInterface(&readwrite_factory, riid, out);
}

static HRESULT WINAPI classfactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("%d.\n", dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl classfactoryvtbl =
{
    classfactory_QueryInterface,
    classfactory_AddRef,
    classfactory_Release,
    classfactory_CreateInstance,
    classfactory_LockServer,
};

static IClassFactory classfactory = { &classfactoryvtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    TRACE("%s, %s, %p.\n", debugstr_guid(clsid), debugstr_guid(riid), out);

    if (IsEqualGUID(clsid, &CLSID_MFReadWriteClassFactory))
        return IClassFactory_QueryInterface(&classfactory, riid, out);

    WARN("Unsupported class %s.\n", debugstr_guid(clsid));
    *out = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}
