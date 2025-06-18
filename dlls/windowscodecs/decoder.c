/*
 * Copyright 2020 Esme Povirk
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

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

struct object_enumerator
{
    IEnumUnknown IEnumUnknown_iface;
    LONG refcount;

    IUnknown **objects;
    unsigned int count;
    unsigned int position;
};

static inline struct object_enumerator *impl_from_IEnumUnknown(IEnumUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct object_enumerator, IEnumUnknown_iface);
}

static HRESULT WINAPI object_enumerator_QueryInterface(IEnumUnknown *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IEnumUnknown) ||
            IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IEnumUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI object_enumerator_AddRef(IEnumUnknown *iface)
{
    struct object_enumerator *enumerator = impl_from_IEnumUnknown(iface);
    ULONG refcount = InterlockedIncrement(&enumerator->refcount);

    TRACE("%p refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI object_enumerator_Release(IEnumUnknown *iface)
{
    struct object_enumerator *enumerator = impl_from_IEnumUnknown(iface);
    ULONG refcount = InterlockedDecrement(&enumerator->refcount);
    unsigned int i;

    TRACE("%p refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < enumerator->count; ++i)
            IUnknown_Release(enumerator->objects[i]);
        free(enumerator->objects);
        free(enumerator);
    }

    return refcount;
}

static HRESULT WINAPI object_enumerator_Next(IEnumUnknown *iface, ULONG count, IUnknown **ret, ULONG *fetched)
{
    struct object_enumerator *enumerator = impl_from_IEnumUnknown(iface);
    ULONG tmp;

    TRACE("%p, %lu, %p, %p.\n", iface, count, ret, fetched);

    if (!fetched) fetched = &tmp;

    *fetched = 0;

    while (enumerator->position < enumerator->count && *fetched < count)
    {
        *ret = enumerator->objects[enumerator->position++];
        IUnknown_AddRef(*ret);

        *fetched = *fetched + 1;
        ret++;
    }

    return *fetched == count ? S_OK : S_FALSE;
}

static HRESULT WINAPI object_enumerator_Skip(IEnumUnknown *iface, ULONG count)
{
    struct object_enumerator *enumerator = impl_from_IEnumUnknown(iface);
    HRESULT hr;

    TRACE("%p, %lu.\n", iface, count);

    hr = (count > enumerator->count || enumerator->position > enumerator->count - count) ? S_FALSE : S_OK;

    count = min(count, enumerator->count - enumerator->position);
    enumerator->position += count;

    return hr;
}

static HRESULT WINAPI object_enumerator_Reset(IEnumUnknown *iface)
{
    struct object_enumerator *enumerator = impl_from_IEnumUnknown(iface);

    TRACE("%p.\n", iface);

    enumerator->position = 0;
    return S_OK;
}

static HRESULT create_object_enumerator(IUnknown **objects, unsigned int position,
        unsigned int count, IEnumUnknown **ret);

static HRESULT WINAPI object_enumerator_Clone(IEnumUnknown *iface, IEnumUnknown **ret)
{
    struct object_enumerator *enumerator = impl_from_IEnumUnknown(iface);

    TRACE("%p, %p.\n", iface, ret);

    if (!ret)
        return E_INVALIDARG;

    return create_object_enumerator(enumerator->objects, enumerator->position, enumerator->count, ret);
}

static const IEnumUnknownVtbl object_enumerator_vtbl =
{
    object_enumerator_QueryInterface,
    object_enumerator_AddRef,
    object_enumerator_Release,
    object_enumerator_Next,
    object_enumerator_Skip,
    object_enumerator_Reset,
    object_enumerator_Clone,
};

static HRESULT create_object_enumerator(IUnknown **objects, unsigned int position,
        unsigned int count, IEnumUnknown **ret)
{
    struct object_enumerator *object;
    unsigned int i;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumUnknown_iface.lpVtbl = &object_enumerator_vtbl;
    object->refcount = 1;
    if (!(object->objects = calloc(count, sizeof(*object->objects))))
    {
        free(object);
        return E_OUTOFMEMORY;
    }
    object->position = position;
    object->count = count;

    for (i = 0; i < count; ++i)
    {
        object->objects[i] = objects[i];
        IUnknown_AddRef(object->objects[i]);
    }

    *ret = &object->IEnumUnknown_iface;

    return S_OK;
}

typedef struct CommonDecoder CommonDecoder;

struct metadata_block_reader
{
    BOOL metadata_initialized;
    UINT metadata_count;
    struct decoder_block *metadata_blocks;
    IWICMetadataReader **readers;
    UINT frame;
    CommonDecoder *decoder;
};

typedef struct CommonDecoder {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    IWICMetadataBlockReader IWICMetadataBlockReader_iface;
    LONG ref;
    CRITICAL_SECTION lock; /* must be held when stream or decoder is accessed */
    IStream *stream;
    UINT frame;
    struct decoder *decoder;
    struct decoder_info decoder_info;
    struct decoder_stat file_info;
    struct metadata_block_reader block_reader;
    WICDecodeOptions cache_options;
} CommonDecoder;

