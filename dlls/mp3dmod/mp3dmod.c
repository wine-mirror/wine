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
#include "mfapi.h"
#include "mferror.h"
#include "mftransform.h"
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
    IMFTransform IMFTransform_iface;
    IUnknown *outer;
    LONG ref;
    mpg123_handle *mh;

    DMO_MEDIA_TYPE intype, outtype;
    BOOL intype_set, outtype_set;

    IMediaBuffer *buffer;
    REFERENCE_TIME timestamp;

    BOOL draining;
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
    else if (IsEqualGUID(iid, &IID_IMFTransform))
        *obj = &This->IMFTransform_iface;
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
    const WAVEFORMATEX *format;

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

    format = (WAVEFORMATEX *) type->pbFormat;

    if (!format->nChannels)
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
    WAVEFORMATEX *format, *in_format;
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
    else if (format->wBitsPerSample == 32)
        enc = MPG123_ENC_FLOAT_32;
    else
    {
        ERR("Cannot decode to bit depth %u.\n", format->wBitsPerSample);
        return DMO_E_TYPE_NOT_ACCEPTED;
    }

    if (format->nChannels * format->wBitsPerSample/8 != format->nBlockAlign
            || format->nSamplesPerSec * format->nBlockAlign != format->nAvgBytesPerSec)
        return E_INVALIDARG;

    in_format = (WAVEFORMATEX *)This->intype.pbFormat;

    if (format->nSamplesPerSec != in_format->nSamplesPerSec &&
            format->nSamplesPerSec*2 != in_format->nSamplesPerSec &&
            format->nSamplesPerSec*4 != in_format->nSamplesPerSec)
    {
        ERR("Cannot decode to %lu samples per second (input %lu).\n", format->nSamplesPerSec, in_format->nSamplesPerSec);
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

static HRESULT convert_dmo_to_mf_error(HRESULT dmo_error)
{
    switch (dmo_error)
    {
        case DMO_E_INVALIDSTREAMINDEX:
            return MF_E_INVALIDSTREAMNUMBER;

        case DMO_E_INVALIDTYPE:
        case DMO_E_TYPE_NOT_ACCEPTED:
            return MF_E_INVALIDMEDIATYPE;

        case DMO_E_TYPE_NOT_SET:
            return MF_E_TRANSFORM_TYPE_NOT_SET;

        case DMO_E_NOTACCEPTING:
            return MF_E_NOTACCEPTING;

        case DMO_E_NO_MORE_ITEMS:
            return MF_E_NO_MORE_TYPES;
    }

    return dmo_error;
}

static inline struct mp3_decoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct mp3_decoder, IMFTransform_iface);
}

static HRESULT WINAPI MFTransform_QueryInterface(IMFTransform *iface, REFIID iid, void **obj)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    return IUnknown_QueryInterface(decoder->outer, iid, obj);
}

static ULONG WINAPI MFTransform_AddRef(IMFTransform *iface)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    return IUnknown_AddRef(decoder->outer);
}

static ULONG WINAPI MFTransform_Release(IMFTransform *iface)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    return IUnknown_Release(decoder->outer);
}

static HRESULT WINAPI MFTransform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, input_minimum %p, input_maximum %p, output_minimum %p, output_maximum %p.\n",
            iface, input_minimum, input_maximum, output_minimum, output_maximum);
    IMediaObject_GetStreamCount(&decoder->IMediaObject_iface, input_minimum, output_minimum);
    *input_maximum = *input_minimum;
    *output_maximum = *output_minimum;
    return S_OK;
}

static HRESULT WINAPI MFTransform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, inputs %p, outputs %p.\n", iface, inputs, outputs);
    IMediaObject_GetStreamCount(&decoder->IMediaObject_iface, inputs, outputs);
    return S_OK;
}

static HRESULT WINAPI MFTransform_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    FIXME("iface %p, input_size %lu, inputs %p, output_size %lu, outputs %p stub!\n", iface,
            input_size, inputs, output_size, outputs);

    return E_NOTIMPL;
}

static HRESULT WINAPI MFTransform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    memset(info, 0, sizeof(*info));

    /* Doc says: If not implemented, assume zero latency. */
    if (FAILED(hr = IMediaObject_GetInputMaxLatency(&decoder->IMediaObject_iface, id, &info->hnsMaxLatency)) && hr != E_NOTIMPL)
        return convert_dmo_to_mf_error(hr);

    if (FAILED(hr = IMediaObject_GetInputStreamInfo(&decoder->IMediaObject_iface, id, &info->dwFlags)))
        return convert_dmo_to_mf_error(hr);

    if (FAILED(hr = IMediaObject_GetInputSizeInfo(&decoder->IMediaObject_iface, id, &info->cbSize, &info->cbMaxLookahead, &info->cbAlignment)))
        return convert_dmo_to_mf_error(hr);

    return S_OK;
}

