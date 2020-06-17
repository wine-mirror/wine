/*
 * Copyright 2020 Nikolay Sivov
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

#include "wine/debug.h"
#include "evr.h"
#include "d3d9.h"

#include "evr_classes.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

struct video_mixer
{
    IMFTransform IMFTransform_iface;
    LONG refcount;
};

static struct video_mixer *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFTransform_iface);
}

static HRESULT WINAPI video_mixer_transform_QueryInterface(IMFTransform *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFTransform) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFTransform_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI video_mixer_transform_AddRef(IMFTransform *iface)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&mixer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI video_mixer_transform_Release(IMFTransform *iface)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&mixer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
        free(mixer);

    return refcount;
}

static HRESULT WINAPI video_mixer_transform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    FIXME("%p, %p, %p, %p, %p.\n", iface, input_minimum, input_maximum, output_minimum, output_maximum);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    FIXME("%p, %p, %p.\n", iface, inputs, outputs);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    FIXME("%p, %u, %p, %u, %p.\n", iface, input_size, inputs, output_size, outputs);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    FIXME("%p, %u, %p.\n", iface, id, info);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    FIXME("%p, %u, %p.\n", iface, id, info);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    FIXME("%p, %p.\n", iface, attributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    FIXME("%p, %u, %p.\n", iface, id, attributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    FIXME("%p, %u, %p.\n", iface, id, attributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    FIXME("%p, %u.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    FIXME("%p, %u, %p.\n", iface, streams, ids);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("%p, %u, %u, %p.\n", iface, id, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("%p, %u, %u, %p.\n", iface, id, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    FIXME("%p, %u, %p, %#x.\n", iface, id, type, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    FIXME("%p, %u, %p, %#x.\n", iface, id, type, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("%p, %u, %p.\n", iface, id, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("%p, %u, %p.\n", iface, id, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("%p, %u, %p.\n", iface, id, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("%p, %p.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    FIXME("%p, %s, %s.\n", iface, wine_dbgstr_longlong(lower), wine_dbgstr_longlong(upper));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("%p, %u, %p.\n", iface, id, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("%p, %u, %#lx.\n", iface, message, param);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    FIXME("%p, %u, %p, %#x.\n", iface, id, sample, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    FIXME("%p, %#x, %u, %p, %p.\n", iface, flags, count, samples, status);

    return E_NOTIMPL;
}

static const IMFTransformVtbl video_mixer_transform_vtbl =
{
    video_mixer_transform_QueryInterface,
    video_mixer_transform_AddRef,
    video_mixer_transform_Release,
    video_mixer_transform_GetStreamLimits,
    video_mixer_transform_GetStreamCount,
    video_mixer_transform_GetStreamIDs,
    video_mixer_transform_GetInputStreamInfo,
    video_mixer_transform_GetOutputStreamInfo,
    video_mixer_transform_GetAttributes,
    video_mixer_transform_GetInputStreamAttributes,
    video_mixer_transform_GetOutputStreamAttributes,
    video_mixer_transform_DeleteInputStream,
    video_mixer_transform_AddInputStreams,
    video_mixer_transform_GetInputAvailableType,
    video_mixer_transform_GetOutputAvailableType,
    video_mixer_transform_SetInputType,
    video_mixer_transform_SetOutputType,
    video_mixer_transform_GetInputCurrentType,
    video_mixer_transform_GetOutputCurrentType,
    video_mixer_transform_GetInputStatus,
    video_mixer_transform_GetOutputStatus,
    video_mixer_transform_SetOutputBounds,
    video_mixer_transform_ProcessEvent,
    video_mixer_transform_ProcessMessage,
    video_mixer_transform_ProcessInput,
    video_mixer_transform_ProcessOutput,
};

HRESULT WINAPI MFCreateVideoMixer(IUnknown *owner, REFIID riid_device, REFIID riid, void **obj)
{
    TRACE("%p, %s, %s, %p.\n", owner, debugstr_guid(riid_device), debugstr_guid(riid), obj);

    *obj = NULL;

    if (!IsEqualIID(riid_device, &IID_IDirect3DDevice9))
        return E_INVALIDARG;

    return CoCreateInstance(&CLSID_MFVideoMixer9, owner, CLSCTX_INPROC_SERVER, riid, obj);
}

HRESULT evr_mixer_create(IUnknown *outer, void **out)
{
    struct video_mixer *object;

    if (outer)
        return E_NOINTERFACE;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFTransform_iface.lpVtbl = &video_mixer_transform_vtbl;
    object->refcount = 1;

    *out = &object->IMFTransform_iface;

    return S_OK;
}
