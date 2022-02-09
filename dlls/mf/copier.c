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

#include "mfapi.h"
#include "mfidl.h"
#include "mf_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum sample_copier_flags
{
    SAMPLE_COPIER_INPUT_TYPE_SET = 0x1,
    SAMPLE_COPIER_OUTPUT_TYPE_SET = 0x2
};

struct sample_copier
{
    IMFTransform IMFTransform_iface;
    LONG refcount;

    IMFAttributes *attributes;
    IMFMediaType *buffer_type;
    DWORD buffer_size;
    IMFSample *sample;
    DWORD flags;
    CRITICAL_SECTION cs;
};

static struct sample_copier *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct sample_copier, IMFTransform_iface);
}

static HRESULT WINAPI sample_copier_transform_QueryInterface(IMFTransform *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

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

static ULONG WINAPI sample_copier_transform_AddRef(IMFTransform *iface)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&transform->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sample_copier_transform_Release(IMFTransform *iface)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&transform->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (transform->attributes)
            IMFAttributes_Release(transform->attributes);
        if (transform->buffer_type)
            IMFMediaType_Release(transform->buffer_type);
        DeleteCriticalSection(&transform->cs);
        free(transform);
    }

    return refcount;
}

static HRESULT WINAPI sample_copier_transform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    TRACE("%p, %p, %p, %p, %p.\n", iface, input_minimum, input_maximum, output_minimum, output_maximum);

    *input_minimum = *input_maximum = *output_minimum = *output_maximum = 1;

    return S_OK;
}

static HRESULT WINAPI sample_copier_transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    TRACE("%p, %p, %p.\n", iface, inputs, outputs);

    *inputs = 1;
    *outputs = 1;

    return S_OK;
}

static HRESULT WINAPI sample_copier_transform_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    TRACE("%p, %lu, %p, %lu, %p.\n", iface, input_size, inputs, output_size, outputs);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_copier_transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);

    TRACE("%p, %lu, %p.\n", iface, id, info);

    memset(info, 0, sizeof(*info));

    EnterCriticalSection(&transform->cs);
    info->cbSize = transform->buffer_size;
    LeaveCriticalSection(&transform->cs);

    return S_OK;
}

static HRESULT WINAPI sample_copier_transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id,
        MFT_OUTPUT_STREAM_INFO *info)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);

    TRACE("%p, %lu, %p.\n", iface, id, info);

    memset(info, 0, sizeof(*info));

    EnterCriticalSection(&transform->cs);
    info->cbSize = transform->buffer_size;
    LeaveCriticalSection(&transform->cs);

    return S_OK;
}

static HRESULT WINAPI sample_copier_transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);

    TRACE("%p, %p.\n", iface, attributes);

    *attributes = transform->attributes;
    IMFAttributes_AddRef(*attributes);

    return S_OK;
}

static HRESULT WINAPI sample_copier_transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    TRACE("%p, %lu, %p.\n", iface, id, attributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_copier_transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    TRACE("%p, %lu, %p.\n", iface, id, attributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_copier_transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    TRACE("%p, %lu.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_copier_transform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    TRACE("%p, %lu, %p.\n", iface, streams, ids);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_copier_transform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    static const GUID *types[] = { &MFMediaType_Video, &MFMediaType_Audio };
    HRESULT hr;

    TRACE("%p, %lu, %lu, %p.\n", iface, id, index, type);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    if (index > ARRAY_SIZE(types) - 1)
        return MF_E_NO_MORE_TYPES;

    if (SUCCEEDED(hr = MFCreateMediaType(type)))
        hr = IMFMediaType_SetGUID(*type, &MF_MT_MAJOR_TYPE, types[index]);

    return hr;
}

static HRESULT WINAPI sample_copier_transform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);
    IMFMediaType *cloned_type = NULL;
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %lu, %p.\n", iface, id, index, type);

    EnterCriticalSection(&transform->cs);
    if (transform->buffer_type)
    {
        if (SUCCEEDED(hr = MFCreateMediaType(&cloned_type)))
            hr = IMFMediaType_CopyAllItems(transform->buffer_type, (IMFAttributes *)cloned_type);
    }
    else if (id)
        hr = MF_E_INVALIDSTREAMNUMBER;
    else
        hr = MF_E_NO_MORE_TYPES;
    LeaveCriticalSection(&transform->cs);

    if (SUCCEEDED(hr))
        *type = cloned_type;
    else if (cloned_type)
        IMFMediaType_Release(cloned_type);

    return hr;
}