static HRESULT WINAPI MFTransform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    memset(info, 0, sizeof(*info));

    if (FAILED(hr = IMediaObject_GetOutputStreamInfo(&decoder->IMediaObject_iface, id, &info->dwFlags)))
        return convert_dmo_to_mf_error(hr);

    if (FAILED(hr = IMediaObject_GetOutputSizeInfo(&decoder->IMediaObject_iface, id, &info->cbSize, &info->cbAlignment)))
        return convert_dmo_to_mf_error(hr);

    return S_OK;
}

static HRESULT WINAPI MFTransform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    TRACE("iface %p, attributes %p.\n", iface, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFTransform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFTransform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFTransform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    TRACE("iface %p, id %#lx.\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFTransform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    TRACE("iface %p, streams %lu, ids %p.\n", iface, streams, ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFTransform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    DMO_MEDIA_TYPE pt;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    if (FAILED(hr = IMediaObject_GetInputType(&decoder->IMediaObject_iface, id, index, &pt)))
        return convert_dmo_to_mf_error(hr);

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    hr = MFInitMediaTypeFromAMMediaType(*type, (AM_MEDIA_TYPE*)&pt);
    MoFreeMediaType(&pt);
    return hr;
}

static HRESULT WINAPI MFTransform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    UINT32 bps, block_align, num_channels;
    const WAVEFORMATEX *input_format;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!decoder->intype_set)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    input_format = (WAVEFORMATEX *)decoder->intype.pbFormat;

    if (index >= 6 || (input_format->nChannels != 2 && index >= 3))
        return MF_E_NO_MORE_TYPES;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(*type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio)))
        goto fail;

    if (FAILED(hr = IMFMediaType_SetGUID(*type, &MF_MT_SUBTYPE, index % 3 ? &MFAudioFormat_PCM : &MFAudioFormat_Float)))
        goto fail;

    if (index % 3 == 0)
        bps = 32;
    else if (index % 3 == 1)
        bps = 16;
    else
        bps = 8;

    num_channels = index / 3 ? 1 : input_format->nChannels;
    block_align = bps/8 * num_channels;

    if (FAILED(hr = IMFMediaType_SetUINT32(*type, &MF_MT_AUDIO_BITS_PER_SAMPLE, bps)))
        goto fail;

    if (FAILED(hr = IMFMediaType_SetUINT32(*type, &MF_MT_AUDIO_NUM_CHANNELS, num_channels)))
        goto fail;

    if (FAILED(hr = IMFMediaType_SetUINT32(*type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, input_format->nSamplesPerSec)))
        goto fail;

    if (FAILED(hr = IMFMediaType_SetUINT32(*type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, input_format->nSamplesPerSec * block_align)))
        goto fail;

    if (FAILED(hr = IMFMediaType_SetUINT32(*type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, block_align)))
        goto fail;

    if (FAILED(hr = IMFMediaType_SetUINT32(*type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1)))
        goto fail;

    return S_OK;

fail:
    IMFMediaType_Release(*type);
    return hr;
}

static HRESULT WINAPI MFTransform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    DMO_MEDIA_TYPE mt;
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (!type)
        flags |= DMO_SET_TYPEF_CLEAR;
    else if (FAILED(hr = MFInitAMMediaTypeFromMFMediaType(type, GUID_NULL, (AM_MEDIA_TYPE*)&mt)))
        return hr;

    hr = IMediaObject_SetInputType(&decoder->IMediaObject_iface, id, &mt, flags);
    MoFreeMediaType(&mt);

    if (hr == S_FALSE)
        return MF_E_INVALIDMEDIATYPE;
    else if (FAILED(hr))
        return convert_dmo_to_mf_error(hr);

    return S_OK;
}

static HRESULT WINAPI MFTransform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    DMO_MEDIA_TYPE mt;
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (!type)
        flags |= DMO_SET_TYPEF_CLEAR;
    else if (FAILED(hr = MFInitAMMediaTypeFromMFMediaType(type, GUID_NULL, (AM_MEDIA_TYPE*)&mt)))
        return hr;

    hr = IMediaObject_SetOutputType(&decoder->IMediaObject_iface, id, &mt, flags);
    MoFreeMediaType(&mt);

    if (hr == S_FALSE)
        return MF_E_INVALIDMEDIATYPE;
    else if (FAILED(hr))
        return convert_dmo_to_mf_error(hr);

    return S_OK;
}