static void metadata_block_reader_initialize(struct metadata_block_reader *block_reader, CommonDecoder *decoder, UINT frame)
{
    memset(block_reader, 0, sizeof(*block_reader));
    block_reader->decoder = decoder;
    block_reader->frame = frame;
}

static void metadata_block_reader_cleanup(struct metadata_block_reader *block_reader)
{
    UINT i;

    for (i = 0; i < block_reader->metadata_count && block_reader->readers; ++i)
    {
        if (block_reader->readers[i])
            IWICMetadataReader_Release(block_reader->readers[i]);
    }
    free(block_reader->readers);
    free(block_reader->metadata_blocks);
    memset(block_reader, 0, sizeof(*block_reader));
}

static HRESULT metadata_block_reader_initialize_metadata(struct metadata_block_reader *block_reader)
{
    HRESULT hr = S_OK;

    if (block_reader->metadata_initialized)
        return S_OK;

    EnterCriticalSection(&block_reader->decoder->lock);

    if (!block_reader->metadata_initialized)
    {
        hr = decoder_get_metadata_blocks(block_reader->decoder->decoder, block_reader->frame,
                &block_reader->metadata_count, &block_reader->metadata_blocks);
        if (SUCCEEDED(hr))
            block_reader->metadata_initialized = TRUE;

        if (SUCCEEDED(hr))
        {
            block_reader->readers = calloc(block_reader->metadata_count, sizeof(*block_reader->readers));
            if (!block_reader->readers)
            {
                metadata_block_reader_cleanup(block_reader);
                hr = E_OUTOFMEMORY;
            }
        }
    }

    LeaveCriticalSection(&block_reader->decoder->lock);

    return hr;
}

static HRESULT metadata_block_reader_get_container_format(struct metadata_block_reader *block_reader, GUID *format)
{
    if (!format) return E_INVALIDARG;
    *format = block_reader->decoder->decoder_info.block_format;
    return S_OK;
}

static HRESULT metadata_block_reader_get_count(struct metadata_block_reader *block_reader, UINT *count)
{
    HRESULT hr;

    if (!count) return E_INVALIDARG;

    hr = metadata_block_reader_initialize_metadata(block_reader);
    if (SUCCEEDED(hr))
        *count = block_reader->metadata_count;

    return hr;
}

