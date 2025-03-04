/*
 * MP3 decoder DMO
 *
 * Copyright 2018 Zebediah Figura
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
#include <stdio.h>
#include <mpg123.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "mmreg.h"
#define COBJMACROS
#include "objbase.h"
#include "dmo.h"
#include "rpcproxy.h"
#include "wmcodecdsp.h"
#include "wine/debug.h"

#include "initguid.h"
DEFINE_GUID(WMMEDIATYPE_Audio, 0x73647561,0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);
DEFINE_GUID(WMMEDIASUBTYPE_MP3,0x00000055,0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);
DEFINE_GUID(WMMEDIASUBTYPE_PCM,0x00000001,0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);
DEFINE_GUID(WMFORMAT_WaveFormatEx,  0x05589f81,0xc356,0x11ce,0xbf,0x01,0x00,0xaa,0x00,0x55,0x59,0x5a);

WINE_DEFAULT_DEBUG_CHANNEL(mp3dmod);

struct mp3_decoder
{
    IUnknown IUnknown_inner;
    IMediaObject IMediaObject_iface;
    IUnknown *outer;
    LONG ref;
    mpg123_handle *mh;

    DMO_MEDIA_TYPE intype, outtype;
    BOOL intype_set, outtype_set;

    IMediaBuffer *buffer;
    REFERENCE_TIME timestamp;
};

static inline struct mp3_decoder *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct mp3_decoder, IUnknown_inner);
}

static HRESULT WINAPI Unknown_QueryInterface(IUnknown *iface, REFIID iid, void **obj)
{
    struct mp3_decoder *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(iid), obj);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *obj = &This->IUnknown_inner;
    else if (IsEqualGUID(iid, &IID_IMediaObject))
        *obj = &This->IMediaObject_iface;
    else
    {
        FIXME("no interface for %s\n", debugstr_guid(iid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI Unknown_AddRef(IUnknown *iface)
{
    struct mp3_decoder *This = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&This->ref);

    TRACE("(%p) AddRef from %ld\n", This, refcount - 1);

    return refcount;
}

static ULONG WINAPI Unknown_Release(IUnknown *iface)
{
    struct mp3_decoder *This = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&This->ref);

    TRACE("(%p) Release from %ld\n", This, refcount + 1);

    if (!refcount)
    {
        if (This->buffer)
            IMediaBuffer_Release(This->buffer);
        if (This->intype_set)
            MoFreeMediaType(&This->intype);
        MoFreeMediaType(&This->outtype);
        mpg123_delete(This->mh);
        free(This);
    }
    return refcount;
}

static const IUnknownVtbl Unknown_vtbl = {
    Unknown_QueryInterface,
    Unknown_AddRef,
    Unknown_Release,
};

static inline struct mp3_decoder *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct mp3_decoder, IMediaObject_iface);
}

static HRESULT WINAPI MediaObject_QueryInterface(IMediaObject *iface, REFIID iid, void **obj)
{
    struct mp3_decoder *This = impl_from_IMediaObject(iface);
    return IUnknown_QueryInterface(This->outer, iid, obj);
}

static ULONG WINAPI MediaObject_AddRef(IMediaObject *iface)
{
    struct mp3_decoder *This = impl_from_IMediaObject(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI MediaObject_Release(IMediaObject *iface)
{
    struct mp3_decoder *This = impl_from_IMediaObject(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI MediaObject_GetStreamCount(IMediaObject *iface, DWORD *input, DWORD *output)
{
    TRACE("iface %p, input %p, output %p.\n", iface, input, output);

    *input = *output = 1;

    return S_OK;
}

static HRESULT WINAPI MediaObject_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    TRACE("iface %p, index %lu, flags %p.\n", iface, index, flags);

    *flags = 0;

    return S_OK;
}

static HRESULT WINAPI MediaObject_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    TRACE("iface %p, index %lu, flags %p.\n", iface, index, flags);

    *flags = 0;

    return S_OK;
}

static HRESULT WINAPI MediaObject_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    TRACE("iface %p, index %lu, type_index %lu, type %p.\n", iface, index, type_index, type);

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;

    if (type_index)
        return DMO_E_NO_MORE_ITEMS;

    type->majortype = WMMEDIATYPE_Audio;
    type->subtype = WMMEDIASUBTYPE_MP3;
    type->formattype = GUID_NULL;
    type->pUnk = NULL;
    type->cbFormat = 0;
    type->pbFormat = NULL;

    return S_OK;
}

static HRESULT WINAPI MediaObject_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    struct mp3_decoder *dmo = impl_from_IMediaObject(iface);
    const WAVEFORMATEX *input_format;
    WAVEFORMATEX *format;

    TRACE("iface %p, index %lu, type_index %lu, type %p.\n", iface, index, type_index, type);

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;

    if (!dmo->intype_set)
        return DMO_E_TYPE_NOT_SET;

    input_format = (WAVEFORMATEX *)dmo->intype.pbFormat;

    if (type_index >= (2 * input_format->nChannels))
        return DMO_E_NO_MORE_ITEMS;

    type->majortype = WMMEDIATYPE_Audio;
    type->subtype = WMMEDIASUBTYPE_PCM;
    type->formattype = WMFORMAT_WaveFormatEx;
    type->pUnk = NULL;
    type->cbFormat = sizeof(WAVEFORMATEX);
    if (!(type->pbFormat = CoTaskMemAlloc(sizeof(WAVEFORMATEX))))
        return E_OUTOFMEMORY;
    format = (WAVEFORMATEX *)type->pbFormat;
    format->wFormatTag = WAVE_FORMAT_PCM;
    format->nSamplesPerSec = input_format->nSamplesPerSec;
    format->nChannels = (type_index / 2) ? 1 : input_format->nChannels;
    format->wBitsPerSample = (type_index % 2) ? 8 : 16;
    format->nBlockAlign = format->nChannels * format->wBitsPerSample / 8;
    format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;
    format->cbSize = 0;

    return S_OK;
}

static HRESULT WINAPI MediaObject_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct mp3_decoder *dmo = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, type %p, flags %#lx.\n", iface, index, type, flags);

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;

    if (flags & DMO_SET_TYPEF_CLEAR)
    {
        if (dmo->intype_set)
            MoFreeMediaType(&dmo->intype);
        dmo->intype_set = FALSE;
        return S_OK;
    }

    if (!IsEqualGUID(&type->majortype, &WMMEDIATYPE_Audio)
            || !IsEqualGUID(&type->subtype, &WMMEDIASUBTYPE_MP3)
            || !IsEqualGUID(&type->formattype, &WMFORMAT_WaveFormatEx))
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (!(flags & DMO_SET_TYPEF_TEST_ONLY))
    {
        if (dmo->intype_set)
            MoFreeMediaType(&dmo->intype);
        MoCopyMediaType(&dmo->intype, type);
        dmo->intype_set = TRUE;
    }

    return S_OK;
}

static HRESULT WINAPI MediaObject_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct mp3_decoder *This = impl_from_IMediaObject(iface);
    WAVEFORMATEX *format;
    long enc;
    int err;

    TRACE("(%p)->(%ld, %p, %#lx)\n", iface, index, type, flags);

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;

    if (!This->intype_set)
        return DMO_E_TYPE_NOT_SET;

    if (flags & DMO_SET_TYPEF_CLEAR)
    {
        MoFreeMediaType(&This->outtype);
        This->outtype_set = FALSE;
        return S_OK;
    }

    if (!IsEqualGUID(&type->formattype, &WMFORMAT_WaveFormatEx))
        return DMO_E_TYPE_NOT_ACCEPTED;

    format = (WAVEFORMATEX *)type->pbFormat;

    if (format->wBitsPerSample == 8)
        enc = MPG123_ENC_UNSIGNED_8;
    else if (format->wBitsPerSample == 16)
        enc = MPG123_ENC_SIGNED_16;
    else
    {
        ERR("Cannot decode to bit depth %u.\n", format->wBitsPerSample);
        return DMO_E_TYPE_NOT_ACCEPTED;
    }

    if (!(flags & DMO_SET_TYPEF_TEST_ONLY))
    {
        err = mpg123_format(This->mh, format->nSamplesPerSec, format->nChannels, enc);
        if (err != MPG123_OK)
        {
            ERR("Failed to set format: %u channels, %lu samples/sec, %u bits/sample.\n",
                format->nChannels, format->nSamplesPerSec, format->wBitsPerSample);
            return DMO_E_TYPE_NOT_ACCEPTED;
        }
        if (This->outtype_set)
            MoFreeMediaType(&This->outtype);
        MoCopyMediaType(&This->outtype, type);
        This->outtype_set = TRUE;
    }

    return S_OK;
}

static HRESULT WINAPI MediaObject_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    struct mp3_decoder *dmo = impl_from_IMediaObject(iface);
    TRACE("(%p)->(%ld, %p)\n", iface, index, type);

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;

    if (!dmo->intype_set)
        return DMO_E_TYPE_NOT_SET;

    MoCopyMediaType(type, &dmo->intype);

    return S_OK;
}

static HRESULT WINAPI MediaObject_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    struct mp3_decoder *dmo = impl_from_IMediaObject(iface);
    TRACE("(%p)->(%ld, %p)\n", iface, index, type);

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;

    if (!dmo->outtype_set)
        return DMO_E_TYPE_NOT_SET;

    MoCopyMediaType(type, &dmo->outtype);

    return S_OK;
}

static HRESULT WINAPI MediaObject_GetInputSizeInfo(IMediaObject *iface,
        DWORD index, DWORD *size, DWORD *lookahead, DWORD *alignment)
{
    struct mp3_decoder *dmo = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, size %p, lookahead %p, alignment %p.\n", iface, index, size, lookahead, alignment);

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;

    if (!dmo->intype_set || !dmo->outtype_set)
        return DMO_E_TYPE_NOT_SET;

    *size = 0;
    *alignment = 1;
    return S_OK;
}

static HRESULT WINAPI MediaObject_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    struct mp3_decoder *dmo = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, size %p, alignment %p.\n", iface, index, size, alignment);

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;

    if (!dmo->intype_set || !dmo->outtype_set)
        return DMO_E_TYPE_NOT_SET;

    *size = 2 * 1152 * ((WAVEFORMATEX *)dmo->outtype.pbFormat)->wBitsPerSample / 8;
    *alignment = 1;
    return S_OK;
}

static HRESULT WINAPI MediaObject_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    FIXME("(%p)->(%ld, %p) stub!\n", iface, index, latency);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    FIXME("(%p)->(%ld, %s) stub!\n", iface, index, wine_dbgstr_longlong(latency));

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_Flush(IMediaObject *iface)
{
    struct mp3_decoder *dmo = impl_from_IMediaObject(iface);

    TRACE("iface %p.\n", iface);

    if (dmo->buffer)
        IMediaBuffer_Release(dmo->buffer);
    dmo->buffer = NULL;
    dmo->timestamp = 0;

    /* mpg123 doesn't give us a way to flush, so just close and reopen the feed. */
    mpg123_close(dmo->mh);
    mpg123_open_feed(dmo->mh);

    return S_OK;
}