static HRESULT WINAPI MFTransform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **out)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    DMO_MEDIA_TYPE mt;
    HRESULT hr;

    TRACE("iface %p, id %#lx, out %p.\n", iface, id, out);

    if (FAILED(hr = IMediaObject_GetInputCurrentType(&decoder->IMediaObject_iface, id, &mt)))
        return convert_dmo_to_mf_error(hr);

    if (FAILED(hr = MFCreateMediaType(out)))
        return hr;

    hr = MFInitMediaTypeFromAMMediaType(*out, (AM_MEDIA_TYPE*)&mt);
    MoFreeMediaType(&mt);
    return hr;
}

static HRESULT WINAPI MFTransform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **out)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    DMO_MEDIA_TYPE mt;
    HRESULT hr;

    TRACE("iface %p, id %#lx, out %p.\n", iface, id, out);

    if (FAILED(hr = IMediaObject_GetOutputCurrentType(&decoder->IMediaObject_iface, id, &mt)))
        return convert_dmo_to_mf_error(hr);

    if (FAILED(hr = MFCreateMediaType(out)))
        return hr;

    hr = MFInitMediaTypeFromAMMediaType(*out, (AM_MEDIA_TYPE*)&mt);
    MoFreeMediaType(&mt);
    return hr;
}

static HRESULT WINAPI MFTransform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{

    FIXME("iface %p, id %#lx, flags %p stub!\n", iface, id, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI MFTransform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFTransform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    FIXME("iface %p, lower %I64d, upper %I64d stub!\n", iface, lower, upper);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFTransform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %#lx, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFTransform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, message %#x, param %p stub!\n", iface, message, (void *)param);

    switch(message)
    {
        case MFT_MESSAGE_COMMAND_FLUSH:
            if (FAILED(hr = IMediaObject_Flush(&decoder->IMediaObject_iface)))
                return convert_dmo_to_mf_error(hr);
            break;

        case MFT_MESSAGE_COMMAND_DRAIN:
            decoder->draining = TRUE;
            break;

        case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
        case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
            if (FAILED(hr = IMediaObject_AllocateStreamingResources(&decoder->IMediaObject_iface)))
                return convert_dmo_to_mf_error(hr);
            break;

        case MFT_MESSAGE_NOTIFY_END_OF_STREAM:
        case MFT_MESSAGE_NOTIFY_END_STREAMING:
            if (FAILED(hr = IMediaObject_FreeStreamingResources(&decoder->IMediaObject_iface)))
                return convert_dmo_to_mf_error(hr);
            break;

        default:
            FIXME("message %#x stub!\n", message);
            break;
    }

    return S_OK;
}

static HRESULT WINAPI MFTransform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    LONGLONG sample_time, duration;
    IMediaBuffer *dmo_buffer;
    IMFMediaBuffer *buffer;
    UINT32 discontinuity;
    HRESULT hr;

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (decoder->draining)
        return MF_E_NOTACCEPTING;

    if (SUCCEEDED(IMFSample_GetUINT32(sample, &MFSampleExtension_Discontinuity, &discontinuity)) && discontinuity)
    {
        if (FAILED(hr = IMediaObject_Discontinuity(&decoder->IMediaObject_iface, id)))
            return convert_dmo_to_mf_error(hr);
    }

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer)))
        return hr;

    hr = MFCreateLegacyMediaBufferOnMFMediaBuffer(sample, buffer, 0, &dmo_buffer);
    IMFMediaBuffer_Release(buffer);
    if (FAILED(hr))
        return hr;

    flags = 0;

    if (SUCCEEDED(IMFSample_GetSampleTime(sample, &sample_time)))
        flags |= DMO_INPUT_DATA_BUFFERF_TIME;

    if (SUCCEEDED(IMFSample_GetSampleDuration(sample, &duration)))
        flags |= DMO_INPUT_DATA_BUFFERF_TIMELENGTH;

    hr = IMediaObject_ProcessInput(&decoder->IMediaObject_iface, id, dmo_buffer, flags, sample_time, duration);
    IMediaBuffer_Release(dmo_buffer);
    if (FAILED(hr))
        return convert_dmo_to_mf_error(hr);

    return S_OK;
}