static HRESULT metadata_block_reader_get_reader(struct metadata_block_reader *block_reader,
        UINT index, IWICMetadataReader **ret_reader)
{
    IWICComponentFactory *factory = NULL;
    IWICMetadataReader *reader;
    IWICStream *stream;
    HRESULT hr;

    if (!ret_reader)
        return E_INVALIDARG;

    *ret_reader = NULL;

    hr = metadata_block_reader_initialize_metadata(block_reader);

    if (SUCCEEDED(hr) && index >= block_reader->metadata_count)
        hr = E_INVALIDARG;

    if (SUCCEEDED(hr) && block_reader->readers[index])
    {
        *ret_reader = block_reader->readers[index];
        IWICMetadataReader_AddRef(*ret_reader);
        return S_OK;
    }

    if (SUCCEEDED(hr))
        hr = create_instance(&CLSID_WICImagingFactory, &IID_IWICComponentFactory, (void**)&factory);

    if (SUCCEEDED(hr))
        hr = IWICComponentFactory_CreateStream(factory, &stream);

    if (SUCCEEDED(hr))
    {
        if (block_reader->metadata_blocks[index].options & DECODER_BLOCK_FULL_STREAM)
        {
            LARGE_INTEGER offset;
            offset.QuadPart = block_reader->metadata_blocks[index].offset;

            hr = IWICStream_InitializeFromIStream(stream, block_reader->decoder->stream);

            if (SUCCEEDED(hr))
                hr = IWICStream_Seek(stream, offset, STREAM_SEEK_SET, NULL);
        }
        else if (block_reader->metadata_blocks[index].options & DECODER_BLOCK_OFFSET_IS_PTR)
        {
            BYTE *data = (BYTE *)(ULONG_PTR)block_reader->metadata_blocks[index].offset;
            UINT size = block_reader->metadata_blocks[index].length;

            hr = IWICStream_InitializeFromMemory(stream, data, size);
        }
        else
        {
            ULARGE_INTEGER offset, length;

            offset.QuadPart = block_reader->metadata_blocks[index].offset;
            length.QuadPart = block_reader->metadata_blocks[index].length;

            hr = IWICStream_InitializeFromIStreamRegion(stream, block_reader->decoder->stream,
                offset, length);
        }

        if (block_reader->metadata_blocks[index].options & DECODER_BLOCK_READER_CLSID)
        {
            IWICPersistStream *persist;
            if (SUCCEEDED(hr))
            {
                hr = create_instance(&block_reader->metadata_blocks[index].reader_clsid,
                    &IID_IWICMetadataReader, (void**)&reader);
            }

            if (SUCCEEDED(hr))
            {
                hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICPersistStream, (void**)&persist);

                if (SUCCEEDED(hr))
                {
                    hr = IWICPersistStream_LoadEx(persist, (IStream*)stream, NULL,
                        block_reader->metadata_blocks[index].options & DECODER_BLOCK_OPTION_MASK);

                    IWICPersistStream_Release(persist);
                }

                if (FAILED(hr))
                {
                    IWICMetadataReader_Release(reader);
                    reader = NULL;
                }
            }
        }
        else
        {
            hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
                &block_reader->decoder->decoder_info.block_format, NULL,
                block_reader->metadata_blocks[index].options & DECODER_BLOCK_OPTION_MASK,
                (IStream *)stream, &reader);
        }

        IWICStream_Release(stream);
    }

    if (factory) IWICComponentFactory_Release(factory);

    if (SUCCEEDED(hr))
    {
        if (InterlockedCompareExchangePointer((void **)&block_reader->readers[index], reader, NULL))
            IWICMetadataReader_Release(reader);

        *ret_reader = block_reader->readers[index];
        IWICMetadataReader_AddRef(*ret_reader);
    }

    return hr;
}

static HRESULT metadata_block_reader_get_enumerator(struct metadata_block_reader *block_reader,
        IEnumUnknown **enumerator)
{
    IUnknown **objects;
    HRESULT hr = S_OK;
    UINT count, i;

    if (!enumerator)
        return E_INVALIDARG;

    *enumerator = NULL;

    if (FAILED(hr = metadata_block_reader_initialize_metadata(block_reader)))
        return hr;

    count = block_reader->metadata_count;

    if (!(objects = calloc(count, sizeof(*objects))))
        return E_OUTOFMEMORY;

    for (i = 0; i < count; ++i)
    {
        hr = metadata_block_reader_get_reader(block_reader, i, (IWICMetadataReader **)&objects[i]);
        if (FAILED(hr))
            break;
    }

    if (SUCCEEDED(hr))
        hr = create_object_enumerator(objects, 0, count, enumerator);

    for (i = 0; i < count; ++i)
    {
        if (objects[i])
            IUnknown_Release(objects[i]);
    }
    free(objects);

    return hr;
}