static HRESULT sample_copier_get_buffer_size(IMFMediaType *type, UINT32 *size)
{
    GUID major, subtype;
    UINT64 frame_size;
    HRESULT hr;

    *size = 0;

    if (FAILED(hr = IMFMediaType_GetMajorType(type, &major)))
        return hr;

    if (IsEqualGUID(&major, &MFMediaType_Video))
    {
        if (SUCCEEDED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        {
            if (SUCCEEDED(hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
            {
                if (FAILED(hr = MFCalculateImageSize(&subtype, (UINT32)(frame_size >> 32), (UINT32)frame_size, size)))
                    WARN("Failed to get image size for video format %s.\n", debugstr_guid(&subtype));
            }
        }
    }
    else if (IsEqualGUID(&major, &MFMediaType_Audio))
    {
        FIXME("Audio formats are not handled.\n");
        hr = E_NOTIMPL;
    }

    return hr;
}

static HRESULT sample_copier_set_media_type(struct sample_copier *transform, BOOL input, DWORD id, IMFMediaType *type,
        DWORD flags)
{
    UINT32 buffer_size;
    HRESULT hr = S_OK;

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&transform->cs);
    if (type)
    {
        hr = sample_copier_get_buffer_size(type, &buffer_size);
        if (!(flags & MFT_SET_TYPE_TEST_ONLY) && SUCCEEDED(hr))
        {
            if (!transform->buffer_type)
                hr = MFCreateMediaType(&transform->buffer_type);
            if (SUCCEEDED(hr))
                hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *)transform->buffer_type);
            if (SUCCEEDED(hr))
                transform->buffer_size = buffer_size;

            if (SUCCEEDED(hr))
            {
                if (input)
                {
                    transform->flags |= SAMPLE_COPIER_INPUT_TYPE_SET;
                    transform->flags &= ~SAMPLE_COPIER_OUTPUT_TYPE_SET;
                }
                else
                    transform->flags |= SAMPLE_COPIER_OUTPUT_TYPE_SET;
            }
        }
    }
    else if (transform->buffer_type)
    {
        IMFMediaType_Release(transform->buffer_type);
        transform->buffer_type = NULL;
    }
    LeaveCriticalSection(&transform->cs);

    return hr;
}

static HRESULT WINAPI sample_copier_transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);

    TRACE("%p, %lu, %p, %#lx.\n", iface, id, type, flags);

    return sample_copier_set_media_type(transform, TRUE, id, type, flags);
}

static HRESULT WINAPI sample_copier_transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);

    TRACE("%p, %lu, %p, %#lx.\n", iface, id, type, flags);

    return sample_copier_set_media_type(transform, FALSE, id, type, flags);
}

static HRESULT sample_copier_get_current_type(struct sample_copier *transform, DWORD id, DWORD flags,
        IMFMediaType **ret)
{
    IMFMediaType *cloned_type = NULL;
    HRESULT hr;

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&transform->cs);
    if (transform->flags & flags)
    {
        if (SUCCEEDED(hr = MFCreateMediaType(&cloned_type)))
            hr = IMFMediaType_CopyAllItems(transform->buffer_type, (IMFAttributes *)cloned_type);
    }
    else
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    LeaveCriticalSection(&transform->cs);

    if (SUCCEEDED(hr))
        *ret = cloned_type;
    else if (cloned_type)
        IMFMediaType_Release(cloned_type);

    return hr;
}

static HRESULT WINAPI sample_copier_transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);

    TRACE("%p, %lu, %p.\n", iface, id, type);

    return sample_copier_get_current_type(transform, id, SAMPLE_COPIER_INPUT_TYPE_SET, type);
}

static HRESULT WINAPI sample_copier_transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);

    TRACE("%p, %lu, %p.\n", iface, id, type);

    return sample_copier_get_current_type(transform, id, SAMPLE_COPIER_OUTPUT_TYPE_SET, type);
}

static HRESULT WINAPI sample_copier_transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p.\n", iface, id, flags);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&transform->cs);
    if (!(transform->flags & SAMPLE_COPIER_INPUT_TYPE_SET))
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    else
        *flags = transform->sample ? 0 : MFT_INPUT_STATUS_ACCEPT_DATA;
    LeaveCriticalSection(&transform->cs);

    return hr;
}

static HRESULT WINAPI sample_copier_transform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, flags);

    EnterCriticalSection(&transform->cs);
    if (!(transform->flags & SAMPLE_COPIER_OUTPUT_TYPE_SET))
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    else
        *flags = transform->sample ? MFT_OUTPUT_STATUS_SAMPLE_READY : 0;
    LeaveCriticalSection(&transform->cs);

    return hr;
}