static HRESULT WINAPI MFTransform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct mp3_decoder *decoder = impl_from_IMFTransform(iface);
    DMO_OUTPUT_DATA_BUFFER *dmo_output;
    IMFMediaBuffer *buffer;
    HRESULT hr;
    int i;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count > 1)
    {
        FIXME("Multiple buffers not handled.\n");
        return E_INVALIDARG;
    }

    dmo_output = calloc(count, sizeof(*dmo_output));
    if (!dmo_output)
        return E_OUTOFMEMORY;

    for (i = 0; i < count; i++)
    {
        if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(samples[i].pSample, &buffer)))
            goto fail;

        hr = MFCreateLegacyMediaBufferOnMFMediaBuffer(samples[i].pSample, buffer, 0, &dmo_output[i].pBuffer);
        IMFMediaBuffer_Release(buffer);
        if (FAILED(hr))
            goto fail;
    }

    hr = IMediaObject_ProcessOutput(&decoder->IMediaObject_iface, flags, count, dmo_output, status);

    for (i = 0; i < count; i++)
        IMediaBuffer_Release(dmo_output[i].pBuffer);

    if (FAILED(hr))
    {
        free(dmo_output);
        return convert_dmo_to_mf_error(hr);
    }

    for (i = 0; i < count; i++)
    {
        if (dmo_output[i].dwStatus & DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT)
            IMFSample_SetUINT32(samples[i].pSample, &MFSampleExtension_CleanPoint, 1);

        if (dmo_output[i].dwStatus & DMO_OUTPUT_DATA_BUFFERF_TIME)
            IMFSample_SetSampleTime(samples[i].pSample, dmo_output[i].rtTimestamp);

        if (dmo_output[i].dwStatus & DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH)
            IMFSample_SetSampleDuration(samples[i].pSample, dmo_output[i].rtTimelength);

        if (dmo_output[i].dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE)
            samples[i].dwStatus |= MFT_OUTPUT_DATA_BUFFER_INCOMPLETE;
    }

    free(dmo_output);

    if (hr == S_FALSE)
    {
        decoder->draining = FALSE;
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    return S_OK;

fail:
    for (i = 0; i < count; i++)
    {
        if (dmo_output[i].pBuffer)
            IMediaBuffer_Release(dmo_output[i].pBuffer);
    }
    free(dmo_output);

    return hr;
}

static const IMFTransformVtbl IMFTransform_vtbl =
{
    MFTransform_QueryInterface,
    MFTransform_AddRef,
    MFTransform_Release,
    MFTransform_GetStreamLimits,
    MFTransform_GetStreamCount,
    MFTransform_GetStreamIDs,
    MFTransform_GetInputStreamInfo,
    MFTransform_GetOutputStreamInfo,
    MFTransform_GetAttributes,
    MFTransform_GetInputStreamAttributes,
    MFTransform_GetOutputStreamAttributes,
    MFTransform_DeleteInputStream,
    MFTransform_AddInputStreams,
    MFTransform_GetInputAvailableType,
    MFTransform_GetOutputAvailableType,
    MFTransform_SetInputType,
    MFTransform_SetOutputType,
    MFTransform_GetInputCurrentType,
    MFTransform_GetOutputCurrentType,
    MFTransform_GetInputStatus,
    MFTransform_GetOutputStatus,
    MFTransform_SetOutputBounds,
    MFTransform_ProcessEvent,
    MFTransform_ProcessMessage,
    MFTransform_ProcessInput,
    MFTransform_ProcessOutput,
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
    This->IMFTransform_iface.lpVtbl = &IMFTransform_vtbl;
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
    MFT_REGISTER_TYPE_INFO mf_in, mf_out;
    DMO_PARTIAL_MEDIATYPE in, out;
    HRESULT hr;

    in.type = WMMEDIATYPE_Audio;
    in.subtype = WMMEDIASUBTYPE_MP3;
    out.type = WMMEDIATYPE_Audio;
    out.subtype = WMMEDIASUBTYPE_PCM;
    mf_in.guidMajorType = MFMediaType_Audio;
    mf_in.guidSubtype = MFAudioFormat_MP3;
    mf_out.guidMajorType = MFMediaType_Audio;
    mf_out.guidSubtype = MFAudioFormat_PCM;
    hr = DMORegister(L"MP3 Decoder DMO", &CLSID_CMP3DecMediaObject, &DMOCATEGORY_AUDIO_DECODER,
        0, 1, &in, 1, &out);
    if (FAILED(hr)) return hr;
    hr = MFTRegister(CLSID_CMP3DecMediaObject, MFT_CATEGORY_AUDIO_DECODER, (LPWSTR) L"MP3 Decoder MFT",
        0, 1, &mf_in, 1, &mf_out, NULL);
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
    hr = MFTUnregister(CLSID_CMP3DecMediaObject);
    if (FAILED(hr)) return hr;

    return __wine_unregister_resources();
}