static inline CommonDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, CommonDecoder, IWICBitmapDecoder_iface);
}

static inline CommonDecoder *impl_decoder_from_IWICMetadataBlockReader(IWICMetadataBlockReader *iface)
{
    return CONTAINING_RECORD(iface, CommonDecoder, IWICMetadataBlockReader_iface);
}

static HRESULT WINAPI CommonDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IWICBitmapDecoder, iid))
    {
        *ppv = &This->IWICBitmapDecoder_iface;
    }
    else if (This->file_info.flags & DECODER_FLAGS_METADATA_AT_DECODER && IsEqualIID(&IID_IWICMetadataBlockReader, iid))
    {
        *ppv = &This->IWICMetadataBlockReader_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI CommonDecoder_AddRef(IWICBitmapDecoder *iface)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI CommonDecoder_Release(IWICBitmapDecoder *iface)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream)
            IStream_Release(This->stream);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        metadata_block_reader_cleanup(&This->block_reader);
        decoder_destroy(This->decoder);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI CommonDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
    DWORD *capability)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, stream, capability);

    if (!stream || !capability) return E_INVALIDARG;

    hr = IWICBitmapDecoder_Initialize(iface, stream, WICDecodeMetadataCacheOnDemand);
    if (hr != S_OK) return hr;

    *capability = (This->file_info.flags & DECODER_FLAGS_CAPABILITY_MASK);
    return S_OK;
}

static HRESULT WINAPI CommonDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    HRESULT hr=S_OK;

    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->stream)
        hr = WINCODEC_ERR_WRONGSTATE;

    if (SUCCEEDED(hr))
        hr = decoder_initialize(This->decoder, pIStream, &This->file_info);

    if (SUCCEEDED(hr))
    {
        This->cache_options = cacheOptions;
        This->stream = pIStream;
        IStream_AddRef(This->stream);
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI CommonDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    memcpy(pguidContainerFormat, &This->decoder_info.container_format, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI CommonDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    return get_decoder_info(&This->decoder_info.clsid, ppIDecoderInfo);
}

static HRESULT WINAPI CommonDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *palette)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    WICColor colors[256];
    UINT num_colors;
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, palette);

    if (FAILED(hr = decoder_get_decoder_palette(This->decoder, This->frame, colors, &num_colors)))
        return hr;

    if (num_colors)
    {
        hr = IWICPalette_InitializeCustom(palette, colors, num_colors);
    }
    else
    {
        hr = WINCODEC_ERR_PALETTEUNAVAILABLE;
    }

    return hr;
}

static HRESULT common_decoder_create_query_reader(IWICMetadataBlockReader *block_reader, IWICMetadataQueryReader **query_reader)
{
    IWICComponentFactory *factory;
    HRESULT hr;

    hr = create_instance(&CLSID_WICImagingFactory, &IID_IWICComponentFactory, (void **)&factory);

    if (SUCCEEDED(hr))
    {
        hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory, block_reader, query_reader);
        IWICComponentFactory_Release(factory);
    }

    return hr;
}

static HRESULT WINAPI CommonDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **reader)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);

    TRACE("(%p,%p)\n", iface, reader);

    if (!reader) return E_INVALIDARG;

    if (This->file_info.flags & DECODER_FLAGS_METADATA_AT_DECODER)
        return common_decoder_create_query_reader(&This->IWICMetadataBlockReader_iface, reader);

    *reader = NULL;
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI CommonDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);

    if (!ppIBitmapSource) return E_INVALIDARG;

    *ppIBitmapSource = NULL;
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI CommonDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI CommonDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    if (!ppIThumbnail) return E_INVALIDARG;

    *ppIThumbnail = NULL;
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI CommonDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    if (!pCount) return E_INVALIDARG;

    if (This->stream)
        *pCount = This->file_info.frame_count;
    else
        *pCount = 0;

    return S_OK;
}