static HRESULT WINAPI MediaObject_Discontinuity(IMediaObject *iface, DWORD index)
{
    TRACE("iface %p.\n", iface);

    return S_OK;
}

static HRESULT WINAPI MediaObject_AllocateStreamingResources(IMediaObject *iface)
{
    TRACE("(%p)->()\n", iface);

    return S_OK;
}

static HRESULT WINAPI MediaObject_FreeStreamingResources(IMediaObject *iface)
{
    TRACE("(%p)->()\n", iface);

    return S_OK;
}

static HRESULT WINAPI MediaObject_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("(%p)->(%ld, %p) stub!\n", iface, index, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_ProcessInput(IMediaObject *iface, DWORD index,
    IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    struct mp3_decoder *This = impl_from_IMediaObject(iface);
    HRESULT hr;
    BYTE *data;
    DWORD len;
    int err;

    TRACE("(%p)->(%ld, %p, %#lx, %s, %s)\n", iface, index, buffer, flags,
          wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(timelength));

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;

    if (This->buffer)
    {
        ERR("Already have a buffer.\n");
        return DMO_E_NOTACCEPTING;
    }

    IMediaBuffer_AddRef(buffer);
    This->buffer = buffer;

    hr = IMediaBuffer_GetBufferAndLength(buffer, &data, &len);
    if (FAILED(hr))
        return hr;

    err = mpg123_feed(This->mh, data, len);
    if (err != MPG123_OK)
    {
        ERR("mpg123_feed() failed: %s\n", mpg123_strerror(This->mh));
        return E_FAIL;
    }

    return S_OK;
}

static DWORD get_framesize(DMO_MEDIA_TYPE *type)
{
    WAVEFORMATEX *format = (WAVEFORMATEX *)type->pbFormat;
    return 1152 * format->nBlockAlign;
}

static REFERENCE_TIME get_frametime(DMO_MEDIA_TYPE *type)
{
    WAVEFORMATEX *format = (WAVEFORMATEX *)type->pbFormat;
    return (REFERENCE_TIME) 10000000 * 1152 / format->nSamplesPerSec;
}

static HRESULT WINAPI MediaObject_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count, DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct mp3_decoder *This = impl_from_IMediaObject(iface);
    REFERENCE_TIME time = 0, frametime;
    DWORD len, maxlen, framesize;
    int got_data = 0;
    size_t written;
    HRESULT hr;
    BYTE *data;
    int err;

    TRACE("(%p)->(%#lx, %ld, %p, %p)\n", iface, flags, count, buffers, status);

    if (count > 1)
        FIXME("Multiple buffers not handled.\n");

    buffers[0].dwStatus = 0;

    if (!buffers[0].pBuffer)
    {
        while ((err = mpg123_read(This->mh, NULL, 0, &written)) == MPG123_NEW_FORMAT);
        if (err == MPG123_NEED_MORE)
            return S_OK;
        else if (err == MPG123_ERR)
            ERR("mpg123_read() failed: %s\n", mpg123_strerror(This->mh));
        else if (err != MPG123_OK)
            ERR("mpg123_read() returned %d\n", err);

        buffers[0].dwStatus = DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;
        return S_OK;
    }

    if (!This->buffer)
        return S_FALSE;

    buffers[0].dwStatus |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;

    hr = IMediaBuffer_GetBufferAndLength(buffers[0].pBuffer, &data, &len);
    if (FAILED(hr)) return hr;

    hr = IMediaBuffer_GetMaxLength(buffers[0].pBuffer, &maxlen);
    if (FAILED(hr)) return hr;

    framesize = get_framesize(&This->outtype);
    frametime = get_frametime(&This->outtype);

    while (1)
    {
        if (maxlen - len < framesize)
        {
            buffers[0].dwStatus |= DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;
            break;
        }

        while ((err = mpg123_read(This->mh, data + len, framesize, &written)) == MPG123_NEW_FORMAT);
        if (err == MPG123_NEED_MORE)
        {
            IMediaBuffer_Release(This->buffer);
            This->buffer = NULL;
            break;
        }
        else if (err == MPG123_ERR)
            ERR("mpg123_read() failed: %s\n", mpg123_strerror(This->mh));
        else if (err != MPG123_OK)
            ERR("mpg123_read() returned %d\n", err);
        if (written < framesize)
            ERR("short write: %Id/%lu\n", written, framesize);

        got_data = 1;

        len += framesize;
        hr = IMediaBuffer_SetLength(buffers[0].pBuffer, len);
        if (FAILED(hr)) return hr;

        time += frametime;
    }

    if (got_data)
    {
        buffers[0].dwStatus |= (DMO_OUTPUT_DATA_BUFFERF_TIME | DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH);
        buffers[0].rtTimelength = time;
        buffers[0].rtTimestamp = This->timestamp;
        This->timestamp += time;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI MediaObject_Lock(IMediaObject *iface, LONG lock)
{
    FIXME("(%p)->(%ld) stub!\n", iface, lock);

    return E_NOTIMPL;
}

static const IMediaObjectVtbl MediaObject_vtbl = {
    MediaObject_QueryInterface,
    MediaObject_AddRef,
    MediaObject_Release,
    MediaObject_GetStreamCount,
    MediaObject_GetInputStreamInfo,
    MediaObject_GetOutputStreamInfo,
    MediaObject_GetInputType,
    MediaObject_GetOutputType,
    MediaObject_SetInputType,
    MediaObject_SetOutputType,
    MediaObject_GetInputCurrentType,
    MediaObject_GetOutputCurrentType,
    MediaObject_GetInputSizeInfo,
    MediaObject_GetOutputSizeInfo,
    MediaObject_GetInputMaxLatency,
    MediaObject_SetInputMaxLatency,
    MediaObject_Flush,
    MediaObject_Discontinuity,
    MediaObject_AllocateStreamingResources,
    MediaObject_FreeStreamingResources,
    MediaObject_GetInputStatus,
    MediaObject_ProcessInput,
    MediaObject_ProcessOutput,
    MediaObject_Lock,
};

static HRESULT create_mp3_decoder(IUnknown *outer, REFIID iid, void **obj)
{
    struct mp3_decoder *This;
    HRESULT hr;
    int err;

    if (!(This = calloc(1, sizeof(*This))))
        return E_OUTOFMEMORY;

    This->IUnknown_inner.lpVtbl = &Unknown_vtbl;
    This->IMediaObject_iface.lpVtbl = &MediaObject_vtbl;
    This->ref = 1;
    This->outer = outer ? outer : &This->IUnknown_inner;

    mpg123_init();
    This->mh = mpg123_new(NULL, &err);
    mpg123_open_feed(This->mh);
    mpg123_format_none(This->mh);

    hr = IUnknown_QueryInterface(&This->IUnknown_inner, iid, obj);
    IUnknown_Release(&This->IUnknown_inner);
    return hr;
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID iid, void **obj)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), obj);

    if (IsEqualGUID(&IID_IUnknown, iid) ||
        IsEqualGUID(&IID_IClassFactory, iid))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    WARN("no interface for %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **obj)
{
    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(iid), obj);

    if (outer && !IsEqualGUID(iid, &IID_IUnknown))
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    return create_mp3_decoder(outer, iid, obj);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("(%d) stub\n", lock);
    return S_OK;
}