static HRESULT WINAPI sample_copier_transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    TRACE("%p, %s, %s.\n", iface, debugstr_time(lower), debugstr_time(upper));

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_copier_transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("%p, %lu, %p.\n", iface, id, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_copier_transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);

    TRACE("%p, %#x, %p.\n", iface, message, (void *)param);

    EnterCriticalSection(&transform->cs);

    if (message == MFT_MESSAGE_COMMAND_FLUSH)
    {
        if (transform->sample)
        {
            IMFSample_Release(transform->sample);
            transform->sample = NULL;
        }
    }

    LeaveCriticalSection(&transform->cs);

    return S_OK;
}

static HRESULT WINAPI sample_copier_transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p, %#lx.\n", iface, id, sample, flags);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&transform->cs);
    if (!transform->buffer_type)
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    else if (transform->sample)
        hr = MF_E_NOTACCEPTING;
    else
    {
        transform->sample = sample;
        IMFSample_AddRef(transform->sample);
    }
    LeaveCriticalSection(&transform->cs);

    return hr;
}

static HRESULT WINAPI sample_copier_transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct sample_copier *transform = impl_from_IMFTransform(iface);
    IMFMediaBuffer *buffer;
    DWORD sample_flags;
    HRESULT hr = S_OK;
    LONGLONG time;

    TRACE("%p, %#lx, %lu, %p, %p.\n", iface, flags, count, buffers, status);

    EnterCriticalSection(&transform->cs);
    if (!(transform->flags & SAMPLE_COPIER_OUTPUT_TYPE_SET))
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    else if (!transform->sample)
        hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
    else
    {
        IMFSample_CopyAllItems(transform->sample, (IMFAttributes *)buffers->pSample);

        if (SUCCEEDED(IMFSample_GetSampleDuration(transform->sample, &time)))
            IMFSample_SetSampleDuration(buffers->pSample, time);

        if (SUCCEEDED(IMFSample_GetSampleTime(transform->sample, &time)))
            IMFSample_SetSampleTime(buffers->pSample, time);

        if (SUCCEEDED(IMFSample_GetSampleFlags(transform->sample, &sample_flags)))
            IMFSample_SetSampleFlags(buffers->pSample, sample_flags);

        if (SUCCEEDED(IMFSample_ConvertToContiguousBuffer(transform->sample, NULL)))
        {
            if (SUCCEEDED(IMFSample_GetBufferByIndex(buffers->pSample, 0, &buffer)))
            {
                if (FAILED(IMFSample_CopyToBuffer(transform->sample, buffer)))
                    hr = MF_E_UNEXPECTED;
                IMFMediaBuffer_Release(buffer);
            }
        }

        IMFSample_Release(transform->sample);
        transform->sample = NULL;
    }
    LeaveCriticalSection(&transform->cs);

    return hr;
}

static const IMFTransformVtbl sample_copier_transform_vtbl =
{
    sample_copier_transform_QueryInterface,
    sample_copier_transform_AddRef,
    sample_copier_transform_Release,
    sample_copier_transform_GetStreamLimits,
    sample_copier_transform_GetStreamCount,
    sample_copier_transform_GetStreamIDs,
    sample_copier_transform_GetInputStreamInfo,
    sample_copier_transform_GetOutputStreamInfo,
    sample_copier_transform_GetAttributes,
    sample_copier_transform_GetInputStreamAttributes,
    sample_copier_transform_GetOutputStreamAttributes,
    sample_copier_transform_DeleteInputStream,
    sample_copier_transform_AddInputStreams,
    sample_copier_transform_GetInputAvailableType,
    sample_copier_transform_GetOutputAvailableType,
    sample_copier_transform_SetInputType,
    sample_copier_transform_SetOutputType,
    sample_copier_transform_GetInputCurrentType,
    sample_copier_transform_GetOutputCurrentType,
    sample_copier_transform_GetInputStatus,
    sample_copier_transform_GetOutputStatus,
    sample_copier_transform_SetOutputBounds,
    sample_copier_transform_ProcessEvent,
    sample_copier_transform_ProcessMessage,
    sample_copier_transform_ProcessInput,
    sample_copier_transform_ProcessOutput,
};

BOOL mf_is_sample_copier_transform(IMFTransform *transform)
{
    return transform->lpVtbl == &sample_copier_transform_vtbl;
}

/***********************************************************************
 *      MFCreateSampleCopierMFT (mf.@)
 */
HRESULT WINAPI MFCreateSampleCopierMFT(IMFTransform **transform)
{
    struct sample_copier *object;
    HRESULT hr;

    TRACE("%p.\n", transform);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFTransform_iface.lpVtbl = &sample_copier_transform_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateAttributes(&object->attributes, 0)))
        goto failed;

    IMFAttributes_SetUINT32(object->attributes, &MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, 1);

    *transform = &object->IMFTransform_iface;

    return S_OK;

failed:

    IMFTransform_Release(&object->IMFTransform_iface);

    return hr;
}