static HRESULT WINAPI CommonDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame);

static const IWICBitmapDecoderVtbl CommonDecoder_Vtbl = {
    CommonDecoder_QueryInterface,
    CommonDecoder_AddRef,
    CommonDecoder_Release,
    CommonDecoder_QueryCapability,
    CommonDecoder_Initialize,
    CommonDecoder_GetContainerFormat,
    CommonDecoder_GetDecoderInfo,
    CommonDecoder_CopyPalette,
    CommonDecoder_GetMetadataQueryReader,
    CommonDecoder_GetPreview,
    CommonDecoder_GetColorContexts,
    CommonDecoder_GetThumbnail,
    CommonDecoder_GetFrameCount,
    CommonDecoder_GetFrame
};

static HRESULT WINAPI CommonDecoder_Block_QueryInterface(IWICMetadataBlockReader *iface, REFIID iid,
    void **ppv)
{
    CommonDecoder *decoder = impl_decoder_from_IWICMetadataBlockReader(iface);
    return IWICBitmapDecoder_QueryInterface(&decoder->IWICBitmapDecoder_iface, iid, ppv);
}

static ULONG WINAPI CommonDecoder_Block_AddRef(IWICMetadataBlockReader *iface)
{
    CommonDecoder *decoder = impl_decoder_from_IWICMetadataBlockReader(iface);
    return IWICBitmapDecoder_AddRef(&decoder->IWICBitmapDecoder_iface);
}

static ULONG WINAPI CommonDecoder_Block_Release(IWICMetadataBlockReader *iface)
{
    CommonDecoder *decoder = impl_decoder_from_IWICMetadataBlockReader(iface);
    return IWICBitmapDecoder_Release(&decoder->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI CommonDecoder_Block_GetContainerFormat(IWICMetadataBlockReader *iface, GUID *format)
{
    CommonDecoder *decoder = impl_decoder_from_IWICMetadataBlockReader(iface);
    return metadata_block_reader_get_container_format(&decoder->block_reader, format);
}

static HRESULT WINAPI CommonDecoder_Block_GetCount(IWICMetadataBlockReader *iface, UINT *count)
{
    CommonDecoder *decoder = impl_decoder_from_IWICMetadataBlockReader(iface);

    TRACE("%p,%p\n", iface, count);

    return metadata_block_reader_get_count(&decoder->block_reader, count);
}

static HRESULT WINAPI CommonDecoder_Block_GetReaderByIndex(IWICMetadataBlockReader *iface,
        UINT index, IWICMetadataReader **reader)
{
    CommonDecoder *decoder = impl_decoder_from_IWICMetadataBlockReader(iface);

    TRACE("%p,%d,%p\n", iface, index, reader);

    return metadata_block_reader_get_reader(&decoder->block_reader, index, reader);
}

static HRESULT WINAPI CommonDecoder_Block_GetEnumerator(IWICMetadataBlockReader *iface,
        IEnumUnknown **enumerator)
{
    CommonDecoder *decoder = impl_decoder_from_IWICMetadataBlockReader(iface);

    TRACE("%p,%p\n", iface, enumerator);

    return metadata_block_reader_get_enumerator(&decoder->block_reader, enumerator);
}

static const IWICMetadataBlockReaderVtbl CommonDecoder_BlockVtbl =
{
    CommonDecoder_Block_QueryInterface,
    CommonDecoder_Block_AddRef,
    CommonDecoder_Block_Release,
    CommonDecoder_Block_GetContainerFormat,
    CommonDecoder_Block_GetCount,
    CommonDecoder_Block_GetReaderByIndex,
    CommonDecoder_Block_GetEnumerator,
};

typedef struct {
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    IWICMetadataBlockReader IWICMetadataBlockReader_iface;
    LONG ref;
    CommonDecoder *parent;
    DWORD frame;
    struct decoder_frame decoder_frame;
    struct metadata_block_reader block_reader;
} CommonDecoderFrame;

static inline CommonDecoderFrame *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, CommonDecoderFrame, IWICBitmapFrameDecode_iface);
}