static const IClassFactoryVtbl classfactory_vtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory mp3_decoder_cf = { &classfactory_vtbl };

/*************************************************************************
 *              DllGetClassObject (DSDMO.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **obj)
{
    TRACE("%s, %s, %p\n", debugstr_guid(clsid), debugstr_guid(iid), obj);

    if (IsEqualGUID(clsid, &CLSID_CMP3DecMediaObject))
        return IClassFactory_QueryInterface(&mp3_decoder_cf, iid, obj);

    FIXME("class %s not available\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllRegisterServer (DSDMO.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    DMO_PARTIAL_MEDIATYPE in, out;
    HRESULT hr;

    in.type = WMMEDIATYPE_Audio;
    in.subtype = WMMEDIASUBTYPE_MP3;
    out.type = WMMEDIATYPE_Audio;
    out.subtype = WMMEDIASUBTYPE_PCM;
    hr = DMORegister(L"MP3 Decoder DMO", &CLSID_CMP3DecMediaObject, &DMOCATEGORY_AUDIO_DECODER,
        0, 1, &in, 1, &out);
    if (FAILED(hr)) return hr;

    return __wine_register_resources();
}

/***********************************************************************
 *              DllUnregisterServer (DSDMO.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    hr = DMOUnregister(&CLSID_CMP3DecMediaObject, &DMOCATEGORY_AUDIO_DECODER);
    if (FAILED(hr)) return hr;

    return __wine_unregister_resources();
}