static inline CommonDecoderFrame *impl_from_IWICMetadataBlockReader(IWICMetadataBlockReader *iface)
{
    return CONTAINING_RECORD(iface, CommonDecoderFrame, IWICMetadataBlockReader_iface);
}

static HRESULT WINAPI CommonDecoderFrame_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
    {
        *ppv = &This->IWICBitmapFrameDecode_iface;
    }
    else if (IsEqualIID(&IID_IWICMetadataBlockReader, iid) &&
             (This->parent->file_info.flags & WICBitmapDecoderCapabilityCanEnumerateMetadata))
    {
        *ppv = &This->IWICMetadataBlockReader_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI CommonDecoderFrame_AddRef(IWICBitmapFrameDecode *iface)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI CommonDecoderFrame_Release(IWICBitmapFrameDecode *iface)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        IWICBitmapDecoder_Release(&This->parent->IWICBitmapDecoder_iface);
        metadata_block_reader_cleanup(&This->block_reader);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI CommonDecoderFrame_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%p,%p)\n", This, puiWidth, puiHeight);

    if (!puiWidth || !puiHeight)
        return E_POINTER;

    *puiWidth = This->decoder_frame.width;
    *puiHeight = This->decoder_frame.height;
    return S_OK;
}

static HRESULT WINAPI CommonDecoderFrame_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%p)\n", This, pPixelFormat);

    if (!pPixelFormat)
        return E_POINTER;

    *pPixelFormat = This->decoder_frame.pixel_format;
    return S_OK;
}

static HRESULT WINAPI CommonDecoderFrame_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%p,%p)\n", This, pDpiX, pDpiY);

    if (!pDpiX || !pDpiY)
        return E_POINTER;

    *pDpiX = This->decoder_frame.dpix;
    *pDpiY = This->decoder_frame.dpiy;
    return S_OK;
}

static HRESULT WINAPI CommonDecoderFrame_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    HRESULT hr=S_OK;

    TRACE("(%p,%p)\n", iface, pIPalette);

    if (This->decoder_frame.num_colors)
    {
        hr = IWICPalette_InitializeCustom(pIPalette, This->decoder_frame.palette, This->decoder_frame.num_colors);
    }
    else
    {
        hr = WINCODEC_ERR_PALETTEUNAVAILABLE;
    }

    return hr;
}

static HRESULT WINAPI CommonDecoderFrame_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    HRESULT hr;
    UINT bytesperrow;
    WICRect rect;

    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    if (!pbBuffer)
        return E_POINTER;

    if (!prc)
    {
        rect.X = 0;
        rect.Y = 0;
        rect.Width = This->decoder_frame.width;
        rect.Height = This->decoder_frame.height;
        prc = &rect;
    }
    else
    {
        if (prc->X < 0 || prc->Y < 0 ||
            prc->X+prc->Width > This->decoder_frame.width ||
            prc->Y+prc->Height > This->decoder_frame.height)
            return E_INVALIDARG;
    }

    bytesperrow = ((This->decoder_frame.bpp * prc->Width)+7)/8;

    if (cbStride < bytesperrow)
        return E_INVALIDARG;

    if ((cbStride * (prc->Height-1)) + bytesperrow > cbBufferSize)
        return E_INVALIDARG;

    EnterCriticalSection(&This->parent->lock);

    hr = decoder_copy_pixels(This->parent->decoder, This->frame,
        prc, cbStride, cbBufferSize, pbBuffer);

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI CommonDecoderFrame_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);

    if (!ppIMetadataQueryReader)
        return E_INVALIDARG;

    if (!(This->parent->file_info.flags & WICBitmapDecoderCapabilityCanEnumerateMetadata))
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;

    return common_decoder_create_query_reader(&This->IWICMetadataBlockReader_iface, ppIMetadataQueryReader);
}

static HRESULT WINAPI CommonDecoderFrame_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    HRESULT hr=S_OK;
    UINT i;
    BYTE *profile;
    DWORD profile_len;

    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);

    if (!pcActualCount) return E_INVALIDARG;

    if (This->parent->file_info.flags & DECODER_FLAGS_UNSUPPORTED_COLOR_CONTEXT)
    {
        FIXME("not supported for %s\n", wine_dbgstr_guid(&This->parent->decoder_info.clsid));
        *pcActualCount = 0;
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }

    *pcActualCount = This->decoder_frame.num_color_contexts;

    if (This->decoder_frame.num_color_contexts && cCount && ppIColorContexts)
    {
        if (cCount >= This->decoder_frame.num_color_contexts)
        {
            EnterCriticalSection(&This->parent->lock);

            for (i=0; i<This->decoder_frame.num_color_contexts; i++)
            {
                hr = decoder_get_color_context(This->parent->decoder, This->frame, i,
                    &profile, &profile_len);
                if (SUCCEEDED(hr))
                {
                    hr = IWICColorContext_InitializeFromMemory(ppIColorContexts[i], profile, profile_len);

                    free(profile);
                }

                if (FAILED(hr))
                    break;
            }

            LeaveCriticalSection(&This->parent->lock);
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

static HRESULT WINAPI CommonDecoderFrame_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    if (!ppIThumbnail) return E_INVALIDARG;

    *ppIThumbnail = NULL;
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static const IWICBitmapFrameDecodeVtbl CommonDecoderFrameVtbl = {
    CommonDecoderFrame_QueryInterface,
    CommonDecoderFrame_AddRef,
    CommonDecoderFrame_Release,
    CommonDecoderFrame_GetSize,
    CommonDecoderFrame_GetPixelFormat,
    CommonDecoderFrame_GetResolution,
    CommonDecoderFrame_CopyPalette,
    CommonDecoderFrame_CopyPixels,
    CommonDecoderFrame_GetMetadataQueryReader,
    CommonDecoderFrame_GetColorContexts,
    CommonDecoderFrame_GetThumbnail
};

static HRESULT WINAPI CommonDecoderFrame_Block_QueryInterface(IWICMetadataBlockReader *iface, REFIID iid,
    void **ppv)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_QueryInterface(&This->IWICBitmapFrameDecode_iface, iid, ppv);
}

static ULONG WINAPI CommonDecoderFrame_Block_AddRef(IWICMetadataBlockReader *iface)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_AddRef(&This->IWICBitmapFrameDecode_iface);
}

static ULONG WINAPI CommonDecoderFrame_Block_Release(IWICMetadataBlockReader *iface)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_Release(&This->IWICBitmapFrameDecode_iface);
}

static HRESULT WINAPI CommonDecoderFrame_Block_GetContainerFormat(IWICMetadataBlockReader *iface,
    GUID *pguidContainerFormat)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);
    return metadata_block_reader_get_container_format(&This->block_reader, pguidContainerFormat);
}

static HRESULT WINAPI CommonDecoderFrame_Block_GetCount(IWICMetadataBlockReader *iface,
    UINT *pcCount)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);

    TRACE("%p,%p\n", iface, pcCount);

    return metadata_block_reader_get_count(&This->block_reader, pcCount);
}

static HRESULT WINAPI CommonDecoderFrame_Block_GetReaderByIndex(IWICMetadataBlockReader *iface,
    UINT nIndex, IWICMetadataReader **ppIMetadataReader)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);

    TRACE("%p,%d,%p\n", iface, nIndex, ppIMetadataReader);

    return metadata_block_reader_get_reader(&This->block_reader, nIndex, ppIMetadataReader);
}

static HRESULT WINAPI CommonDecoderFrame_Block_GetEnumerator(IWICMetadataBlockReader *iface,
        IEnumUnknown **enumerator)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);

    TRACE("%p,%p\n", iface, enumerator);

    return metadata_block_reader_get_enumerator(&This->block_reader, enumerator);
}

static const IWICMetadataBlockReaderVtbl CommonDecoderFrame_BlockVtbl = {
    CommonDecoderFrame_Block_QueryInterface,
    CommonDecoderFrame_Block_AddRef,
    CommonDecoderFrame_Block_Release,
    CommonDecoderFrame_Block_GetContainerFormat,
    CommonDecoderFrame_Block_GetCount,
    CommonDecoderFrame_Block_GetReaderByIndex,
    CommonDecoderFrame_Block_GetEnumerator,
};

static HRESULT WINAPI CommonDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    HRESULT hr=S_OK;
    CommonDecoderFrame *result;

    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!ppIBitmapFrame)
        return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->stream || index >= This->file_info.frame_count)
        hr = WINCODEC_ERR_FRAMEMISSING;

    if (SUCCEEDED(hr))
    {
        result = malloc(sizeof(*result));
        if (!result)
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        result->IWICBitmapFrameDecode_iface.lpVtbl = &CommonDecoderFrameVtbl;
        result->IWICMetadataBlockReader_iface.lpVtbl = &CommonDecoderFrame_BlockVtbl;
        result->ref = 1;
        result->parent = This;
        result->frame = index;
        metadata_block_reader_initialize(&result->block_reader, This, index);

        hr = decoder_get_frame_info(This->decoder, index, &result->decoder_frame);

        if (SUCCEEDED(hr) && This->cache_options == WICDecodeMetadataCacheOnLoad)
            hr = metadata_block_reader_initialize_metadata(&result->block_reader);

        if (SUCCEEDED(hr))
            This->frame = result->frame;

        if (FAILED(hr))
            free(result);
    }

    LeaveCriticalSection(&This->lock);

    if (SUCCEEDED(hr))
    {
        TRACE("-> %ux%u, %u-bit pixelformat=%s res=%f,%f colors=%lu contexts=%lu\n",
            result->decoder_frame.width, result->decoder_frame.height,
            result->decoder_frame.bpp, wine_dbgstr_guid(&result->decoder_frame.pixel_format),
            result->decoder_frame.dpix, result->decoder_frame.dpiy,
            result->decoder_frame.num_colors, result->decoder_frame.num_color_contexts);
        IWICBitmapDecoder_AddRef(&This->IWICBitmapDecoder_iface);
        *ppIBitmapFrame = &result->IWICBitmapFrameDecode_iface;
    }
    else
    {
        *ppIBitmapFrame = NULL;
    }

    return hr;
}

HRESULT CommonDecoder_CreateInstance(struct decoder *decoder,
    const struct decoder_info *decoder_info, REFIID iid, void** ppv)
{
    CommonDecoder *This;
    HRESULT hr;

    TRACE("(%s,%s,%p)\n", debugstr_guid(&decoder_info->clsid), debugstr_guid(iid), ppv);

    if (!(This = calloc(1, sizeof(*This))))
    {
        decoder_destroy(decoder);
        return E_OUTOFMEMORY;
    }

    This->IWICBitmapDecoder_iface.lpVtbl = &CommonDecoder_Vtbl;
    This->IWICMetadataBlockReader_iface.lpVtbl = &CommonDecoder_BlockVtbl;
    This->ref = 1;
    This->decoder = decoder;
    This->decoder_info = *decoder_info;
    metadata_block_reader_initialize(&This->block_reader, This, ~0u);
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": CommonDecoder.lock");

    hr = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return hr;
}
